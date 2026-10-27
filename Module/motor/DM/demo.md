# DM 电机一步一步上手

## 预备动作
1. #include "dm_driver.hpp"，准备 Motor_n::DmMotor_n::DmDriver_c *motor 指针。
2. 初始化前 __disable_irq();，防止 CAN 中断抢跑。

## 步骤 1：填好基类配置
```cpp
namespace MM = Motor_n::MotorBaseDef_n;
MM::Motor_Base_Config_t cfg = MM::Motor_Base_Config_t("WheelMotor", MM::Motor_Type_euc::DM8009)
    .SetControlSetting(MM::Motor_Control_Setting_t{MM::Closeloop_Type_euc::ANGLE_AND_SPEED_LOOP})
    .SetPIDConfig(alg_n::PidInitConfig_t{/* Angle */},
                  alg_n::PidInitConfig_t{/* Speed */},
                  alg_n::PidInitConfig_t{/* Current */})
    .SetCANConfig(BSP_n::CanInitConfig_s{.can_handle = &hcan2, .tx_id = 0x4C, .rx_id = 0x4D})
    .SetMechanicalParams(MM::Motor_Data_t::Motor_Fixed_Param_t{.zero_offset = 0, .radius = 0.05f, .ratio = 1.0f})
    .SetOutputLimit(4.0f, -4.0f);
```
再准备 DM 官方调参上位机中对应的范围：
```cpp
Motor_n::DmMotor_n::DmDriver_c::DM_ModePrame_s dm_param = {
    .kp_min = 0,    .kp_max = 500,
    .kd_min = 0,    .kd_max = 5,
    .v_min = -45,   .v_max = 45,
    .p_min = -12.5, .p_max = 12.5,
    .t_min = -10,   .t_max = 10,
};
```
这些值必须和上位机“电机参数”页面一致，否则 MIT 控制会异常。

## 步骤 2：实例化 + 校零
```cpp
motor = new Motor_n::DmMotor_n::DmDriver_c(cfg, dm_param);
motor->motor_controller_.SetSpeedFeedbackPtr(&motor->motor_data_.motor_raw_data.feedback_speed);
motor->motor_controller_.SetAngleFeedbackPtr(&motor->motor_data_.motor_raw_data.position);
motor->SetZero();
auto *motorbase = &motor->get_base();
```

## 步骤 3：恢复中断
__enable_irq(); 之后回调才能把 DM 反馈写进对象。

## 步骤 4：主循环模板
```cpp
while (1)
{
    motor->Enable();                         // 确保电机被使能
    static float t = 0;
    float target_angle = 3.0f * sinf(t);     // 目标角度（弧度）
    float out = motor->PIDCalculate(&target_angle);
    motor->SetMITData(0, 0, 0, 0, out);      // 也可以直接 MIT 闭环
    motor->DmTransmit(1.0f, Motor_n::DmMotor_n::DmDriver_c::DM_CMD_MOTOR_MODE);
    t += 0.01f;
    HAL_Delay(1);
}
```
DmTransmit 内部带有简单的降频保护，可以直接周期调用。

## 温馨提示
1. **限流/限矩**：修改 .SetOutputLimit() 或者 SetMotorOutputFix() 即可。
2. **错误处理**：motor->ClearErrorFlag(); 立即清除过流、过温等错误；motor->OnlyReceive(); 可以在调参时仅观察数据。
3. **重新设零**：随时调用 motor->SetZero(); 即可把当前位置当作机械零位。
4. **MIT 模式**：SetMITData(pos, vel, kp, kd, torque); 然后直接 Transmit() 发出去，省掉 PID。

xd照抄xd，有问题迅速寻找当期负责motor框架的人
