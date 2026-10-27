# power_ctrl_noRLS.cpp 使用说明

## 1. 功能概述
`power_ctrl_noRLS.cpp` 用于底盘功率限制与扭矩输出修正（不使用RLS）。  
输入各电机的目标扭矩与实时转速，结合功率模型与裁判系统功率上限，输出经过功率限制后的扭矩。

---

## 2. 前置准备
### 2.1 功率模型
每个电机需要提供功率模型系数：  
`P = k0 + k1*T + k2*w + k3*T*w + k4*T^2 + k5*w^2`

单位要求：
- 扭矩 `T`：牛·米  
- 转速 `w`：弧度/秒

k值由拟合来而来，需使用matlab，matlab文件放在了文件夹内，文件例子M3508.csv
此处给出笔者模拟出的K值可先使用试试

3508电机
k0 = 3.9989624914，
k1 = 19.1773325768, 
k2 = -0.000410903760641, 
k3 = 1.190481312524, 
k4 = 367.716261875, 
k5 = 0.000072887900709,

6220电机
k0 = 0.46242430693, 
k1 = -0.162637535733, 
k2 = 0.00054054735969, 
k3 = 1.12288256112, 
k4 = -8.97381891926, 
k5 = 0.00215630780016,

### 2.2 Motors 结构体用法
`Motors` 用来封装电机指针与 K 系数：

```cpp
// 例：4个轮电机
std::vector<const Motor_n::DjiMotor_n::DjiDriver_c*> motor_list = {
    wheel0, wheel1, wheel2, wheel3
};

std::vector<std::array<float,6>> k_list = {
    k0, k1, k2, k3
};

Motors motors(motor_list, k_list);
```

也可以逐个设置：
```cpp
Motors motors;
motors.Set(0, wheel0, k0);
motors.Set(1, wheel1, k1);
motors.Set(2, wheel2, k2);
motors.Set(3, wheel3, k3);
```

---

## 3. 初始化
```cpp
PowerCtrl_c* power_ctrl = PowerCtrl_c::GetInstance();
power_ctrl->Init(motors);
```

---

## 4. 调用方式
核心计算函数 `Calc`：

```cpp
std::vector<float*> torque_cmd;   // 每个电机目标扭矩指针(牛·米)
std::vector<float*> speed_rad_s;  // 每个电机实时转速指针(弧度/秒)

float* limited_torque = power_ctrl->Calc(torque_cmd, speed_rad_s);
```

### 输入说明
- `torque_cmd[i]`：第 i 个电机的目标扭矩
- `speed_rad_s[i]`：第 i 个电机的转速（必须是 rad/s）

### 输出说明
返回值 `limited_torque[i]` 为经过功率限制后的最终扭矩。

---

## 5. 策略配置
在 `power_ctrl_noRLS.hpp` 中：

```c
#define CHASSIS_TYPE EIGHT

// #define POWER_CTRL_USE_TORQUE_DECAY
```

### CHASSIS_TYPE == FOUR
- **开宏 `POWER_CTRL_USE_TORQUE_DECAY`**：扭矩衰减  
- **不开宏**：功率衰减  

### CHASSIS_TYPE == EIGHT
- **开宏**：轮电机使用扭矩衰减  
- **不开宏**：轮电机使用功率衰减  
- 舵电机始终使用功率衰减  

---

## 6. 注意事项
- EIGHT 默认索引：0~3 为轮电机，4~7 为舵电机（如索引不一致需调整）
- `torque_out` 为静态数组指针，多线程使用需注意并发安全
