# TFmini Plus ToF 模块使用说明

本模块用于北�?TFmini Plus 激�?ToF 测距模块，使�?ACE_ECF_26 �?`BSP_n::USART_c` 串口封装通信�?
默认解析 TFmini Plus �?UART 标准 9 字节数据帧：

```text
59 59 Dist_L Dist_H Strength_L Strength_H Temp_L Temp_H Checksum
```

模块支持多个 ToF：每�?`TFminiPlus_c` 对象绑定一个独�?UART 句柄，互不共享接收缓冲、解析状态和掉线状态�?
## 1. 工程配置

### 1.1 开启模�?
�?CMake 配置中开启：

```bash
-DUSE_TOF=ON
```

也可以把工程根目�?[CMakeLists.txt](../../../CMakeLists.txt) 中的 `TOF` 默认值改�?`ON`�?
```cmake
ADD_MODULE(TOF ON
        ${CMAKE_SOURCE_DIR}/ACE_ECF_26/Bsp/DWT
        ${CMAKE_SOURCE_DIR}/ACE_ECF_26/App/safe_task
        ${CMAKE_SOURCE_DIR}/ACE_ECF_26/Bsp/USART
        ${CMAKE_SOURCE_DIR}/ACE_ECF_26/Module/tof
)
```

### 1.2 CubeMX 串口参数

TFmini Plus 默认 UART 参数�?
```text
BaudRate: 115200
WordLength: 8B
StopBits: 1
Parity: None
Mode: TX_RX
```

模块当前接收方式�?`DMA_IDLE`，所以每个接 ToF 的串口都需要开�?RX DMA，并启用对应 UART 全局中断。当前模块不再使�?`ecf_configList.h` 配置串口号，串口句柄由初始化入参或构造函数入参传入�?
## 2. 初始�?
### 2.1 Init 入参方式

```cpp
#include "tfmini_plus.hpp"
#include "usart.h"

Tof_n::TFminiPlus_c tof_front;

void UserInit()
{
    tof_front.TFminiPlusInit(&huart8, "TOF_F");
}
```

### 2.2 构造函数入参方�?
```cpp
#include "tfmini_plus.hpp"
#include "usart.h"

Tof_n::TFminiPlus_c tof_front(&huart8, "TOF_F");
```

如果 CubeMX �?`MX_USARTx_UART_Init()` 还没有执行，不要在全局构造阶段直接传串口初始化。更稳妥的写法是在系统初始化完成后调�?`TFminiPlusInit()`�?
## 3. 多个 ToF 示例

```cpp
#include "tfmini_plus.hpp"
#include "usart.h"

Tof_n::TFminiPlus_c tof_front;
Tof_n::TFminiPlus_c tof_back;

void UserInit()
{
    tof_front.TFminiPlusInit(&huart8, "TOF_F");
    tof_back.TFminiPlusInit(&huart6, "TOF_B");
}
```

使用时分别读取：

```cpp
if (tof_front.IsOnline() && tof_front.IsDistanceValid())
{
    float front_m = tof_front.GetDistanceM();
    (void)front_m;
}

if (tof_back.IsOnline() && tof_back.IsDistanceValid())
{
    float back_m = tof_back.GetDistanceM();
    (void)back_m;
}
```

不要让两�?`TFminiPlus_c` 对象绑定同一�?UART 句柄。`BSP_n::USART_c` �?UART 句柄分发回调，同一个串口只应注册一个接收对象�?
## 4. 读取接口

常用接口�?
```cpp
uint16_t distance_cm = tof_front.GetDistanceCm();
float distance_m = tof_front.GetDistanceM();
uint16_t strength = tof_front.GetStrength();
float temperature = tof_front.GetTemperatureC();
bool online = tof_front.IsOnline();
bool valid = tof_front.IsDistanceValid();
```

推荐控制逻辑中这样使用：

```cpp
if (tof_front.IsOnline() && tof_front.IsDistanceValid())
{
    float dist_m = tof_front.GetDistanceM();
    // 使用 dist_m 做控制或判断
}
```

`IsDistanceValid()` 的基础判断条件�?
```text
distance != 0
strength > 100
strength != 65535
```

## 5. 数据结构

每个对象的完整数据保存在 `data_`�?
```cpp
Tof_n::TFminiPlusData_t data = tof_front.data_;
```

结构体定义：

```cpp
typedef struct
{
    uint16_t distance_cm;
    uint16_t strength;
    int16_t temp_raw;
    float temperature_c;
    uint8_t frame_valid;
    uint8_t distance_valid;
    float actual_frame_rate_hz;
    uint32_t update_count;
    uint32_t checksum_error_count;
    uint32_t frame_error_count;
} TFminiPlusData_t;
```

| 字段 | 说明 |
| --- | --- |
| `distance_cm` | 距离，单�?cm |
| `strength` | 信号强度 |
| `temp_raw` | 原始温度�?|
| `temperature_c` | 摄氏温度，计算方式为 `Temp / 8 - 256` |
| `frame_valid` | 最近一帧校验是否通过 |
| `distance_valid` | 最近距离值是否可�?|
| `actual_frame_rate_hz` | 实际接收并校验通过的数据帧率，单位 Hz，约�?1s 刷新一�?|
| `update_count` | 成功解析帧计�?|
| `checksum_error_count` | 校验失败计数 |
| `frame_error_count` | 找帧失败计数 |

## 6. 自定义配置命令接�?
所有配置命令都通过对象绑定的串口发送。发送完配置类命令后，如果希望断电后仍然生效，需要调�?`SaveSettings()`�?
| 手册配置�?| 接口 | 下行命令格式 |
| --- | --- | --- |
| 获取固件版本 | `GetFirmwareVersion()` | `5A 04 01 5F` |
| 系统复位 | `SystemReset()` | `5A 04 02 60` |
| 输出帧率 | `SetOutputRate(uint16_t hz)` | `5A 06 03 LL HH SU` |
| 单次触发指令 | `TriggerOnce()` | `5A 04 04 62` |
| 输出模式 | `SetOutputMode(mode)` | `5A 05 05 MODE SU` |
| 波特�?| `SetBaudRate(uint32_t baudrate)` | `5A 08 06 H1 H2 H3 H4 SU` |
| 输出开�?| `SetOutputEnable(bool enable)` | `5A 05 07 00/01 SU` |
| 通信接口设置 | `SetCommunicationInterface(interface)` | `5A 05 0A MODE SU` |
| 修改 I2C 从机地址 | `SetI2CAddress(uint8_t address)` | `5A 05 0B ADDR SU` |
| 获取测距结果 cm | `RequestDistanceCm()` | `5A 05 00 01 60` |
| 获取测距结果 mm | `RequestDistanceMm()` | `5A 05 00 06 65` |
| I/O 模式使能 | `SetIOMode(mode, dist_cm, zone_cm)` | `5A 09 3B MODE DL DH ZoneL ZoneH SU` |
| 信号强度低阈值和低阈值输出�?| `SetStrengthLowThreshold(strength, output_cm)` | `5A 07 22 XX LL HH 00` |
| 低功耗模式使�?| `SetLowPowerMode(uint8_t level)` | `5A 06 35 0X 00 SU` |
| 恢复出厂设置 | `FactoryReset()` | `5A 04 10 6E` |
| 保存设置 | `SaveSettings()` | `5A 04 11 6F` |

### 6.1 获取固件版本

```cpp
tof_front.GetFirmwareVersion();
```

设备会上行版本帧，格式为 `5A 07 01 V1 V2 V3 SU`�?
### 6.2 系统复位

```cpp
tof_front.SystemReset();
```

发送后保持通电并等待约 1s�?
### 6.3 设置输出帧率

```cpp
tof_front.SetOutputRate(100);
tof_front.SaveSettings();
```

手册支持 1-1000Hz。帧率提高时，数据输出稳定性会下降，必要时提高波特率�?
### 6.4 单次触发

```cpp
tof_front.SetOutputRate(0);
tof_front.TriggerOnce();
```

单次触发通常配合输出帧率 0 使用�?
### 6.5 输出模式

```cpp
tof_front.SetOutputMode(Tof_n::TFminiPlusOutputMode_e::StandardCm);
tof_front.SetOutputMode(Tof_n::TFminiPlusOutputMode_e::StringM);
tof_front.SetOutputMode(Tof_n::TFminiPlusOutputMode_e::StandardMm);
```

也提供便捷接口：

```cpp
tof_front.SetOutputModeStandardCm();
tof_front.SetOutputModeStringM();
tof_front.SetOutputModeStandardMm();
```

当前模块默认只解析标�?9 字节二进制帧。切到字符串格式后，读取接口不会继续按字符串解析�?
### 6.6 设置波特�?
```cpp
tof_front.SetBaudRate(256000);
tof_front.SaveSettings();
```

发送修改波特率命令后，需要保持通电；保存配置后重新初始�?MCU 串口到目标波特率�?
### 6.7 输出开�?
```cpp
tof_front.SetOutputEnable(false);
tof_front.SetOutputEnable(true);
```

### 6.8 通信接口

```cpp
tof_front.SetCommunicationInterface(Tof_n::TFminiPlusInterface_e::UART);
tof_front.SetCommunicationInterface(Tof_n::TFminiPlusInterface_e::I2C);
tof_front.SaveSettings();
```

切换�?I2C 后，当前 UART 模块将不能继续接收数据，需要改�?I2C 驱动�?
### 6.9 I2C 地址

```cpp
tof_front.SetI2CAddress(0x10);
tof_front.SaveSettings();
```

默认 I2C 地址�?`0x10`�?
### 6.10 获取测距结果

```cpp
tof_front.RequestDistanceCm();
tof_front.RequestDistanceMm();
```

手册标注该项�?I2C 模式下可用；UART 连续输出模式下一般不用调用�?
### 6.11 I/O 模式

```cpp
tof_front.SetIOMode(Tof_n::TFminiPlusIOMode_e::NearHighFarLow, 120, 10);
tof_front.SaveSettings();
```

参数说明�?
```text
mode: 0 标准数据模式�? 近高远低�? 近低远高
dist_cm: 临界距离，单�?cm
zone_cm: 滞回区间，单�?cm
```

使用 I/O 模式前，手册建议先把低阈值默认输出值设�?1200，避免远距离误报�?
```cpp
tof_front.SetStrengthLowThreshold(100, 1200);
tof_front.SaveSettings();
```

### 6.12 信号强度低阈值和低阈值输出�?
```cpp
tof_front.SetStrengthLowThreshold(100, 1200);
tof_front.SaveSettings();
```

手册�?`XX = strength / 10`，例如阈�?100 对应 `0x0A`；输出值为小端格式，例�?1200cm 对应 `B0 04`。手册示例命令末尾为固定 `00`，本接口按该示例发送�?
### 6.13 低功耗模�?
```cpp
tof_front.SetLowPowerMode(0);  // 关闭低功�?tof_front.SetLowPowerMode(5);  // 开启低功耗，等级 0x1-0xA
tof_front.SaveSettings();
```

低功耗模式下输出频率不支持超�?10Hz。关闭低功耗后，如果仍需�?100Hz 输出，需要重新设置输出帧率�?
### 6.14 恢复出厂设置

```cpp
tof_front.FactoryReset();
tof_front.SaveSettings();
```

### 6.15 保存设置

```cpp
tof_front.SaveSettings();
```

## 7. 自定义原始命�?
如果需要发送手册以外的命令，可使用�?
```cpp
const uint8_t cmd[] = {0x5A, 0x04, 0x01, 0x5F};
tof_front.SendCommand(cmd, sizeof(cmd));
```

通用配置帧格式：

```text
Byte0: Head, 固定 0x5A
Byte1: Len, 指令总长�?Byte2: ID
Byte3~ByteN-2: Payload
ByteN-1: Checksum
```

多数命令�?`Checksum` 为从 `Head` �?`Payload` 所有字节求和后取低 8 位�?
## 8. 注意事项

1. TFmini Plus 供电�?`5V±0.5V`，通信电平�?`LVTTL 3.3V`�?2. 默认距离单位�?cm，本模块�?cm 解析�?3. 当信号强度过低或过曝时，TFmini Plus 可能输出距离 0，此�?`IsDistanceValid()` 返回 false�?4. 每个 ToF 对象需要独占一�?UART�?5. 每个�?ToF �?UART 都需�?RX DMA；如果使�?`huart8`，请先确�?CubeMX 已为 `UART8_RX` 配置 DMA�?6. `TFminiPlus_c` 对象应为全局、静态或生命周期长于串口回调的对象，不要在局部函数里创建临时对象后立刻离开作用域�?7. 多个 TFmini Plus 同向照射且光斑重合时可能互相干扰，安装时应避开对射或同点重叠�?
## 9. 最小使用示�?
```cpp
#include "tfmini_plus.hpp"
#include "usart.h"

Tof_n::TFminiPlus_c tof_front;

void UserInit()
{
    tof_front.TFminiPlusInit(&huart8, "TOF_F");
}

void UserTask()
{
    if (tof_front.IsOnline() && tof_front.IsDistanceValid())
    {
        uint16_t dist_cm = tof_front.GetDistanceCm();
        float dist_m = tof_front.GetDistanceM();
        uint16_t strength = tof_front.GetStrength();

        (void)dist_cm;
        (void)dist_m;
        (void)strength;
    }
}
```
