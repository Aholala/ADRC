# DJI 电机一步一步上手

## 必备操作
1. 先 #include "dji_driver.hpp" 并准备一个 Motor_n::DjiMotor_n::DjiDriver_c *motor 指针。
2. 进入初始化前先 __disable_irq();，避免还没配置完就被中断访问。

## 步骤 1：拼好配置
照着下面的模板把电机名字、类型、PID 以及 CAN 等参数填好：
```cpp
namespace MM = Motor_n::MotorBaseDef_n;
MM::Motor_Base_Config_t cfg = MM::Motor_Base_Config_t("WheelMotor", MM::Motor_Type_euc::M3508)
    .SetControlSetting(MM::Motor_Control_Setting_t{MM::Closeloop_Type_euc::ANGLE_AND_SPEED_LOOP})
    .SetPIDConfig(
        alg_n::PidInitConfig_t{
        .Kp = 0.1f,
        .Ki = 0.0f,
        .Kd = 0.0f,
        .mode = Output_Limit | Integral_Limit | StepIn,
        .max_out = 8192,
        .max_Ierror = 220000,
        .deadband = 0,
        .stepIn = 10},/* Angle PID */
        alg_n::PidInitConfig_t{          
        .Kp = 1.0f,
        .Ki = 0.0f,
        .Kd = 0.0f,
        .mode = Output_Limit | Integral_Limit,
        .max_out = Current_limit_H_3508,
        .max_Ierror = 200,
        .deadband = 0,
        .errorabsmax = 0,
        .errorabsmin = 0},/* Speed PID */
        alg_n::PidInitConfig_t{        
        .Kp = 0.0f,
        .Ki = 0.0f,
        .Kd = 0.0f,
        .Kfa = 0.0f,
        .Kfb = 0.0f,
        .ActualValueSource = nullptr})/* Current PID */
    .SetCANConfig(BSP_n::CanInitConfig_s{.can_handle = &hcan1, .tx_id = 3})
    .SetMechanicalParams(MM::Motor_Data_t::Motor_Fixed_Param_t{.zero_offset = 0, .radius = 0.05f, .ratio = 1.0f})
    .SetOutputLimit(Current_limit_H_3508, Current_limit_L_3508);
```
提示：三个 PID 结构体都要填，且顺序一定是角度→速度→电流。

## 步骤 2：实例化并绑定
```cpp
motor = new Motor_n::DjiMotor_n::DjiDriver_c(cfg);
auto *motorbase = &motor->get_base();
```
构造函数会自动完成分组、注册回调以及零偏清理，因此不用自己再操作。但是还是列出来
```cpp
motor->motor_controller_.SetSpeedFeedbackPtr(&motor->motor_data_.motor_raw_data.feedback_speed);
motor->motor_controller_.SetAngleFeedbackPtr(&motor->motor_data_.motor_raw_data.total_ecd);//记得放在构造函数之后或者构造函数里面
```

## 步骤 3：恢复中断
配置完调用 __enable_irq();，此时 CAN 中断就能正常把反馈分发给电机对象。

## 步骤 4：主循环模板
```cpp
while (1)
{
    motor->Enable();                     // 确保处于启用状态
    motor->DjiMotorSetRef(1.0f);         // 根据闭环类型写角度/速度/电流
    Motor_n::DjiMotor_n::DjiMotorControl(); // 逐个算 PID
    Motor_n::DjiMotor_n::set_GiveCurrent(); // 把 PID 输出写进分组缓冲
    Motor_n::DjiMotor_n::MotorTransmit();   // 统一发送
}
```
只要把 DjiMotorSetRef 的值改成你想要的设定即可。

## 温馨提示
1. **切换闭环**：motor->DjiMotorSetLoop(MM::Closeloop_Type_euc::ANGLE_LOOP)增加一层环路，DjiMotorDeleteLoop删减。
2. **使用外部反馈**：在初始化后直接调用 motor->motor_controller_.SetAngleFeedbackPtr(...) 指向你自己的传感器。
3. **固定输出/开环**：调用 motor->SetMotorOutputFix(target_current); 或者把闭环改成 OPEN_LOOP 再用 DjiMotorSetRef。
4. **堵转检测**：Is_MotorStuck 会在你设置的阈值超限时置位 stuck_flag_，可以在主循环里轮询。
5. **力矩输出**：SetMotorOutputFix(float output)目前不支持GM6020，因为GM6020是电压控制。
6. **只接收模式**：需要某个电机挂在总线上仅回传数据时，直接 motor->OnlyReceive()。
7. **温度保护**：温度超过 60℃ 会自动 Disable()。

xd照抄xd，有问题迅速寻找当期负责motor框架的人