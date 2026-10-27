/*************************** Dongguan-University of Technology -ACE**************************
 * @file    power_ctrl_noRLS.cpp
 * @author  coconutjuice 
 * @version v26.0.0.0
 * @date    2026-2-4
 * @brief   功率控制文件
 ******************************************************************************
 * @verbatim
 * 功率控制（不使用递归最小二乘）
   输入：每个电机的扭矩(牛·米)与转速(弧度/秒)
   模型: P = k0 + k1*T + k2*w + k3*T*w + k4*T^2 + k5*w^2
   总功率超限时，用衰减算法重新求扭矩
   功率衰减：只缩放正功率，负功率保持,所有功率按同一系数缩放
   扭矩衰减：所有扭矩按同一系数缩放
 * @attention
 *      依赖文件：
            motor_base_def
            new_Cap
            Alg_PID
            referee
 * @version           time
 * v26.0.0.0  全新的体验  2026-2-6    
 ************************** Dongguan-University of Technology -ACE***************************/
#include "power_ctrl_noRLS.hpp"

/**
* @brief 初始化
*/
void PowerCtrl_c::Init(Motors &motor)
{
    // 用法：先准备电机与K系数，再初始化功率控制
    power_cap_data_ = Cap_Ctrl_n::Cap_Ctrl_c::getInstance();
    power_cap_data_->init(); // 超电数据
    power_referee_data_ = &Referee_n::referee_c::GetInstance()->referee_data_; // 裁判系统数据
    motors_ = motor; // 电机数据
/* --------------------- 能量环相关初始化 ------------------------ */
    /*电容能量PID*/
    alg_n::PidInitConfig_t energy_pid_config_ = {
        .Kp = ENERGY_PID_KP,
        .Ki = ENERGY_PID_KI,
        .Kd = ENERGY_PID_KD,
        .Kfa = 0, 
        .Kfb = 0,
        .ActualValueSource = nullptr,
        .mode = Output_Limit ,
        .max_out    = ENERGY_PID_MAXOUT,
        .max_Ierror = ENERGY_PID_MAX_IERROR,
        .deadband   = 0.0f,
    };
    energy_pid_alg = new alg_n::PID_c(energy_pid_config_);    //能量误差
    energy_controller_.max_voltage = 23;     //超级电容最高电压
    energy_controller_.min_voltage = 14;     //底盘可以动的最低电容电压
    energy_controller_.min_power_limit = 40; //底盘可以动的最低功率
}

/**
* @brief 电机功率模型返回
*/
static inline float MotorPowerFromModel(float torque, float speed_rad_s, const float *k)
{
    if (k == nullptr)
    {
        return 0.0f;
    }
    // 电机功率模型：P = k0 + k1*T + k2*w + k3*T*w + k4*T^2 + k5*w^2
    // T为扭矩(牛·米)，w为转速(弧度/秒)
    return k[0] + k[1] * torque + k[2] * speed_rad_s + k[3] * torque * speed_rad_s +
           k[4] * torque * torque + k[5] * speed_rad_s * speed_rad_s;
}

/**
* @brief 求解二次方程（ρP）
*/
static float SolveTorqueForTargetPower(float torque_origin, float speed_rad_s, const float *k, float target_power)
{
    if (k == nullptr)
    {
        return 0.0f;
    }
    // 固定 w 后，对扭矩 T 求解二次方程：
    // k4*T^2 + (k1 + k3*w)*T + (k0 + k2*w + k5*w^2 - P目标) = 0
    // 求解速度固定时转矩的二次方程，选择最接近原扭矩的根
    const float a = k[4];
    const float b = k[1] + k[3] * speed_rad_s;
    const float c = k[0] + k[2] * speed_rad_s + k[5] * speed_rad_s * speed_rad_s - target_power;

    const float eps = 1e-6f;
    if (std::fabs(a) < eps)
    {
        if (std::fabs(b) < eps)
        {
            return 0.0f;
        }
        return -c / b;
    }

    const float delta = b * b - 4.0f * a * c;
    if (delta < 0.0f)
    {
        // 无实数解，认为不可达
        return 0.0f;
    }

    const float sqrt_delta = std::sqrt(delta);
    const float t1 = (-b - sqrt_delta) / (2.0f * a);
    const float t2 = (-b + sqrt_delta) / (2.0f * a);
    // 选与原始扭矩最接近的解（来自推导说明）
    return (std::fabs(t1 - torque_origin) <= std::fabs(t2 - torque_origin)) ? t1 : t2;
}

/**
* @brief 求解二次方程（ηI）
*/
static float SolveEtaForTorque(const float *torque_origin, const float *speed_rad_s, const float (*k)[6], int motor_count, float pmax)
{
    const int count = motor_count < 0 ? 0 : (motor_count > CHASSIS_TYPE ? CHASSIS_TYPE : motor_count);
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;

    // 假设统一缩放：扭矩' = 系数 * 扭矩，构造总功率关于系数的二次方程
    for (int i = 0; i < count; i++)
    {
        const float T = torque_origin ? torque_origin[i] : 0.0f;
        const float w = speed_rad_s ? speed_rad_s[i] : 0.0f;
        const float *ki = k ? k[i] : nullptr;
        const float k0 = ki ? ki[0] : 0.0f;
        const float k1 = ki ? ki[1] : 0.0f;
        const float k2 = ki ? ki[2] : 0.0f;
        const float k3 = ki ? ki[3] : 0.0f;
        const float k4 = ki ? ki[4] : 0.0f;
        const float k5 = ki ? ki[5] : 0.0f;

        a += k4 * T * T;
        b += k1 * T + k3 * T * w;
        c += k0 + k2 * w + k5 * w * w;
    }

    const float eps = 1e-6f;
    if (std::fabs(a) < eps)
    {
        if (std::fabs(b) < eps)
        {
            return 0.0f;
        }
        // 一次情况：b*系数 + (c - 功率上限) = 0
        const float eta = (pmax - c) / b;
        if (eta < 0.0f || eta > 1.0f)
        {
            return 0.0f;
        }
        return eta;
    }

    const float delta = b * b - 4.0f * a * (c - pmax);
    if (delta < 0.0f)
    {
        return 0.0f;
    }

    const float sqrt_delta = std::sqrt(delta);
    const float eta1 = (-b - sqrt_delta) / (2.0f * a);
    const float eta2 = (-b + sqrt_delta) / (2.0f * a);
    const bool in1 = (eta1 >= 0.0f && eta1 <= 1.0f);
    const bool in2 = (eta2 >= 0.0f && eta2 <= 1.0f);

    // 选择[0,1]内的根；两者都在则取较大
    if (in1 && in2) return (eta1 > eta2) ? eta1 : eta2;
    if (in1) return eta1;
    if (in2) return eta2;
    return 0.0f;
}


/**
* @brief 衰减功率法：返回衰减系数rho并输出衰减后的力矩
*/
float PowerDecaySolve(const float *torque_origin,
                      const float *speed_rad_s,
                      const float (*k)[6],
                      int motor_count,
                      float rho,
                      float *torque_out)
{
    // 按功率衰减系数计算目标功率并反解扭矩
    for (int i = 0; i < motor_count; i++)
    {
        const float T0 = torque_origin ? torque_origin[i] : 0.0f;
        const float w = speed_rad_s ? speed_rad_s[i] : 0.0f;
        const float p0 = MotorPowerFromModel(T0, w, k ? k[i] : nullptr);
        const float target_power = (p0 > 0.0f) ? (rho * p0) : p0;
        if (torque_out)
        {
            torque_out[i] = SolveTorqueForTargetPower(T0, w, k ? k[i] : nullptr, target_power);
        }
    }
    return rho;
}

/**
* @brief 衰减扭矩法：返回衰减系数eta并输出衰减后的力矩
*/
float CurrentDecaySolve(const float *torque_origin,
                        const float *speed_rad_s,
                        const float (*k)[6],
                        int motor_count,
                        float pmax,
                        float *torque_out)
{
    const int count = motor_count < 0 ? 0 : (motor_count > CHASSIS_TYPE ? CHASSIS_TYPE : motor_count);
    const float eta = SolveEtaForTorque(torque_origin, speed_rad_s, k, motor_count, pmax);

    // 按系数对扭矩做统一缩放
    for (int i = 0; i < count; i++)
    {
        if (torque_out) torque_out[i] = (torque_origin ? torque_origin[i] : 0.0f) * eta;
    }
    return eta;
}

/**
* @brief 功率模式设置
*/
void PowerCtrl_c::SetPowerMode(PowerMode_e power_mode)
{
    power_mode_ =  power_mode;
}

/**
* @brief 电容和裁判系统在线处理
*/
void PowerCtrl_c::ReAndCapProcess()
{
/* --------------------- 获取在线情况 ------------------------ */
    static uint8_t last_robot_powerlimit_;
    is_cap_online_ = get_cap_static();
    is_re_online_ = Referee_n::referee_c::GetInstance()->is_online_re_;
/* --------------------- 判断在线情况 ------------------------ */
#ifdef USE_SUPPER_CAP
    if (is_cap_online_ && is_re_online_)
    {
        uint16_t change_power_set = power_referee_data_->Robot_Status.chassis_power_limit + (power_referee_data_->Power_Heat.buffer_energy - 70)/10;
        Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->Send_Limit_Data(change_power_set);
    }
    else if (is_cap_online_ && !is_re_online_)
    {
        uint16_t change_power_set = last_robot_powerlimit_;
        Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->Send_Limit_Data(change_power_set);
    }
    // todo 在线状态完善
    last_robot_powerlimit_ = power_referee_data_->Robot_Status.chassis_power_limit;
#else
    if (is_re_online_)
    {
        uint16_t change_power_set = power_referee_data_->Robot_Status.chassis_power_limit + (power_referee_data_->Power_Heat.buffer_energy - 70)/10;
        Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->Send_Limit_Data(change_power_set);
    }
    else if (!is_re_online_)
    {
        uint16_t change_power_set = last_robot_powerlimit_;
        Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->Send_Limit_Data(change_power_set);
    }
    // todo 在线状态完善
    last_robot_powerlimit_ = power_referee_data_->Robot_Status.chassis_power_limit;
#endif
}

/**
* @brief 超级电容给的功率上限调节
*/
void PowerCtrl_c::SupperCapPowerLimitUpdate()
{
/* --------------------- 计算能量百分比 ------------------------ */
    // (当前电压 - 最低电压) / (最高电压 - 最低电压) * 100
    energy_controller_.actual_energy_percent = (power_cap_data_->cap_data.Vcap - energy_controller_.min_voltage)/
                                                (energy_controller_.max_voltage - energy_controller_.min_voltage) *100.0f;
    energy_controller_.actual_energy_percent = user_maths_c().user_val_limit(energy_controller_.actual_energy_percent, 0.0f, 100.0f);
/* --------------------- 根据我的设置来设置 ------------------------ */
    switch (power_mode_) {
    case PowerMode_e::CAPACITOR_MODE_ON:
        energy_controller_.target_energy_percent = CAP_ON_TARGET_ENERGY;  // 开电容模式: 40% 目标能量 (激进，多用电)
        break;
    case PowerMode_e::CAPACITOR_MODE_OFF:
        energy_controller_.target_energy_percent = CAP_OFF_TARGET_ENERGY;  // 关电容模式: 80% 目标能量 (保守，多存电)
        break;
    default:
        energy_controller_.target_energy_percent = 100.0f;  // 关电容模式: 100% 目标能量 (保守，多存电)
    }
/* --------------------- 制作动态功率限制 ------------------------ */
    energy_controller_.dynamic_power_limit = energy_pid_alg->Calc(energy_controller_.target_energy_percent, energy_controller_.actual_energy_percent);
}

/**
* @brief 功率分配函数（主要函数）
*/
float *PowerCtrl_c::Calc(std::vector<float *> &raw_output , std::vector<float *> &current_speed)
{
    // 输入1：扭矩指针(牛·米)
    // 输入2：转速指针(弧度/秒)
    // 返回静态数组 torque_out[CHASSIS_TYPE] 的指针
/* --------------------- 数据清零、获取 ------------------------ */
    for (int i = 0; i < CHASSIS_TYPE; i++)
    {
        measured_single_power_[i] = 0.0f;
        measured_feedback_single_power_[i] = 0.0f;
    }
    measured_sum_power_ = 0.0f;
    measured_wheel_sum_power_ = 0.0f;
    measured_steer_sum_power_ = 0.0f;
    measured_feedback_sum_power_ = 0.0f;
    measured_feedback_wheel_sum_power_ = 0.0f;
    measured_feedback_steer_sum_power_ = 0.0f;
    wheel_pos = 0.0f;
    steer_pos = 0.0f;
    wheel_neg = 0.0f;
    steer_neg = 0.0f;
    static float torque_out[CHASSIS_TYPE] = {0.0f};
    // 功率上限 = 电容动态能量控制 + 裁判系统上限
    #ifdef USE_SUPPER_CAP
        power_limit_set_ = energy_controller_.dynamic_power_limit + power_referee_data_->Robot_Status.chassis_power_limit;
    #else
        power_limit_set_ = power_referee_data_->Robot_Status.chassis_power_limit;
    #endif
/* --------------------- 数据有误，直接返回 ------------------------ */
    if(raw_output.size() != CHASSIS_TYPE || current_speed.size() != CHASSIS_TYPE)
    {
        // 尺寸不匹配：在线电机返回已有扭矩，否则置0
        for (int i = 0; i < CHASSIS_TYPE; i++)
        {
            const bool online = (motors_[i] != nullptr) && motors_[i]->motor_online_flag_;
            torque_out[i] = (online && i < static_cast<int>(raw_output.size()) && raw_output[i] != nullptr) ? *raw_output[i] : 0.0f;
        }
        return torque_out;
    }
/* --------------------- 计算预测功率 ------------------------ */
    // 缓存原始扭矩/转速，供衰减求解使用
    float torque_origin[CHASSIS_TYPE] = {0.0f};
    float speed_rad_s[CHASSIS_TYPE] = {0.0f};
    for (int i = 0; i < CHASSIS_TYPE; i++)
    {
        // 仅在线电机参与功率计算，离线置零
        if (motors_[i] != nullptr && motors_[i]->motor_online_flag_)
        {
            // 电机功率模型：P = k0 + k1*T + k2*w + k3*T*w + k4*T^2 + k5*w^2
            float T = *raw_output[i];
            float w = *current_speed[i];
            const float *k = motors_.KAt(i);
            const float feedback_torque = motors_[i]->motor_data_.motor_processed_data.torque / motors_[i]->motor_data_.motor_fixed_param.ratio;
            measured_single_power_[i] = MotorPowerFromModel(T, w, k);
            measured_feedback_single_power_[i] = MotorPowerFromModel(feedback_torque, w, k);
            if (i < 4)
            {
                measured_wheel_sum_power_ += measured_single_power_[i];
                measured_feedback_wheel_sum_power_ += measured_feedback_single_power_[i];
                if (measured_single_power_[i] > 0.0f) wheel_pos += measured_single_power_[i];
                else wheel_neg += measured_single_power_[i];
            }
            else
            {
                measured_steer_sum_power_ += measured_single_power_[i];
                measured_feedback_steer_sum_power_ += measured_feedback_single_power_[i];
                if (measured_single_power_[i] > 0.0f) steer_pos += measured_single_power_[i];
                else steer_neg += measured_single_power_[i];
            }
            measured_sum_power_ += measured_single_power_[i];
            measured_feedback_sum_power_ += measured_feedback_single_power_[i];
            torque_origin[i] = T;
            speed_rad_s[i] = w;
        }
        else
        {
            torque_origin[i] = 0.0f;
            speed_rad_s[i] = 0.0f;
        }
    }
/* --------------- 无需功率限制，直接返回 ----------------- */
    if (measured_sum_power_ <= power_limit_set_)
    {
        for (int i = 0; i < CHASSIS_TYPE; i++)
        {
            torque_out[i] = torque_origin[i];
        }
        return torque_out;
    }
    /* --------------- 需要功率限制，开始计算 ----------------- */
#if CHASSIS_TYPE == EIGHT
    // 1) 舵向功率：最多使用50%总限制功率，使用功率衰减法
    float steer_limit = 0.5f * power_limit_set_;
    float steer_rho = 1.0f;
    if (steer_pos > 0.0f)
    {
        steer_rho = (steer_limit - steer_neg) / steer_pos;
        if (steer_rho < 0.0f) steer_rho = 0.0f;
        if (steer_rho > 1.0f) steer_rho = 1.0f;
    }
    const float steer_total = steer_neg + steer_rho * steer_pos;

    // 2) 轮向功率：使用剩余功率（宏定义决定衰减方式）
    float wheel_limit = power_limit_set_ - steer_total;
    if (wheel_limit < 0.0f) wheel_limit = 0.0f;

    float wheel_torque[4] = {0.0f};
    float wheel_speed[4] = {0.0f};
    float wheel_out[4] = {0.0f};
    float wheel_k[4][6] = {{0.0f}};
    float steer_torque[4] = {0.0f};
    float steer_speed[4] = {0.0f};
    float steer_out[4] = {0.0f};
    float steer_k[4][6] = {{0.0f}};

    for (int i = 0; i < 4; i++)
    {
        wheel_torque[i] = torque_origin[i];
        wheel_speed[i] = speed_rad_s[i];
        steer_torque[i] = torque_origin[i + 4];
        steer_speed[i] = speed_rad_s[i + 4];
        for (int j = 0; j < 6; j++)
        {
            wheel_k[i][j] = motors_.k[i][j];
            steer_k[i][j] = motors_.k[i + 4][j];
        }
    }

    if (steer_pos > 0.0f && steer_rho < 1.0f)
    {
        PowerDecaySolve(steer_torque, steer_speed, steer_k, 4, steer_rho, steer_out);
    }
    else
    {
        for (int i = 0; i < 4; i++) { steer_out[i] = steer_torque[i]; }
    }

    const float wheel_total = wheel_pos + wheel_neg;
    if (wheel_limit <= 0.0f)
    {
        for (int i = 0; i < 4; i++) { wheel_out[i] = 0.0f; }
    }
    else if (wheel_total <= wheel_limit)
    {
        for (int i = 0; i < 4; i++) { wheel_out[i] = wheel_torque[i]; }
    }
    else
    {
#ifdef POWER_CTRL_USE_TORQUE_DECAY
        CurrentDecaySolve(wheel_torque, wheel_speed, wheel_k, 4, wheel_limit, wheel_out);
#else
        float wheel_rho = 1.0f;
        if (wheel_pos > 0.0f)
        {
            wheel_rho = (wheel_limit - wheel_neg) / wheel_pos;
            if (wheel_rho < 0.0f) wheel_rho = 0.0f;
            if (wheel_rho > 1.0f) wheel_rho = 1.0f;
        }
        PowerDecaySolve(wheel_torque, wheel_speed, wheel_k, 4, wheel_rho, wheel_out);
#endif
    }

    for (int i = 0; i < 4; i++)
    {
        torque_out[i] = wheel_out[i];
        torque_out[i + 4] = steer_out[i];
    }
    return torque_out;
#elif CHASSIS_TYPE == FOUR
    // FOUR：开宏用扭矩衰减，不开用功率衰减
    const float total_pos = wheel_pos;
    const float total_neg = wheel_neg;
    if (total_pos <= 0.0f)
    {
        for (int i = 0; i < CHASSIS_TYPE; i++)
        {
            torque_out[i] = 0.0f;
        }
        return torque_out;
    }
#ifdef POWER_CTRL_USE_TORQUE_DECAY
    CurrentDecaySolve(torque_origin, speed_rad_s, motors_.k, CHASSIS_TYPE, power_limit_set_, torque_out);
#else
    float rho = (power_limit_set_ - total_neg) / total_pos;
    if (rho < 0.0f) rho = 0.0f;
    if (rho > 1.0f) rho = 1.0f;
    PowerDecaySolve(torque_origin, speed_rad_s, motors_.k, CHASSIS_TYPE, rho, torque_out);
#endif
    return torque_out;
#else
    // 其它电机数量：默认功率衰减
    float sum_pos = 0.0f;
    float sum_neg = 0.0f;
    for (int i = 0; i < CHASSIS_TYPE; i++)
    {
        if (measured_single_power_[i] > 0.0f) sum_pos += measured_single_power_[i];
        else sum_neg += measured_single_power_[i];
    }
    if (sum_pos <= 0.0f)
    {
        for (int i = 0; i < CHASSIS_TYPE; i++) { torque_out[i] = 0.0f; }
        return torque_out;
    }
    float rho = (power_limit_set_ - sum_neg) / sum_pos;
    if (rho < 0.0f) rho = 0.0f;
    if (rho > 1.0f) rho = 1.0f;
    PowerDecaySolve(torque_origin, speed_rad_s, motors_.k, CHASSIS_TYPE, rho, torque_out);
    return torque_out;
#endif
}

