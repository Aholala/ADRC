/**
 * @file bsp_jlink_rtt.cpp
 * @author OrangeHan(chenzihan060319@foxmail.com)
 * @brief J-Link RTT 的 Bsp 轻量封装实现。
 * @version V26.0.1
 * @date 2026-03-29
 * @details
 * - 为 ACE_ECF_26 提供非阻塞 RTT 日志读写能力。
 * - 通过独立接口接入，不影响原有串口/其他日志链路。
 * - 支持 LOG_RTT_I/W/E 日志前缀中的时间戳与文件行号可选开关。
 *
 * @example
 * #include "bsp_jlink_rtt.hpp"
 *
 * void rtt_demo()
 * {
 *     BSP_n::JLinkRTT_c::init();
 *     LOG_RTT_I("RTT demo start");
 *     LOG_RTT_W("battery low: %.2fV", 22.8f);
 *     LOG_RTT_E("motor offline id=%d", 3);
 *     JLINK_RTT_LOG("raw value=%d\r\n", 42);
 * }
 */
#include "bsp_jlink_rtt.hpp"

#include <cstdio>
#include <cstring>

extern "C" {
#include "cmsis_os.h"
}

#if (JLINK_RTT_ASYNC_QUEUE_DEPTH < 2)
#error "JLINK_RTT_ASYNC_QUEUE_DEPTH must be >= 2"
#endif

#if (JLINK_RTT_ASYNC_LINE_MAX < 32)
#error "JLINK_RTT_ASYNC_LINE_MAX must be >= 32"
#endif

namespace
{
#if JLINK_RTT_ASYNC_ENABLE
struct AsyncLogLine_t
{
    uint16_t len = 0;
    char data[JLINK_RTT_ASYNC_LINE_MAX] = {0};
};

static AsyncLogLine_t s_async_queue[JLINK_RTT_ASYNC_QUEUE_DEPTH];
static volatile uint16_t s_async_head = 0;
static volatile uint16_t s_async_tail = 0;
static volatile uint32_t s_dropped_cnt = 0;
static volatile uint32_t s_truncated_cnt = 0;
#endif

static inline uint32_t _irq_lock()
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void _irq_unlock(uint32_t primask)
{
    __set_PRIMASK(primask);
}

#if JLINK_RTT_ASYNC_ENABLE
static inline uint32_t _counter_read(volatile uint32_t* counter)
{
    uint32_t primask = _irq_lock();
    uint32_t value = *counter;
    _irq_unlock(primask);
    return value;
}

static inline void _counter_inc(volatile uint32_t* counter)
{
    uint32_t primask = _irq_lock();
    *counter = *counter + 1U;
    _irq_unlock(primask);
}

static bool _queue_push(const char* data, uint16_t len)
{
    if ((data == nullptr) || (len == 0))
    {
        return false;
    }

    uint16_t used_len = len;
    if (used_len > JLINK_RTT_ASYNC_LINE_MAX)
    {
        used_len = JLINK_RTT_ASYNC_LINE_MAX;
        _counter_inc(&s_truncated_cnt);
    }

    uint32_t primask = _irq_lock();
    uint16_t next = static_cast<uint16_t>((s_async_head + 1U) % JLINK_RTT_ASYNC_QUEUE_DEPTH);
    if (next == s_async_tail)
    {
        s_dropped_cnt = s_dropped_cnt + 1U;
        _irq_unlock(primask);
        return false;
    }

    AsyncLogLine_t& slot = s_async_queue[s_async_head];
    memcpy(slot.data, data, used_len);
    slot.len = used_len;
    s_async_head = next;
    _irq_unlock(primask);
    return true;
}

static uint16_t _queue_pop(char* out_data, uint16_t out_max_len)
{
    if ((out_data == nullptr) || (out_max_len == 0))
    {
        return 0;
    }

    uint32_t primask = _irq_lock();
    if (s_async_tail == s_async_head)
    {
        _irq_unlock(primask);
        return 0;
    }

    AsyncLogLine_t& slot = s_async_queue[s_async_tail];
    uint16_t out_len = slot.len;
    if (out_len > out_max_len)
    {
        out_len = out_max_len;
        s_truncated_cnt = s_truncated_cnt + 1U;
    }
    memcpy(out_data, slot.data, out_len);
    s_async_tail = static_cast<uint16_t>((s_async_tail + 1U) % JLINK_RTT_ASYNC_QUEUE_DEPTH);
    _irq_unlock(primask);
    return out_len;
}
#endif

static inline void _append_cstr(char* line, uint32_t* used_len, const char* str, bool* truncated)
{
    if ((line == nullptr) || (used_len == nullptr) || (str == nullptr))
    {
        return;
    }

    if (*used_len >= (JLINK_RTT_ASYNC_LINE_MAX - 1U))
    {
        if (truncated != nullptr)
        {
            *truncated = true;
        }
        return;
    }

    uint32_t remain = (JLINK_RTT_ASYNC_LINE_MAX - 1U) - *used_len;
    uint32_t src_len = static_cast<uint32_t>(strlen(str));
    uint32_t copy_len = (src_len <= remain) ? src_len : remain;
    memcpy(line + *used_len, str, copy_len);
    *used_len += copy_len;
    line[*used_len] = '\0';

    if (copy_len < src_len && truncated != nullptr)
    {
        *truncated = true;
    }
}

static const char* _file_name_only(const char* path)
{
    if (path == nullptr)
    {
        return "";
    }

    const char* file = path;
    for (const char* p = path; *p != '\0'; ++p)
    {
        if ((*p == '\\') || (*p == '/'))
        {
            file = p + 1;
        }
    }
    return file;
}

static const char* _level_color(const char* level)
{
#if JLINK_RTT_LOG_ENABLE_COLOR
    if (level == nullptr)
    {
        return "";
    }
    if (strcmp(level, "I") == 0)
    {
        return JLINK_RTT_LOG_COLOR_INFO;
    }
    if (strcmp(level, "W") == 0)
    {
        return JLINK_RTT_LOG_COLOR_WARN;
    }
    if (strcmp(level, "E") == 0)
    {
        return JLINK_RTT_LOG_COLOR_ERROR;
    }
#endif
    return "";
}

static inline bool _fmt_need_format(const char* fmt)
{
    if (fmt == nullptr)
    {
        return false;
    }
    return (strchr(fmt, '%') != nullptr);
}

static int _terminal_out_string(uint8_t terminal_id, const char* str)
{
    if (str == nullptr)
    {
        return 0;
    }
    return SEGGER_RTT_TerminalOut(static_cast<unsigned char>(terminal_id), str);
}
}

namespace BSP_n
{
bool JLinkRTT_c::is_inited_ = false;
uint8_t JLinkRTT_c::up_channel_ = 0;
uint8_t JLinkRTT_c::down_channel_ = 0;

void JLinkRTT_c::init(const Config_t& config)
{
    if (is_inited_)
    {
        return;
    }

    SEGGER_RTT_Init();

    up_channel_ = config.up_channel;
    down_channel_ = config.down_channel;

    SEGGER_RTT_SetFlagsUpBuffer(up_channel_, config.up_mode & SEGGER_RTT_MODE_MASK);
    SEGGER_RTT_SetFlagsDownBuffer(down_channel_, config.down_mode & SEGGER_RTT_MODE_MASK);

    is_inited_ = true;
}

void JLinkRTT_c::init()
{
    init(Config_t{});
}

uint32_t JLinkRTT_c::write(const void* data, uint32_t len)
{
    if (!is_inited_)
    {
        init();
    }

    if (data == nullptr || len == 0)
    {
        return 0;
    }

#if JLINK_RTT_ASYNC_ENABLE
    const char* bytes = static_cast<const char*>(data);
    uint32_t total_write = 0;
    uint32_t left = len;
    while (left > 0)
    {
        uint16_t chunk = static_cast<uint16_t>((left > JLINK_RTT_ASYNC_LINE_MAX) ? JLINK_RTT_ASYNC_LINE_MAX : left);
        if (!_queue_push(bytes, chunk))
        {
            break;
        }
        bytes += chunk;
        left -= chunk;
        total_write += chunk;
    }
    return total_write;
#else
    return SEGGER_RTT_Write(up_channel_, data, len);
#endif
}

uint32_t JLinkRTT_c::write_string(const char* str)
{
    if (!is_inited_)
    {
        init();
    }

    if (str == nullptr)
    {
        return 0;
    }

    return write(str, static_cast<uint32_t>(strlen(str)));
}

uint32_t JLinkRTT_c::read(void* data, uint32_t len)
{
    if (!is_inited_)
    {
        init();
    }

    if (data == nullptr || len == 0)
    {
        return 0;
    }

    return SEGGER_RTT_Read(down_channel_, data, len);
}

int JLinkRTT_c::vprintf(const char* fmt, va_list args)
{
    if (fmt == nullptr)
    {
        return 0;
    }

    char buffer[JLINK_RTT_ASYNC_LINE_MAX];
    int formatted_len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (formatted_len <= 0)
    {
        return formatted_len;
    }

    uint32_t write_len = static_cast<uint32_t>(formatted_len);
    if (write_len >= sizeof(buffer))
    {
        write_len = sizeof(buffer) - 1;
#if JLINK_RTT_ASYNC_ENABLE
        _counter_inc(&s_truncated_cnt);
#endif
    }

    return static_cast<int>(write(buffer, write_len));
}

int JLinkRTT_c::printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);

    return ret;
}

int JLinkRTT_c::terminal_vprintf(uint8_t terminal_id, const char* fmt, va_list args)
{
    if (!is_inited_)
    {
        init();
    }

    if (fmt == nullptr)
    {
        return 0;
    }

    char buffer[JLINK_RTT_ASYNC_LINE_MAX] = {0};
    int formatted_len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (formatted_len <= 0)
    {
        return formatted_len;
    }

    if (static_cast<uint32_t>(formatted_len) >= sizeof(buffer))
    {
#if JLINK_RTT_ASYNC_ENABLE
        _counter_inc(&s_truncated_cnt);
#endif
    }

    return _terminal_out_string(terminal_id, buffer);
}

int JLinkRTT_c::terminal_printf(uint8_t terminal_id, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = terminal_vprintf(terminal_id, fmt, args);
    va_end(args);

    return ret;
}

int JLinkRTT_c::log_string_with_level(const char* level, const char* file, int line, const char* msg)
{
    if (msg == nullptr)
    {
        return 0;
    }

    char out_line[JLINK_RTT_ASYNC_LINE_MAX] = {0};
    uint32_t used_len = 0;
    bool truncated = false;
    const char* color_code = _level_color(level);

#if JLINK_RTT_LOG_ENABLE_COLOR
    const char* color_reset = JLINK_RTT_LOG_COLOR_RESET;
#else
    const char* color_reset = "";
#endif

    if (color_code[0] != '\0')
    {
        _append_cstr(out_line, &used_len, color_code, &truncated);
    }

    if (used_len < (sizeof(out_line) - 1U))
    {
        int prefix_len = 0;
#if JLINK_RTT_LOG_ENABLE_TIMESTAMP && JLINK_RTT_LOG_ENABLE_FILELINE
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%lu][%s][%s:%d] ",
                              static_cast<unsigned long>(HAL_GetTick()), level, _file_name_only(file), line);
#elif JLINK_RTT_LOG_ENABLE_TIMESTAMP
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%lu][%s] ",
                              static_cast<unsigned long>(HAL_GetTick()), level);
#elif JLINK_RTT_LOG_ENABLE_FILELINE
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%s][%s:%d] ",
                              level, _file_name_only(file), line);
#else
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%s] ", level);
#endif

        if (prefix_len > 0)
        {
            uint32_t remain = static_cast<uint32_t>(sizeof(out_line) - used_len);
            uint32_t add_len = static_cast<uint32_t>(prefix_len);
            if (add_len >= remain)
            {
                add_len = remain - 1U;
                truncated = true;
            }
            used_len += add_len;
        }
    }

    _append_cstr(out_line, &used_len, msg, &truncated);
    if (color_reset[0] != '\0')
    {
        _append_cstr(out_line, &used_len, color_reset, &truncated);
    }
    _append_cstr(out_line, &used_len, "\r\n", &truncated);

#if JLINK_RTT_ASYNC_ENABLE
    if (truncated)
    {
        _counter_inc(&s_truncated_cnt);
    }
#endif
    return static_cast<int>(write(out_line, used_len));
}

int JLinkRTT_c::vlog_with_level(const char* level, const char* file, int line, const char* fmt, va_list args)
{
    if (fmt == nullptr)
    {
        return 0;
    }

    if (!_fmt_need_format(fmt))
    {
        return log_string_with_level(level, file, line, fmt);
    }

    char out_line[JLINK_RTT_ASYNC_LINE_MAX] = {0};
    uint32_t used_len = 0;
    bool truncated = false;
    const char* color_code = _level_color(level);

#if JLINK_RTT_LOG_ENABLE_COLOR
    const char* color_reset = JLINK_RTT_LOG_COLOR_RESET;
#else
    const char* color_reset = "";
#endif

    if (color_code[0] != '\0')
    {
        _append_cstr(out_line, &used_len, color_code, &truncated);
    }

    if (used_len < (sizeof(out_line) - 1U))
    {
        int prefix_len = 0;
#if JLINK_RTT_LOG_ENABLE_TIMESTAMP && JLINK_RTT_LOG_ENABLE_FILELINE
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%lu][%s][%s:%d] ",
                              static_cast<unsigned long>(HAL_GetTick()), level, _file_name_only(file), line);
#elif JLINK_RTT_LOG_ENABLE_TIMESTAMP
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%lu][%s] ",
                              static_cast<unsigned long>(HAL_GetTick()), level);
#elif JLINK_RTT_LOG_ENABLE_FILELINE
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%s][%s:%d] ",
                              level, _file_name_only(file), line);
#else
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%s] ", level);
#endif

        if (prefix_len > 0)
        {
            uint32_t remain = static_cast<uint32_t>(sizeof(out_line) - used_len);
            uint32_t add_len = static_cast<uint32_t>(prefix_len);
            if (add_len >= remain)
            {
                add_len = remain - 1U;
                truncated = true;
            }
            used_len += add_len;
        }
    }

    if (used_len < (sizeof(out_line) - 1U))
    {
        int body_len = vsnprintf(out_line + used_len, sizeof(out_line) - used_len, fmt, args);
        if (body_len < 0)
        {
            return body_len;
        }
        if (body_len > 0)
        {
            uint32_t remain = static_cast<uint32_t>(sizeof(out_line) - used_len);
            uint32_t add_len = static_cast<uint32_t>(body_len);
            if (add_len >= remain)
            {
                add_len = remain - 1U;
                truncated = true;
            }
            used_len += add_len;
        }
    }

    if (color_reset[0] != '\0')
    {
        _append_cstr(out_line, &used_len, color_reset, &truncated);
    }
    _append_cstr(out_line, &used_len, "\r\n", &truncated);

#if JLINK_RTT_ASYNC_ENABLE
    if (truncated)
    {
        _counter_inc(&s_truncated_cnt);
    }
#endif
    return static_cast<int>(write(out_line, used_len));
}

int JLinkRTT_c::log_with_level(const char* level, const char* file, int line, const char* fmt, ...)
{
    if (fmt == nullptr)
    {
        return 0;
    }

    va_list args;
    va_start(args, fmt);
    int ret = vlog_with_level(level, file, line, fmt, args);
    va_end(args);
    return ret;
}

int JLinkRTT_c::log_string_with_level_on_terminal(uint8_t terminal_id, const char* level, const char* file, int line,
                                                   const char* msg)
{
    if (!is_inited_)
    {
        init();
    }

    if (msg == nullptr)
    {
        return 0;
    }

    char out_line[JLINK_RTT_ASYNC_LINE_MAX] = {0};
    uint32_t used_len = 0;
    bool truncated = false;
    const char* color_code = _level_color(level);

#if JLINK_RTT_LOG_ENABLE_COLOR
    const char* color_reset = JLINK_RTT_LOG_COLOR_RESET;
#else
    const char* color_reset = "";
#endif

    if (color_code[0] != '\0')
    {
        _append_cstr(out_line, &used_len, color_code, &truncated);
    }

    if (used_len < (sizeof(out_line) - 1U))
    {
        int prefix_len = 0;
#if JLINK_RTT_LOG_ENABLE_TIMESTAMP && JLINK_RTT_LOG_ENABLE_FILELINE
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%lu][%s][%s:%d] ",
                              static_cast<unsigned long>(HAL_GetTick()), level, _file_name_only(file), line);
#elif JLINK_RTT_LOG_ENABLE_TIMESTAMP
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%lu][%s] ",
                              static_cast<unsigned long>(HAL_GetTick()), level);
#elif JLINK_RTT_LOG_ENABLE_FILELINE
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%s][%s:%d] ",
                              level, _file_name_only(file), line);
#else
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%s] ", level);
#endif

        if (prefix_len > 0)
        {
            uint32_t remain = static_cast<uint32_t>(sizeof(out_line) - used_len);
            uint32_t add_len = static_cast<uint32_t>(prefix_len);
            if (add_len >= remain)
            {
                add_len = remain - 1U;
                truncated = true;
            }
            used_len += add_len;
        }
    }

    _append_cstr(out_line, &used_len, msg, &truncated);
    if (color_reset[0] != '\0')
    {
        _append_cstr(out_line, &used_len, color_reset, &truncated);
    }
    _append_cstr(out_line, &used_len, "\r\n", &truncated);

#if JLINK_RTT_ASYNC_ENABLE
    if (truncated)
    {
        _counter_inc(&s_truncated_cnt);
    }
#endif
    return _terminal_out_string(terminal_id, out_line);
}

int JLinkRTT_c::vlog_with_level_on_terminal(uint8_t terminal_id, const char* level, const char* file, int line,
                                             const char* fmt, va_list args)
{
    if (!is_inited_)
    {
        init();
    }

    if (fmt == nullptr)
    {
        return 0;
    }

    if (!_fmt_need_format(fmt))
    {
        return log_string_with_level_on_terminal(terminal_id, level, file, line, fmt);
    }

    char out_line[JLINK_RTT_ASYNC_LINE_MAX] = {0};
    uint32_t used_len = 0;
    bool truncated = false;
    const char* color_code = _level_color(level);

#if JLINK_RTT_LOG_ENABLE_COLOR
    const char* color_reset = JLINK_RTT_LOG_COLOR_RESET;
#else
    const char* color_reset = "";
#endif

    if (color_code[0] != '\0')
    {
        _append_cstr(out_line, &used_len, color_code, &truncated);
    }

    if (used_len < (sizeof(out_line) - 1U))
    {
        int prefix_len = 0;
#if JLINK_RTT_LOG_ENABLE_TIMESTAMP && JLINK_RTT_LOG_ENABLE_FILELINE
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%lu][%s][%s:%d] ",
                              static_cast<unsigned long>(HAL_GetTick()), level, _file_name_only(file), line);
#elif JLINK_RTT_LOG_ENABLE_TIMESTAMP
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%lu][%s] ",
                              static_cast<unsigned long>(HAL_GetTick()), level);
#elif JLINK_RTT_LOG_ENABLE_FILELINE
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%s][%s:%d] ",
                              level, _file_name_only(file), line);
#else
        prefix_len = snprintf(out_line + used_len, sizeof(out_line) - used_len, "[%s] ", level);
#endif

        if (prefix_len > 0)
        {
            uint32_t remain = static_cast<uint32_t>(sizeof(out_line) - used_len);
            uint32_t add_len = static_cast<uint32_t>(prefix_len);
            if (add_len >= remain)
            {
                add_len = remain - 1U;
                truncated = true;
            }
            used_len += add_len;
        }
    }

    if (used_len < (sizeof(out_line) - 1U))
    {
        int body_len = vsnprintf(out_line + used_len, sizeof(out_line) - used_len, fmt, args);
        if (body_len < 0)
        {
            return body_len;
        }
        if (body_len > 0)
        {
            uint32_t remain = static_cast<uint32_t>(sizeof(out_line) - used_len);
            uint32_t add_len = static_cast<uint32_t>(body_len);
            if (add_len >= remain)
            {
                add_len = remain - 1U;
                truncated = true;
            }
            used_len += add_len;
        }
    }

    if (color_reset[0] != '\0')
    {
        _append_cstr(out_line, &used_len, color_reset, &truncated);
    }
    _append_cstr(out_line, &used_len, "\r\n", &truncated);

#if JLINK_RTT_ASYNC_ENABLE
    if (truncated)
    {
        _counter_inc(&s_truncated_cnt);
    }
#endif
    return _terminal_out_string(terminal_id, out_line);
}

int JLinkRTT_c::log_with_level_on_terminal(uint8_t terminal_id, const char* level, const char* file, int line,
                                           const char* fmt, ...)
{
    if (fmt == nullptr)
    {
        return 0;
    }

    va_list args;
    va_start(args, fmt);
    int ret = vlog_with_level_on_terminal(terminal_id, level, file, line, fmt, args);
    va_end(args);
    return ret;
}

uint32_t JLinkRTT_c::process_pending(uint32_t max_lines)
{
    if (!is_inited_)
    {
        init();
    }

#if JLINK_RTT_ASYNC_ENABLE
    if (max_lines == 0)
    {
        return 0;
    }

    uint32_t handled = 0;
    char line[JLINK_RTT_ASYNC_LINE_MAX];
    while (handled < max_lines)
    {
        uint16_t len = _queue_pop(line, sizeof(line));
        if (len == 0)
        {
            break;
        }
        (void)SEGGER_RTT_Write(up_channel_, line, len);
        ++handled;
    }
    return handled;
#else
    (void)max_lines;
    return 0;
#endif
}

uint32_t JLinkRTT_c::dropped_count()
{
#if JLINK_RTT_ASYNC_ENABLE
    return _counter_read(&s_dropped_cnt);
#else
    return 0;
#endif
}

uint32_t JLinkRTT_c::truncated_count()
{
#if JLINK_RTT_ASYNC_ENABLE
    return _counter_read(&s_truncated_cnt);
#else
    return 0;
#endif
}
}

extern "C" void JlinkLogTask(void const* argument)
{
    (void)argument;
    BSP_n::JLinkRTT_c::init();
    while (1)
    {
        (void)BSP_n::JLinkRTT_c::process_pending(JLINK_RTT_LOG_TASK_FLUSH_LINES);
        osDelay(JLINK_RTT_LOG_TASK_DELAY_MS);
    }
}
