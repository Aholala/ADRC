# JLINK_RTT 模块说明

## 1. 模块简介
`JLINK_RTT` 是 ACE_ECF_26 的 Bsp 层日志模块，基于 SEGGER RTT 提供实时调试输出能力。

本模块目标：
- 轻量接入，不改动原有串口日志链路。
- 默认非阻塞（`NO_BLOCK_SKIP`），避免日志阻塞控制任务。
- 默认异步入队，建议由独立任务统一刷出 RTT。
- 提供独立 API 与一组易用宏（含 `LOG_RTT_I/W/E`）。

已实现：
- RTT 初始化与通道配置（Up/Down Channel）。
- 非阻塞写入、字符串写入、下行读取。
- 异步日志队列（默认启用）与任务侧 `process_pending()` 刷新接口。
- `printf` 风格输出。
- 级别日志宏：`LOG_RTT_I`、`LOG_RTT_W`、`LOG_RTT_E`。
- 日志前缀可选时间戳、文件名/行号。

预留/未落地：
- 与系统统一日志框架自动桥接（当前为独立接口）。
- 多通道分级策略（当前默认通道 0）。

## 2. 文件结构
- `bsp_jlink_rtt.hpp`：对外 API 与日志宏。
- `bsp_jlink_rtt.cpp`：模块实现。
- `SEGGER_RTT.c/.h`：RTT 核心实现。
- `SEGGER_RTT_Conf.h`：RTT 缓冲区与锁配置。
- `SEGGER.h`：SEGGER 公共头。

## 3. 5 分钟上手
### 3.1 编译开关
工程 `CMakeLists.txt` 中默认已启用：

```cmake
ADD_MODULE(JLINK_RTT ON
        ${CMAKE_SOURCE_DIR}/ACE_ECF_26/Bsp/JLINK_RTT
)
```

会自动定义：`USE_JLINK_RTT=1`。

### 3.2 代码包含
在业务文件中包含：

```cpp
#include "bsp_jlink_rtt.hpp"
```

### 3.3 最小示例
```cpp
void DebugTask()
{
    // 任务中每次循环都把队列里的日志刷到 RTT（建议 1~5ms 周期）
    BSP_n::JLinkRTT_c::process_pending(32);

    LOG_RTT_I("debug task start");
    LOG_RTT_W("battery low: %.2fV", 22.8f);
    LOG_RTT_E("motor offline id=%d", 3);

    JLINK_RTT_LOG("raw log value=%d\r\n", 42);
    JLINK_RTT_WRITE_STR("hello rtt\r\n");
}

// 也可以直接在 CubeMX 任务入口填这个函数名
// void JlinkLogTask(void const* argument);
```

### 3.4 运行成功判断
- J-Link RTT Viewer 或 Ozone RTT 窗口可持续看到日志输出。
- 控制循环无明显卡顿（日志大量输出时应为“丢日志而非阻塞”）。

## 4. Public API
命名空间：`BSP_n`

### 4.1 初始化
```cpp
static void JLinkRTT_c::init();
static void JLinkRTT_c::init(const Config_t& config);
```

`Config_t` 字段：
- `up_channel`：上行通道号，默认 `0`。
- `down_channel`：下行通道号，默认 `0`。
- `up_mode`：上行模式，默认 `SEGGER_RTT_MODE_NO_BLOCK_SKIP`。
- `down_mode`：下行模式，默认 `SEGGER_RTT_MODE_NO_BLOCK_SKIP`。

调用时机：
- 可在系统初始化阶段主动调用。
- 若未主动调用，首次写/读会自动初始化。

### 4.2 读写接口
```cpp
static uint32_t JLinkRTT_c::write(const void* data, uint32_t len);
static uint32_t JLinkRTT_c::write_string(const char* str);
static uint32_t JLinkRTT_c::read(void* data, uint32_t len);
static uint32_t JLinkRTT_c::process_pending(uint32_t max_lines = 16);
static uint32_t JLinkRTT_c::dropped_count();
static uint32_t JLinkRTT_c::truncated_count();
```

输入输出：
- 输入为空或长度为 0 时返回 0。
- 返回值为实际写入/读取字节数。

### 4.3 格式化输出
```cpp
static int JLinkRTT_c::printf(const char* fmt, ...);
```

说明：
- 内部单条日志缓冲默认 `192` 字节（`JLINK_RTT_ASYNC_LINE_MAX`），超长会截断。
- 默认为异步入队，实际 RTT 发送在 `process_pending()` 中完成。

## 5. 日志宏
### 5.1 基础宏
- `JLINK_RTT_LOG(fmt, ...)`
- `JLINK_RTT_WRITE_STR(str)`

### 5.2 级别宏
- `LOG_RTT_I(fmt, ...)`
- `LOG_RTT_W(fmt, ...)`
- `LOG_RTT_E(fmt, ...)`
- `LOG_RTT_I_S(str)`（纯字符串，无 `vsnprintf`）
- `LOG_RTT_W_S(str)`（纯字符串，无 `vsnprintf`）
- `LOG_RTT_E_S(str)`（纯字符串，无 `vsnprintf`）

默认格式（示例）：
- `[12345][I][chassis.cpp:88] speed=2.10`

其中时间戳来自 `HAL_GetTick()`，单位 ms。

## 6. 可选配置
在包含 `bsp_jlink_rtt.hpp` 之前定义以下宏，可控制日志前缀：

```cpp
#define JLINK_RTT_LOG_ENABLE_TIMESTAMP 1
#define JLINK_RTT_LOG_ENABLE_FILELINE  1
#include "bsp_jlink_rtt.hpp"
```

可选值：`0/1`。

异步相关宏（默认值）：

```cpp
#define JLINK_RTT_ASYNC_ENABLE      1
#define JLINK_RTT_ASYNC_QUEUE_DEPTH 64
#define JLINK_RTT_ASYNC_LINE_MAX    192
#define JLINK_RTT_LOG_TASK_FLUSH_LINES 32
#define JLINK_RTT_LOG_TASK_DELAY_MS    1
```

RTT 缓冲参数可在 `SEGGER_RTT_Conf.h` 调整：
- `BUFFER_SIZE_UP`（默认 1024）
- `BUFFER_SIZE_DOWN`（默认 16）

## 7. 设计说明与权衡（WHY）
- 选择默认非阻塞：实时控制系统优先保证控制链路，允许在高负载时丢日志。
- 保持独立接口：避免影响现有 UART/Flash 日志功能，降低迁移风险。
- 自动初始化：降低接入成本，减少“忘记 init”导致的无日志问题。

Trade-off：
- 非阻塞模式在日志突发时会丢消息，不保证日志完整性。
- 异步模式需要周期调用 `process_pending()`，否则日志只会堆积在队列中。
- `printf` 走本地格式化缓冲，单条超长日志会被截断。

## 8. 常见问题与排查
### 问题 1：RTT Viewer 无输出
现象：程序运行正常，但 RTT 窗口没有日志。

排查步骤：
- 确认下载方式为 J-Link，且 RTT Viewer 已连接目标。
- 确认编译包含 `ACE_ECF_26/Bsp/JLINK_RTT`（检查 `USE_JLINK_RTT`）。
- 在代码中强制调用一次 `BSP_n::JLinkRTT_c::init();` 再输出。
- 确认有任务周期调用 `BSP_n::JLinkRTT_c::process_pending(...)`。
- 检查是否频繁 reset 导致观察窗口不断重连。

关键变量/点：
- `BSP_n::JLinkRTT_c::is_inited()`
- `up_channel/down_channel` 配置

### 问题 2：日志不完整或偶发缺行
现象：连续高频打印时丢失部分日志。

原因：默认 `NO_BLOCK_SKIP`，缓冲区满时跳过写入。
异步模式下还可能是软件队列满，调用点会直接丢日志。

排查与优化：
- 降低日志频率。
- 增大 `BUFFER_SIZE_UP`。
- 增大 `JLINK_RTT_ASYNC_QUEUE_DEPTH`。
- 提高 `process_pending()` 调用频率或每次处理条数。
- 将高频路径日志改为采样输出。

### 问题 3：日志前缀太长影响可读性
排查与优化：
- 关闭 `JLINK_RTT_LOG_ENABLE_FILELINE` 或 `JLINK_RTT_LOG_ENABLE_TIMESTAMP`。

## 9. 推荐使用规范
- 控制环高频路径只保留必要 `W/E` 日志。
- `I` 级日志建议用于状态切换、初始化结果、关键参数快照。
- 异常日志优先包含对象 ID、状态码、关键测量值。
