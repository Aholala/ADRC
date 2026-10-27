extern "C" {
#include "main.h"
}

#ifndef __BSP_JLINK_RTT_HPP
#define __BSP_JLINK_RTT_HPP

#include <cstdarg>
#include <cstdint>

extern "C" {
#include "SEGGER_RTT.h"
}

namespace BSP_n
{
class JLinkRTT_c
{
public:
    struct Config_t
    {
        uint8_t up_channel = 0;
        uint8_t down_channel = 0;
        unsigned up_mode = SEGGER_RTT_MODE_NO_BLOCK_SKIP;
        unsigned down_mode = SEGGER_RTT_MODE_NO_BLOCK_SKIP;
    };

    static void init();
    static void init(const Config_t& config);
    static uint32_t write(const void* data, uint32_t len);
    static uint32_t write_string(const char* str);
    static uint32_t read(void* data, uint32_t len);
    static int vprintf(const char* fmt, va_list args);
    static int printf(const char* fmt, ...);
    static int terminal_vprintf(uint8_t terminal_id, const char* fmt, va_list args);
    static int terminal_printf(uint8_t terminal_id, const char* fmt, ...);
    static int log_string_with_level(const char* level, const char* file, int line, const char* msg);
    static int vlog_with_level(const char* level, const char* file, int line, const char* fmt, va_list args);
    static int log_with_level(const char* level, const char* file, int line, const char* fmt, ...);
    static int log_string_with_level_on_terminal(uint8_t terminal_id, const char* level, const char* file, int line,
                                                 const char* msg);
    static int vlog_with_level_on_terminal(uint8_t terminal_id, const char* level, const char* file, int line,
                                           const char* fmt, va_list args);
    static int log_with_level_on_terminal(uint8_t terminal_id, const char* level, const char* file, int line,
                                          const char* fmt, ...);
    static uint32_t process_pending(uint32_t max_lines = 16);
    static uint32_t dropped_count();
    static uint32_t truncated_count();
    static bool is_inited() { return is_inited_; }

private:
    static bool is_inited_;
    static uint8_t up_channel_;
    static uint8_t down_channel_;
};
}

#ifndef JLINK_RTT_LOG_ENABLE_TIMESTAMP
#define JLINK_RTT_LOG_ENABLE_TIMESTAMP 1
#endif

#ifndef JLINK_RTT_LOG_ENABLE_FILELINE
#define JLINK_RTT_LOG_ENABLE_FILELINE 1
#endif

#ifndef JLINK_RTT_LOG_ENABLE_COLOR
#define JLINK_RTT_LOG_ENABLE_COLOR 1
#endif

#ifndef JLINK_RTT_LOG_COLOR_INFO
#define JLINK_RTT_LOG_COLOR_INFO "\x1b[32m"
#endif

#ifndef JLINK_RTT_LOG_COLOR_WARN
#define JLINK_RTT_LOG_COLOR_WARN "\x1b[33m"
#endif

#ifndef JLINK_RTT_LOG_COLOR_ERROR
#define JLINK_RTT_LOG_COLOR_ERROR "\x1b[31m"
#endif

#ifndef JLINK_RTT_LOG_COLOR_RESET
#define JLINK_RTT_LOG_COLOR_RESET "\x1b[0m"
#endif

#ifndef JLINK_RTT_ASYNC_ENABLE
#define JLINK_RTT_ASYNC_ENABLE 1
#endif

#ifndef JLINK_RTT_ASYNC_QUEUE_DEPTH
#define JLINK_RTT_ASYNC_QUEUE_DEPTH 64
#endif

#ifndef JLINK_RTT_ASYNC_LINE_MAX
#define JLINK_RTT_ASYNC_LINE_MAX 192
#endif

#ifndef JLINK_RTT_LOG_TASK_FLUSH_LINES
#define JLINK_RTT_LOG_TASK_FLUSH_LINES 32
#endif

#ifndef JLINK_RTT_LOG_TASK_DELAY_MS
#define JLINK_RTT_LOG_TASK_DELAY_MS 1
#endif

#define JLINK_RTT_LOG(fmt, ...) (void)BSP_n::JLinkRTT_c::printf((fmt) __VA_OPT__(,) __VA_ARGS__)
#define JLINK_RTT_WRITE_STR(str) (void)BSP_n::JLinkRTT_c::write_string((str))
#define LOG_RTT_I(fmt, ...) (void)BSP_n::JLinkRTT_c::log_with_level("I", __FILE__, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__)
#define LOG_RTT_W(fmt, ...) (void)BSP_n::JLinkRTT_c::log_with_level("W", __FILE__, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__)
#define LOG_RTT_E(fmt, ...) (void)BSP_n::JLinkRTT_c::log_with_level("E", __FILE__, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__)
#define LOG_RTT_I_T(terminal, fmt, ...)                                                                                     \
    (void)BSP_n::JLinkRTT_c::log_with_level_on_terminal((terminal), "I", __FILE__, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__)
#define LOG_RTT_W_T(terminal, fmt, ...)                                                                                     \
    (void)BSP_n::JLinkRTT_c::log_with_level_on_terminal((terminal), "W", __FILE__, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__)
#define LOG_RTT_E_T(terminal, fmt, ...)                                                                                     \
    (void)BSP_n::JLinkRTT_c::log_with_level_on_terminal((terminal), "E", __FILE__, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__)
#define LOG_RTT_I_S(str) (void)BSP_n::JLinkRTT_c::log_string_with_level("I", __FILE__, __LINE__, (str))
#define LOG_RTT_W_S(str) (void)BSP_n::JLinkRTT_c::log_string_with_level("W", __FILE__, __LINE__, (str))
#define LOG_RTT_E_S(str) (void)BSP_n::JLinkRTT_c::log_string_with_level("E", __FILE__, __LINE__, (str))

#ifdef __cplusplus
extern "C" {
#endif
void JlinkLogTask(void const* argument);
#ifdef __cplusplus
}
#endif

#endif
