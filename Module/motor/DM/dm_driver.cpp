/************************** Dongguan-University of Technology -ACE**************************
* @file  dm_driver.cpp
* @brief DM关节电机驱动库
* @author 胡炜 coconutjuice
* @version v26.0.7.6
* @date 2025-11-28
* @brief 电机模块文件
* ==============================================================================
* @verbatim
* 避免冗余，用MIT控制就够了
* 详细使用看demo.md文件，太长了，看起来太难受了
* @attention
*      依赖文件：
        motor_base_def.hpp
* @version           time
* v26.0.0.0   全新的体验,加入电机文档    2025-11-28
* v26.0.4.1   修复DWT计时错误的问题 2025-12-02
* v26.0.5.1   需求性更改，达妙电机掉线之后不再发送失能帧，达妙电机mit自锁函数，达妙电机新增数据单圈相对角度归一化到-180~180
* v26.0.5.5   H7的C++类初始化，成员变量如果没有手动清，可能会保存以前下电的值
* v26.0.5.6   H7的C++类初始化(二)，成员变量如果没有手动清，可能会保存以前下电的值
* v26.0.6.6   修改了线速度和线位置相关逻辑;电机的滤波参数做了接口传入;修复了达妙的total_angle的零点是上电位置作为零点这一问题
* v26.0.7.6   修改了达妙原始反馈值单位，feedback_speed由RPM->rad/s
* v26.0.8.3   dm的error = 1表示正常不需要报错，接收函数内处理为0
************************** Dongguan-University of Technology -ACE***************************/
#include "dm_driver.hpp"

// 发送控制频率控制宏
#define CONTROL_SEND_HZ(HZ)\
{\
static int16_t hz = 0;\
hz++;\
if(hz < HZ)   return;\
hz = 0;\
}

using namespace Motor_n::DmMotor_n;

/**
 * @brief 绕过PID直接按照MIT格式输出固定力矩
 */
void DmDriver_c::SetMotorOutputFix(float output) {
    motor_working_status_ = MotorBaseDef_n::Motor_Working_Status_euc::MOTOR_OUTPUT_ONLY_ME;
    motor_controller_.pid_output = output;

    SetMITData(0,0,0,0,output);
    MotorBaseDef_n::Motor_Send_t send_data;
    send_data.position_des = float_to_uint(motor_mit_contorl_data_.pos, param_.p_min, param_.p_max, 16);
    send_data.velocity_des = float_to_uint(motor_mit_contorl_data_.vel, param_.v_min, param_.v_max, 12);
    send_data.Kp = float_to_uint(motor_mit_contorl_data_.kp, param_.kp_min, param_.kp_max, 12);
    send_data.Kd = float_to_uint(motor_mit_contorl_data_.kd, param_.kd_min, param_.kd_max, 12);
    send_data.torque_des = float_to_uint(motor_mit_contorl_data_.torq, param_.t_min, param_.t_max, 12);

    // std::copy(reinterpret_cast<uint8_t *>(&send_data), reinterpret_cast<uint8_t *>(&send_data) + sizeof(send_data), tx_buffer_.begin());
    // std::copy(tx_buffer_.begin(), tx_buffer_.end(), motor_can_instace_.tx_buff);
    tx_buffer_[0] = send_data.position_des >> 8;
    tx_buffer_[1] = send_data.position_des;
    tx_buffer_[2] = send_data.velocity_des >> 4;
    tx_buffer_[3] = ((send_data.velocity_des & 0xF) << 4) | (send_data.Kp >> 8);
    tx_buffer_[4] = send_data.Kp;
    tx_buffer_[5] = send_data.Kd >> 4;
    tx_buffer_[6] = ((send_data.Kd & 0xf) << 4) | (send_data.torque_des >> 8);
    tx_buffer_[7] = send_data.torque_des;
    std::copy(tx_buffer_.begin(), tx_buffer_.end(), motor_can_instance_.tx_buff_);
    CONTROL_SEND_HZ(1);
    motor_can_instance_.Transmit(1.0f);
}

/**
 * @brief 缓存MIT位置/速度/力矩参数
 */
void DmDriver_c::SetMITData(float pos, float vel, float kp, float kd, float t)
{
    // 可添加一些通用参数的限幅，可参考dm_mit_mode.cpp

    motor_mit_contorl_data_.pos = pos;
    motor_mit_contorl_data_.vel = vel;
    motor_mit_contorl_data_.kp = kp;
    motor_mit_contorl_data_.kd = kd;
    motor_mit_contorl_data_.torq = t;
}

/**
 * @brief 将缓存的MIT参数编码后发送
 */
void DmDriver_c::Transmit(float outtime)
{
    MotorBaseDef_n::Motor_Send_t send_data;
    send_data.position_des = float_to_uint(motor_mit_contorl_data_.pos, param_.p_min, param_.p_max, 16);
    send_data.velocity_des = float_to_uint(motor_mit_contorl_data_.vel, param_.v_min, param_.v_max, 12);
    send_data.Kp = float_to_uint(motor_mit_contorl_data_.kp, param_.kp_min, param_.kp_max, 12);
    send_data.Kd = float_to_uint(motor_mit_contorl_data_.kd, param_.kd_min, param_.kd_max, 12);
    send_data.torque_des = float_to_uint(motor_mit_contorl_data_.torq, param_.t_min, param_.t_max, 12);
    
    // std::copy(reinterpret_cast<uint8_t *>(&send_data), reinterpret_cast<uint8_t *>(&send_data) + sizeof(send_data), tx_buffer_.begin());
    // std::copy(tx_buffer_.begin(), tx_buffer_.end(), motor_can_instace_.tx_buff);
    tx_buffer_[0] = send_data.position_des >> 8;
    tx_buffer_[1] = send_data.position_des;
    tx_buffer_[2] = send_data.velocity_des >> 4;
    tx_buffer_[3] = ((send_data.velocity_des & 0xF) << 4) | (send_data.Kp >> 8);
    tx_buffer_[4] = send_data.Kp;
    tx_buffer_[5] = send_data.Kd >> 4;
    tx_buffer_[6] = ((send_data.Kd & 0xf) << 4) | (send_data.torque_des >> 8);
    tx_buffer_[7] = send_data.torque_des;
    std::copy(tx_buffer_.begin(), tx_buffer_.end(), motor_can_instance_.tx_buff_);
    if (motor_working_status_ != MotorBaseDef_n::Motor_Working_Status_euc::MOTOR_OUTPUT_ONLY_ME)
    motor_can_instance_.Transmit(outtime);
}

/**
 * @brief 发送外部准备好的完整数据帧
 */
void DmDriver_c::Transmit(const std::array<uint8_t, 8> &data, float outtime)
{
    std::copy(data.begin(), data.end(), tx_buffer_.begin());
    std::copy(data.begin(), data.end(), motor_can_instance_.tx_buff_);

    motor_can_instance_.Transmit(outtime);
}

/**
 * @brief 先设置状态再按需降频发送MIT命令
 */
void DmDriver_c::DmTransmit(float outtime ,DMMotor_Status_euc state_set)
{
    CONTROL_SEND_HZ(1);// >1才降频
    DmMotorStateSet(state_set);
    Transmit(outtime);
}

/**
 * @brief 解析DM反馈帧并刷新原始/加工数据
 */
void DmDriver_c::ParseCANData(const std::array<uint8_t, 8> data)
{
    // 处理原始数据
    int p_int, v_int, t_int;
    p_int = (data[1] << 8) | data[2];
    v_int = (data[3] << 4) | (data[4] >> 4);
    t_int = ((data[4] & 0xF) << 8) | data[5];

    // D[0] = ID(低4位) | ERR(高4位)
    const uint8_t motor_id = data[0] & 0x0F; // 如需使用可保留
    motor_data_.motor_raw_data.error = (data[0] >> 4) & 0x0F; // 错误码
    motor_data_.motor_raw_data.position = uint_to_float(p_int, param_.p_min, param_.p_max, 16); // 单位弧度
    motor_data_.motor_raw_data.feedback_speed = uint_to_float(v_int, param_.v_min, param_.v_max, 12); // 单位rad/s
    motor_data_.motor_raw_data.force_feedback = uint_to_float(t_int, param_.t_min, param_.t_max, 12); // 单位Nm
    motor_data_.motor_raw_data.temperature[0] = static_cast<float>(data[6]); // 单位摄氏度
    motor_data_.motor_raw_data.temperature[1] = static_cast<float>(data[7]); // 单位摄氏度

    // 处理后的数据
    motor_data_.motor_processed_data.speed = motor_data_.motor_raw_data.feedback_speed * RAD_2_DEGREE; // 单位°/s
    this->motor_data_.motor_processed_data.speed_difference =
        (this->motor_data_.motor_raw_data.feedback_speed - this->motor_data_.motor_raw_data.last_feedback_speed) * RAD_2_DEGREE; // 单位°/s
    motor_data_.motor_processed_data.torque = motor_data_.motor_raw_data.force_feedback; // 单位Nm
    float temp_absangle = (motor_data_.motor_raw_data.position*RAD_2_DEGREE); // 单位°
    temp_absangle = motor_maths.loop_fp32_constrain(temp_absangle,0.0f,360.0f); // 转为0~360°
    motor_data_.motor_processed_data.absolute_angle = temp_absangle;
    motor_data_.motor_processed_data.relative_angle =
            motor_maths.loop_fp32_constrain(motor_data_.motor_processed_data.absolute_angle - motor_data_.motor_fixed_param.zero_offset, 0.0f, 360.0f); // 单位°，0~360°
    motor_data_.motor_processed_data.relative_angle_180 =
            motor_maths.loop_fp32_constrain(motor_data_.motor_processed_data.relative_angle, -180.0f, 180.0f); // 单位°，-180~180°
    

    if (first_frame_)
    {
        // 首帧初始化总角度和圈数
        motor_data_.motor_processed_data.total_angle = motor_data_.motor_processed_data.relative_angle_180;
        motor_data_.motor_processed_data.total_round = motor_data_.motor_processed_data.total_angle / 360.0f;
        last_absolute_angle_ = motor_data_.motor_processed_data.absolute_angle;
        first_frame_ = false;
    }
    else
    {
        float error = motor_data_.motor_processed_data.absolute_angle - last_absolute_angle_;
        if (error > 180.0f)
        {
            error -= 360.0f;
        }
        else if (error < -180.0f)
        {
            error += 360.0f;
        }
        motor_data_.motor_processed_data.total_angle += error;
        motor_data_.motor_processed_data.total_round = motor_data_.motor_processed_data.total_angle / 360.0f; // 直接由累计角度计算圈数，避免累加误差
        last_absolute_angle_ = motor_data_.motor_processed_data.absolute_angle;
    }

    motor_data_.motor_raw_data.last_feedback_speed = motor_data_.motor_raw_data.feedback_speed;

    if(motor_data_.motor_raw_data.error == 1)
    { // 已将error注册到灯语报错中，但dm的error = 1表示正常不需要报错，故处理为0
        motor_data_.motor_raw_data.error = 0;
    }
}

/**
 * @brief 根据目标状态执行使能、失能、清错等操作
 */
void DmDriver_c::DmMotorStateSet(DMMotor_Status_euc state_set)
{
    switch (state_set)
    {
        case DM_CMD_MOTOR_MODE:
            Enable();
            break;
        case DM_CMD_RESET_MODE:
            Disable();
            break;
        case DM_CMD_CLEAR_ERROR:
            ClearErrorFlag();
            break;
        case DM_CMD_ZERO_POSITION:
            SetZero();
            break;
        default:
            break;
    }
}

/**
 * @brief 使能电机并更新运行状态
 */
void DmDriver_c::Enable()
{
    Transmit(setCtrl(0xFC),1.0f);   // 使能
    motor_init_flag_ = true; 
    motor_working_status_ = MotorBaseDef_n::Motor_Working_Status_euc::MOTOR_ENALBED;
}

/**
 * @brief 失能电机并阻止输出
 */
void DmDriver_c::Disable()
{
    Transmit(setCtrl(0xFD),1.0f);   // 失能
    motor_working_status_ = MotorBaseDef_n::Motor_Working_Status_euc::MOTOR_STOP;
}

/**
 * @brief 切换为只接收模式，不再发送命令
 */
void DmDriver_c::OnlyReceive()
{
    motor_working_status_ = MotorBaseDef_n::Motor_Working_Status_euc::MOTOR_ONLY_RECEIVE;
}

/**
 * @brief 向电机下发清除错误指令
 */
void DmDriver_c::ClearErrorFlag()
{
    Transmit(setCtrl(0xFB),1.0f);  // 清除错误标志
}

/**
 * @brief 将当前位置设为新的零点
 */
void DmDriver_c::SetZero()
{
    Transmit(setCtrl(0xFE),1.0f);  // 设置零点
}

/**
 * @brief 锁定当前角度（MIT 位置保持）
 */
void DmDriver_c::SelfLock(float kp, float kd, float torque)
{
    if (!self_lock_enabled_)
    {
        self_lock_pos_rad_ = motor_data_.motor_raw_data.position; // rad，记录一次
        self_lock_enabled_ = true;
    }
    SetMITData(self_lock_pos_rad_, 0.0f, kp, kd, torque);
    Transmit(1.0f);
}

/**
 * @brief 解除锁定，下次调用 SelfLock 会重新记录当前位置
 */
void DmDriver_c::CancelSelfLock()
{
    self_lock_enabled_ = false;
}
