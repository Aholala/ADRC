# DR16 UART5 DMA 双缓冲故障记录

## 1. 问题背景

本工程使用 STM32H723VE 的 UART5 接收 DR16/DT7 遥控器数据，接收方式为 DMA + UART IDLE + 双缓冲。每帧长度为 18 字节，两块 DMA 缓冲区分别保存一帧。

UART5 配置如下：

| 项目 | 配置 |
| --- | --- |
| 波特率 | 100000 |
| 数据位 | 9 Bits |
| 校验 | Even parity |
| 停止位 | 1 Stop Bit |
| RX 引脚 | PD2 |
| DMA | DMA1 Stream2，Circular |
| 单块缓冲区长度 | 18 字节 |

目标状态为：

- 遥控器在线时 `onlineflag == 1`；
- 摇杆静止时 `ch[0..4] == 0`；
- 保留 DMA 双缓冲，不改成单缓冲；
- 保持正常任务顺序，不通过提前初始化 DR16 或关闭 IMU 规避问题。

## 2. 故障现象

调试过程中先后出现了两类现象。

### 2.1 online 为 1，但通道全部为 -1024

J-Link 在 `DT7callback()` 入口捕获到：

```text
Size = 18
接收缓冲区 = 全 0
```

全零帧经过 DR16 解析后，每个摇杆通道都会减去中值 1024，因此得到：

```text
ch[0..4] = -1024
```

这说明 `onlineflag == 1` 并不一定代表收到了真实遥控器帧，也可能是错误地解析了全零缓冲区。

### 2.2 正常任务运行时所有数据和 online 都为 0

关闭 IMU 或提前执行 `RoboCmdInit()` 时，DR16 可以正常接收；恢复正常任务后，DR16 对象和 DMA 最终能够初始化，但出现：

```text
DMA NDTR = 18
M0/M1 缓冲区 = 全 0
onlineflag = 0
```

UART5 状态寄存器同时存在 `FE` 和 `ORE`。PD2 输入电平在变化，说明遥控器信号已经到达 MCU 引脚，但 UART5 没有继续产生有效 DMA 接收请求。

## 3. 根因分析

本问题由两个独立缺陷叠加造成。

### 3.1 完整帧后的伪 IDLE 被当成新帧解析

旧双缓冲实现存在以下问题：

1. 使用 `HAL_DMAEx_MultiBufferStart()`，没有启用 DMA TC 中断；
2. DMA 每块传输长度错误地使用了两块缓冲区的总长度 36，而不是单块长度 18；
3. UART IDLE 回调中手动停止 DMA、修改 CT 位并解析缓冲区；
4. Circular DMA 完成一块传输后，`NDTR` 自动重新装载为18；
5. HAL UART IDLE 逻辑在 `NDTR == RxXferSize` 时仍可能上报一次 `Size == 18` 的 IDLE 事件。

因此，真实帧完成后的 IDLE 被误认为另一帧数据，全零缓冲区进入 `DT7callback()`，覆盖了之前的正确结果。

### 3.2 DR16 延迟初始化前积累了 UART 历史错误

正常任务中，控制任务需要先等待 IMU 初始化完成，然后才执行 `RoboCmdInit()`。在这段等待时间内：

1. CubeMX 已经初始化并使能 UART5 接收器；
2. 遥控器持续向 PD2 发送数据；
3. 此时 DR16 DMA 尚未启动，RDR 中的数据无人读取；
4. UART5 逐渐积累 `FE`、`NE` 或 `ORE`；
5. 自定义双缓冲启动函数直接启动 DMA，没有清除旧错误和刷新 RDR；
6. DMA 虽然显示 EN、DBM 和 DMAR 已使能，但 `NDTR` 始终保持18，收不到有效帧。

提前初始化 DR16 能成功，是因为 DMA 在 UART 错误积累前就启动了。这只能作为诊断对照，不能作为最终解决方案。

## 4. 最终解决方法

### 4.1 每块 DMA 长度固定为18字节

DR16 仍提供两块18字节缓冲区，总大小配置为36：

```cpp
.RxBuffSize = DT7_FRAME_LENGTH * 2,
.RxType = BSP_n::USART_c::RxType_t::DMA_IDLE_DOUBLE,
.pRxBuffer = sbus_rx_buffer_[0],
.pSecRxBuffer = sbus_rx_buffer_[1],
```

USART 驱动启动 DMA 时必须使用单块长度：

```cpp
const uint16_t single_buffer_size = get_single_buffer_size(); // 18

HAL_DMAEx_MultiBufferStart_IT(handle->hdmarx,
                              get_data_register_address(),
                              reinterpret_cast<uint32_t>(rx_buffer),
                              reinterpret_cast<uint32_t>(sec_rx_buffer),
                              single_buffer_size);
```

### 4.2 启动双缓冲前清理 UART 接收状态

在启动 DMA 之前关闭 DMAR、清除历史错误并刷新 RDR：

```cpp
CLEAR_BIT(handle->Instance->CR3, USART_CR3_DMAR);

__HAL_UART_CLEAR_FLAG(handle,
                      UART_CLEAR_PEF |
                      UART_CLEAR_FEF |
                      UART_CLEAR_NEF |
                      UART_CLEAR_OREF |
                      UART_CLEAR_IDLEF);

__HAL_UART_SEND_REQ(handle, UART_RXDATA_FLUSH_REQUEST);
```

DMA 启动成功后，再恢复 UART 的校验错误、普通错误、DMA 接收和 IDLE 中断：

```cpp
if (handle->Init.Parity != UART_PARITY_NONE)
{
    ATOMIC_SET_BIT(handle->Instance->CR1, USART_CR1_PEIE);
}

ATOMIC_SET_BIT(handle->Instance->CR3, USART_CR3_EIE | USART_CR3_DMAR);
__HAL_UART_ENABLE_IT(handle, UART_IT_IDLE);
```

这样无论 DR16 在上电后立即初始化，还是等待 IMU 数秒后再初始化，DMA 都从干净的 UART 接收状态开始工作。

### 4.3 完整帧只在 DMA TC 回调中解析

使用 `HAL_DMAEx_MultiBufferStart_IT()`。DMA中断由框架的RAM向量表统一注册：

```cpp
DmaIrqRouter::Register(handle->hdmarx);
```

路由器会将当前向量表复制到RAM，通过统一入口调用
`HAL_DMA_IRQHandler(hdma)`，因此无需修改CubeMX生成的
`DMA1_Stream2_IRQHandler()`，重新生成Cube代码也不会破坏DMA回调链路。
USART框架的 `DMA_IDLE`、`DMA_NUM`、`DMA_IDLE_DOUBLE` 和 DMA发送模式
都会自动注册对应的DMA Handle，上层模块不需要关心具体DMA Stream及IRQ名称。

M0 和 M1 的完成回调统一进入同一个处理函数。DMA TC 发生后，硬件 CT 位已经指向当前正在接收的缓冲区，因此另一块缓冲区才是刚完成的缓冲区：

```cpp
const bool memory1_is_active =
    ((((DMA_Stream_TypeDef *)handle->hdmarx->Instance)->CR & DMA_SxCR_CT) != RESET);

uint8_t *completed_buffer = memory1_is_active ? rx_buffer : sec_rx_buffer;
rx_callback(completed_buffer, 18);
```

不要仅根据 HAL 的 `XferCpltCallback` 和 `XferM1CpltCallback` 名称判断完成缓冲区，应以硬件 CT 位为准。

### 4.4 IDLE 不再解析完整帧

完整的18字节帧只由 DMA TC 回调处理。UART IDLE 回调规则如下：

- `Size == 18`：完整帧后的伪 IDLE，直接忽略；
- `Size == 0`：没有新增数据，直接忽略；
- `0 < Size < 18`：残帧，清除当前活动缓冲并重装 `NDTR=18`，用于下一帧重新对齐；
- 残帧不得调用 DR16 解析，也不得置 online。

## 5. J-Link 验收结果

在未关闭 IMU、未提前执行 `RoboCmdInit()` 的正常任务流程下，等待 IMU 初始化完成后读取到：

```text
DMA DBM = 1
M0/M1 = 两个独立的18字节缓冲区
UART FE/NE/ORE = 0
onlineflag = 1
ch[0] = 0
ch[1] = 0
ch[2] = 0
ch[3] = 0
ch[4] = 0
s1 = 2
s2 = 2
```

接收缓冲区中的静止遥控器帧头为：

```text
00 04 20 00 01 A8 ...
```

连续两次采样中 `NDTR` 在帧接收期间变化，并在下一块缓冲开始时重新装载，说明 DMA 双缓冲持续工作正常。

## 6. 调试注意事项

1. `onlineflag == 1` 只能说明合法长度的帧触发了在线逻辑，还必须同时检查原始帧和解析通道。
2. 全零原始帧会被解析为 `ch == -1024`，这是判断伪回调的重要特征。
3. `NDTR == 18` 可能表示当前块刚重新装载，不能单独据此判断 DMA 未工作；应连续采样 CT、NDTR 和缓冲区。
4. J-Link 长时间暂停 CPU 时，UART 外设仍可能继续接收数据，从而人为制造 ORE。读取状态时应尽量缩短暂停时间。
5. DMA 缓冲区地址和 DR16 实例地址会随固件链接结果变化，调试前应使用 ELF 符号重新确认，不要长期使用固定地址。
6. 本次故障不是 D-Cache 一致性问题；实测 Cortex-M7 D-Cache 未启用。
7. 不要通过删除双缓冲、关闭 IMU任务或改变正常任务初始化顺序作为最终修复。

## 7. 相关文件

- `ACE_ECF_26/Bsp/USART/bsp_usart.hpp`
- `ACE_ECF_26/Bsp/USART/bsp_usart.cpp`
- `ACE_ECF_26/Module/dr16/dr16.cpp`
- `Core/Src/stm32h7xx_it.c`
- `Core/Src/usart.c`
- `ACE26_H7.ioc`
