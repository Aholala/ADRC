## 快速开始

```cpp
#include "vesc_driver.hpp"

// 创建 VESC 实例：CAN 句柄 + 控制器 ID
Motor_n::Vesc_c vesc1(&hcan1, 0);

// 读取反馈数据
const auto &data = vesc1.data();
float erpm    = data.erpm;           // 电气转速, erpm
float current = data.current * 0.1f; // 电流, A (原始值×0.1)
float duty    = data.duty * 0.001f;  // 占空比 (原始值×0.001)
bool  online  = vesc1.is_online();   // 在线状态

// 发送控制指令
vesc1.Set_Duty(0.5f);           // 50% 占空比
vesc1.Set_Current(10.0f);       // 10A 电流
vesc1.Set_ERPM(21000.0f);       // 21000erpm 转速
vesc1.Set_Pos(90.0f);           // 90° 位置
vesc1.Set_Current_Brake(5.0f);  // 5A 刹车电流
vesc1.Set_HandBrake(3.0f);      // 3A 手刹电流
```

## 控制指令

| 指令                  | 发送数据           | 说明                 |
| --------------------- | ------------------ | -------------------- |
| `Set_Duty`            | duty × 100000      | 占空比 (0.0~1.0)     |
| `Set_Current`         | current × 1000     | 电流控制 (A)         |
| `Set_Current_Brake`   | current × 1000     | 刹车电流 (A)         |
| `Set_ERPM`            | erpm × 1           | 转速控制 (erpm)      |
| `Set_Pos`             | pos × 1000000      | 位置控制 (°)         |
| `Set_Current_Rel`     | current_rel × 1e5  | 电流相对值 (0.0~1.0) |
| `Set_Current_Brake_Rel` | current_rel × 1e5 | 刹车相对值 (0.0~1.0) |
| `Set_HandBrake`       | current × 1000     | 手刹电流 (A)         |
| `Set_HandBrake_Rel`   | current_rel × 1e5  | 手刹相对值 (0.0~1.0) |
| `Set_Current_OffDelay` / `Set_Current_Rel_OffDelay` | 同上 + off_delay | 带超时关断的电流控制 |

## 反馈数据

反馈数据需要在 **vesc_tool** 中开启对应状态的 CAN 发送功能。驱动端通过 `rx_enable` 位掩码按需订阅：

| 状态包    | 包含数据                                                       | 启用位   |
| --------- | -------------------------------------------------------------- | -------- |
| STATUS_1  | erpm、current、duty                                            | bit0     |
| STATUS_2  | amp_hours、amp_hours_charged                                   | bit1     |
| STATUS_3  | watt_hours、watt_hours_charged                                 | bit2     |
| STATUS_4  | temp_fet、temp_motor、current_in、pid_pos_now                  | bit3     |
| STATUS_5  | tacho_value、v_in                                              | bit4     |
| STATUS_6  | adc1、adc2、adc3、ppm                                          | bit5     |

### 数据精度

| 字段               | 原始缩放     | 实际单位 |
| ------------------ | ------------ | -------- |
| `erpm`             | ×1           | erpm     |
| `current`          | ×0.1         | A        |
| `duty`             | ×0.001       | —        |
| `amp_hours`        | ×0.0001      | Ah       |
| `watt_hours`       | ×0.0001      | Wh       |
| `temp_fet/motor`   | ×0.1         | ℃        |
| `current_in`       | ×0.1         | A        |
| `pid_pos_now`      | ×0.02        | °        |
| `v_in`             | ×0.1         | V        |

## 构造函数

```cpp
Vesc_c(CAN_HandleTypeDef *use_can,    // CAN 句柄 (&hcan1 / &hcan2)
       uint8_t driver_id = 0,          // VESC 控制器 ID (0~255)
       uint8_t rx_enable = RX_ENABLE_ALL, // 启用的状态接收掩码
       uint64_t disconnect_ms = 500);  // 掉线超时 (ms)
```
