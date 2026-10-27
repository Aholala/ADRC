/*************************** Dongguan-University of Technology -ACE**************************
 * @file    power_ctrl_noRLS.cpp
 * @author  coconutjuice 
 * @version v26.0.0.0
 * @date    2026-2-4
 * @brief   功率控制文件
 ******************************************************************************
 * @verbatim
 * 功率控制（使用递归最小二乘）
 * @attention
 *      依赖文件：
            motor_base_def
            new_Cap
            Alg_PID
            referee
 * @version           time
 * v26.0.0.0  全新的体验  2026-2-6  
   v27.0.0.0  全新的体验  2026-10-27 ahola  
 ************************** Dongguan-University of Technology -ACE***************************/

#include "power_ctrl.hpp"
#include "user_maths.hpp"

#include <cmath>
#include <limits>
#include <new>

#if IS_Connect_Super_Cap && CAP_CTRL_TYPE == PM01_CAP
#include "pm01/pm01.hpp"
#endif

void PowerCtrl_c::Init(Motors &motors)
{
    if (power_status_.initialized)
        return;

    for (std::size_t i = 0; i < Motors::WheelCount; i++)
    {
        if (motors[i] == nullptr || motors.target_speed[i] == nullptr ||
            !std::isfinite(motors.feedback_torque_scale[i]) ||
            motors.feedback_torque_scale[i] == 0.0f ||
            !std::isfinite(motors.feedback_speed_scale[i]) ||
            motors.feedback_speed_scale[i] == 0.0f ||
            !std::isfinite(motors.max_torque[i]) || motors.max_torque[i] <= 0.0f)
            return;
    }

#if CAP_CTRL_TYPE == PM01_CAP
    const float power_upper_limit = PM01_INIT_POWER_UPPER_LIMIT;
#elif CAP_CTRL_TYPE == OWNER_CAP
    const float power_upper_limit = OWNER_POWER_UPPER_LIMIT;
#else
    return;
#endif

#if CAP_CTRL_TYPE == PM01_CAP || CAP_CTRL_TYPE == OWNER_CAP
    const float config_values[] = {
        REFEREE_FALLBACK_POWER, power_upper_limit, USER_MIN_POWER,
        LOSS_RESERVE_POWER, CHARGE_RESERVE_POWER, RECHARGE_RESERVE_POWER,
        RLS_K1, RLS_K2, RLS_K3, RLS_DELTA, RLS_LAMBDA,
        MIN_VOLTAGE, MAX_VOLTAGE,
        ENERGY_PID_KP, ENERGY_PID_KI, ENERGY_PID_KD,
        ENERGY_PID_KFA, ENERGY_PID_KFB, ENERGY_PID_MAX_OUT,
        ENERGY_PID_MAX_IERROR, ENERGY_PID_DEADBAND,
        CAP_ON_TARGET_ENERGY, CAP_OFF_TARGET_ENERGY, OTHER_TARGET_ENERGY,
        WHEEL_ERROR_POWERDISTRIBUT_SET, WHEEL_PROP_POWERDISTRIBUT_SET,
    };
    for (float value : config_values)
    {
        if (!std::isfinite(value))
            return;
    }

    if (RLS_K1 < 0.0f || RLS_K2 <= 0.0f || RLS_K3 < 0.0f ||
        RLS_DELTA <= 0.0f || RLS_LAMBDA <= 0.0f || RLS_LAMBDA > 1.0f ||
        REFEREE_FALLBACK_POWER <= 0.0f || power_upper_limit <= 0.0f ||
        USER_MIN_POWER < 0.0f || USER_MIN_POWER > power_upper_limit ||
        LOSS_RESERVE_POWER < 0.0f ||
        MIN_VOLTAGE < 0.0f || MAX_VOLTAGE <= MIN_VOLTAGE ||
        ENERGY_PID_MAX_OUT < 0.0f || ENERGY_PID_MAX_IERROR < 0.0f ||
        ENERGY_PID_DEADBAND < 0.0f || WHEEL_PROP_POWERDISTRIBUT_SET < 0.0f ||
        WHEEL_ERROR_POWERDISTRIBUT_SET <= WHEEL_PROP_POWERDISTRIBUT_SET)
        return;

#if IS_Connect_Super_Cap
    if (!std::isfinite(float(CAP_NUM)) || CAP_NUM <= 0.0f ||
        !std::isfinite(float(SINGLE_CAP_CAPACITY)) || SINGLE_CAP_CAPACITY <= 0.0f)
        return;
#if CAP_CTRL_TYPE == PM01_CAP
    const float init_input_power = PM01_INIT_INPUT_POWER;
    if (!std::isfinite(init_input_power) || init_input_power < 0.0f ||
        init_input_power > 255.0f || std::floor(init_input_power) != init_input_power ||
        !std::isfinite(float(PM01_RUN_POWER_UPPER_LIMIT)) || PM01_RUN_POWER_UPPER_LIMIT <= 0.0f)
        return;
#endif
#endif

    /*电容能量PID*/
    //周期函数传入实际百分比
    alg_n::PidInitConfig_t energy_pid_config_ = {};
    energy_pid_config_.Kp = ENERGY_PID_KP;
    energy_pid_config_.Ki = ENERGY_PID_KI;
    energy_pid_config_.Kd = ENERGY_PID_KD;
    energy_pid_config_.Kfa = ENERGY_PID_KFA;
    energy_pid_config_.Kfb = ENERGY_PID_KFB;
    energy_pid_config_.ActualValueSource = nullptr;
    energy_pid_config_.mode = Output_Limit;
    energy_pid_config_.max_out = ENERGY_PID_MAX_OUT;
    energy_pid_config_.max_Ierror = ENERGY_PID_MAX_IERROR;
    energy_pid_config_.deadband = ENERGY_PID_DEADBAND;
    energy_pid_alg_ = new (std::nothrow) alg_n::PID_c(energy_pid_config_); // 能量误差
    if (energy_pid_alg_ == nullptr)
        return;

    for (std::size_t i = 0; i < Motors::WheelCount; i++)
    {
        motors_.motor[i] = motors[i];
        motors_.target_speed[i] = motors.target_speed[i];
        motors_.feedback_torque_scale[i] = motors.feedback_torque_scale[i];
        motors_.feedback_speed_scale[i] = motors.feedback_speed_scale[i];
        motors_.max_torque[i] = motors.max_torque[i];
    }

    //状态与RLS初值同步
    power_status_.k1 = RLS_K1;
    power_status_.k2 = RLS_K2;
    power_status_.k3 = RLS_K3;
    float initParams[2] = {power_status_.k1, power_status_.k2};
    rls_.setParamVector(Matrixf<2, 1>(initParams));

    power_data_.fallback_power = REFEREE_FALLBACK_POWER;
    power_data_.power_upper_limit = power_upper_limit;
    power_data_.min_power_limit = USER_MIN_POWER;
    power_data_.cap_loss_power = LOSS_RESERVE_POWER;
    power_data_.net_change_power = CHARGE_RESERVE_POWER;
    power_data_.net_rechange_power = RECHARGE_RESERVE_POWER;
    power_data_.wheel_error_power_distribution = WHEEL_ERROR_POWERDISTRIBUT_SET;
    power_data_.wheel_prop_power_distribution = WHEEL_PROP_POWERDISTRIBUT_SET;
    energy_controller_.max_voltage = MAX_VOLTAGE; // 超级电容最高电压
    energy_controller_.min_voltage = MIN_VOLTAGE; // 底盘可以动的最低电容电压

#ifdef USE_REFEREE
    power_referee_data_ = &Referee_n::referee_c::GetInstance()->referee_data_;
#endif

    // 初始化控制板
#if IS_Connect_Super_Cap
#if CAP_CTRL_TYPE == PM01_CAP
    pm01_ = Superpower_Calc::getinstance();
    pm01_->Init();
    pm01_->SetInitInputPower(std::uint8_t(init_input_power));
#elif CAP_CTRL_TYPE == OWNER_CAP
    power_cap_data_ = Cap_Ctrl_n::Cap_Ctrl_c::getInstance();
    power_cap_data_->init();
#endif
#endif

    power_status_.initialized = true;
#endif
}

void PowerCtrl_c::Power_Ctrl_Loop()
{
    if (!power_status_.initialized)
        return;

    // 判断错误状态
#if IS_Connect_Super_Cap
#if CAP_CTRL_TYPE == PM01_CAP
    power_status_.is_cap_online = pm01_->Pm01_is_connect != 1;
#elif CAP_CTRL_TYPE == OWNER_CAP
    power_status_.is_cap_online = get_cap_static() != 0;
#endif
#else
    power_status_.is_cap_online = false;
#endif

#ifdef USE_REFEREE
    power_status_.is_referee_online = power_referee_data_ != nullptr &&
        Referee_n::referee_c::GetInstance()->is_online_re_ != 0;
#else
    power_status_.is_referee_online = false;
#endif
    power_status_.is_all_motor_online = true;
    for (std::size_t i = 0; i < Motors::WheelCount; i++)
    {
        if (!motors_[i]->motor_online_flag_)
        {
            power_status_.is_all_motor_online = false;
            break;
        }
    }

#if CAP_CTRL_TYPE == PM01_CAP
    power_data_.power_upper_limit = PM01_RUN_POWER_UPPER_LIMIT;
#elif CAP_CTRL_TYPE == OWNER_CAP
    power_data_.power_upper_limit = OWNER_POWER_UPPER_LIMIT;
#endif

    SupperCapPowerLimitUpdate();
    UpdatePowerBudget();
    ReAndCapProcess();
    UpdateModel();
}

void PowerCtrl_c::ReAndCapProcess()
{
#if IS_Connect_Super_Cap
#if CAP_CTRL_TYPE == PM01_CAP
    std::uint16_t power_buffer = 50U;
#ifdef USE_REFEREE
    if (power_status_.is_referee_online && power_referee_data_ != nullptr)
        power_buffer = power_referee_data_->Power_Heat.buffer_energy;
#endif
    const float base = power_status_.basePowerLimit;
    if (!std::isfinite(base) || base < 0.0f || base >= 256.0f)
        return;
    const std::uint8_t base_encoded = std::uint8_t(base);

    const int request = base_encoded < 10U ? 120 : int(base_encoded) + (int(power_buffer) - 50) / 10;
    if (request < 0 || request > 655)
        return;
    pm01_->Set_Chassis_Power_Limit(base_encoded);
    pm01_->Read_and_Write_Power(power_buffer);
    power_status_.charge_power_set = pm01_->Cap_Input_Power;
#elif CAP_CTRL_TYPE == OWNER_CAP
    // 缓冲能量环设置充电功率
    power_status_.charge_power_set = power_status_.basePowerLimit;
#ifdef USE_REFEREE
    if (power_status_.is_referee_online && power_referee_data_ != nullptr)
        power_status_.charge_power_set = power_status_.basePowerLimit +
            (float(power_referee_data_->Power_Heat.buffer_energy) - 70) / 10;
#endif
    const float request = power_status_.charge_power_set;

    if (!std::isfinite(request) || request < 0.0f || request >= 656.0f)
        return;
    const std::uint32_t now_tick = HAL_GetTick();
    // 最小发送间隔 20ms
    if (!limit_send_initialized_ || (now_tick - last_limit_send_tick_) >= 20U)
    {
        power_cap_data_->Send_Limit_Data(std::uint16_t(request));
        last_limit_send_tick_ = now_tick;
        limit_send_initialized_ = true;
    }
#endif
#endif
}

void PowerCtrl_c::UpdateModel()
{
    Matrixf<2, 1> samples;
    samples[0][0] = 0;
    samples[1][0] = 0;
    float effectivePower = 0;
    bool all_motor_online = true;
    power_status_.rls_updated = false;

    for (std::size_t i = 0; i < Motors::WheelCount; i++)
    {
        if (motors_[i]->motor_online_flag_)
            motor_disconnect_counter_[i] = 0U;
        else
        {
            all_motor_online = false;
            motor_disconnect_counter_[i]++;
        }
        if (motor_disconnect_counter_[i] < 1000U) // 我们认为刚刚断开的电机仍在使用电力，假设电机保持
                                                  // 最新输出和RPM持续1秒，否则只使用能量回路是不安全的
        {
            const float motorTorque = motors_[i]->motor_data_.motor_raw_data.force_feedback * motors_.feedback_torque_scale[i];
            const float motorAv = motors_[i]->motor_data_.motor_raw_data.feedback_speed * motors_.feedback_speed_scale[i];
            effectivePower += motorTorque * motorAv;
            samples[0][0] += fabsf(motorAv);
            samples[1][0] += motorTorque * motorTorque;
        }
        else
        {
            motor_disconnect_counter_[i] = 1000U;
        }
    }

    // 模型公式：P = τω + k1|ω| + k2τ² + k3/4
    const float last_extpower = power_status_.estimatedPower;
    power_status_.estimatedPower = (0.9) * last_extpower + 0.1 *
        (power_status_.k1 * samples[0][0] + power_status_.k2 * samples[1][0] + effectivePower + power_status_.k3);

    // 获取当前功率
    // 有超电的情况下读取超电输出功率，否则使用预测的功率
    float measuredPower = power_status_.estimatedPower;
    power_status_.measuredPowerValid = false;
#if IS_Connect_Super_Cap
    if (power_status_.is_cap_online)
    {
#if CAP_CTRL_TYPE == PM01_CAP
        measuredPower = pm01_->pm01_od.p_out / 100.0f;
#elif CAP_CTRL_TYPE == OWNER_CAP
        measuredPower = power_cap_data_->cap_data.Chassis_Power;
#endif
        power_status_.measuredPowerValid = true;
    }
#endif
    power_status_.effectivePower = effectivePower;
    power_status_.chassis_power = measuredPower;
    power_status_.powerLoss = measuredPower - effectivePower;
    power_status_.efficiency = fabsf(measuredPower) > 1e-6f ?
        user_maths_c().user_val_limit(effectivePower / measuredPower, 0.0f, 1.0f) : 0.0f;

    if (power_status_.rls_enabled && power_status_.measuredPowerValid && all_motor_online &&
        fabs(measuredPower) > 5.0f && !(!power_status_.is_cap_online && power_status_.estimatedPower < 0))
    {
        Matrixf<2, 1> params = rls_.update(samples, measuredPower - effectivePower - power_status_.k3);
        power_status_.k1 = fmax(params[0][0], 1e-5f);
        power_status_.k2 = fmax(params[1][0], 1e-5f);
        power_status_.rls_updated = true;
    }

    //发布电压
#if IS_Connect_Super_Cap
#if CAP_CTRL_TYPE == PM01_CAP
    power_status_.Cap_Vol = pm01_->pm01_od.v_out / 100.0f;
    power_status_.charge_power = pm01_->pm01_od.p_in / 100;
    power_status_.cap_power = pm01_->pm01_od.p_out / 100;
    power_status_.chassis_energy = pm01_->true_energy;
#elif CAP_CTRL_TYPE == OWNER_CAP
    power_status_.Cap_Vol = power_cap_data_->cap_data.Vcap;
    power_status_.charge_power = power_cap_data_->cap_data.Pbat;
    power_status_.cap_power = power_cap_data_->cap_data.Pcap;
    power_status_.chassis_energy = power_cap_data_->cap_data.Chassis_energy;
#endif
    power_status_.estimatedCapEnergy = 0.5f * SINGLE_CAP_CAPACITY * power_status_.Cap_Vol * power_status_.Cap_Vol / CAP_NUM;
#endif
}

/**
 * @brief 获取当前功率状态
 * @return 当前功率状态
 */
const PowerStatus &PowerCtrl_c::GetPowerStatus() const
{
    return power_status_;
}

//是否启用RLS
void PowerCtrl_c::SetRLSEnabled(bool enabled)
{
    power_status_.rls_enabled = enabled;
}

//保存模式。
void PowerCtrl_c::SetPowerMode(PowerMode_e mode)
{
    power_mode_ = mode;
}

//设置最大功率
void PowerCtrl_c::SetMaxPowerConfigured(float power)
{
    if (!power_status_.initialized)
        return;

    //value limit
    power_status_.userConfiguredMaxPower = user_maths_c().user_val_limit(
        power, power_data_.min_power_limit, power_data_.power_upper_limit);
}

//设置净充电功率
void PowerCtrl_c::SetChangePower(float power)
{
    power_data_.net_change_power = power;
}

//设置净补电功率
void PowerCtrl_c::SetRechangePower(float power)
{
    power_data_.net_rechange_power = power;
}

//注册功率控制的回调函数
void PowerCtrl_c::RegisterPowerCallbackFunc(float (*callback)(void))
{
    power_callback_ = callback;
}

float PowerCtrl_c::MotorPowerFromModel(float torque, float speed_rad_s)
{
    // 根据模型公式计算单电机功率：P = τω + k1|ω| + k2τ² + k3/4
    // 入参已为N·m、rad/s
    return torque * speed_rad_s + fabs(speed_rad_s) * power_status_.k1 +
           torque * torque * power_status_.k2 + power_status_.k3 / float(Motors::WheelCount);
}

float PowerCtrl_c::SolveTorqueForTargetPower(float torque_origin, float speed_rad_s, float target_power)
{
    // 构造二次方程：k2τ² + ωτ + (k1|ω| + k3/4 - P_allocated) = 0
    float delta = speed_rad_s * speed_rad_s -
                  4.0f * power_status_.k2 * (power_status_.k1 * fabs(speed_rad_s) +
                  power_status_.k3 / float(Motors::WheelCount) - target_power);

    // 选择符号与原始PID输出一致的解
    if (fabs(delta) < 1e-5f) // repeat roots
    {
        return -speed_rad_s / (2.0f * power_status_.k2);
    }
    else if (delta > 0.0f) // distinct roots
    {
        return torque_origin > 0.0f ? (-speed_rad_s + sqrtf(delta)) / (2.0f * power_status_.k2)
                                    : (-speed_rad_s - sqrtf(delta)) / (2.0f * power_status_.k2);
    }
    else // 虚根情况，取中间值
    {
        return -speed_rad_s / (2.0f * power_status_.k2);
    }
}

/*超级电容给的功率上限*/
void PowerCtrl_c::SupperCapPowerLimitUpdate()
{
    // 计算能量百分比: (当前电压 - 最低电压) / (最高电压 - 最低电压) * 100
    energy_controller_.actual_energy_percent = (GetPowerStatus().Cap_Vol - energy_controller_.min_voltage) /
                                               (energy_controller_.max_voltage - energy_controller_.min_voltage) * 100.0f;

    energy_controller_.actual_energy_percent = user_maths_c().user_val_limit(
        energy_controller_.actual_energy_percent, 0.0f, 100.0f);

    switch (power_mode_)
    {
    case PowerMode_e::CAPACITOR_MODE_ON:
        energy_controller_.target_energy_percent = CAP_ON_TARGET_ENERGY; // 开电容：默认40%（激进，多用电）
        break;
    case PowerMode_e::CAPACITOR_MODE_OFF:
        energy_controller_.target_energy_percent = CAP_OFF_TARGET_ENERGY; // 关电容：默认80%（保守，多存电）
        break;
    default:
        energy_controller_.target_energy_percent = OTHER_TARGET_ENERGY; // 其它模式：默认100%
    }

    energy_controller_.dynamic_power_limit = energy_pid_alg_->Calc(
        energy_controller_.target_energy_percent, energy_controller_.actual_energy_percent);
}

void PowerCtrl_c::UpdatePowerBudget()
{
    //无裁判数据或失联时 立即回退基础
    power_status_.basePowerLimit = power_data_.fallback_power;
#ifdef USE_REFEREE
    if (power_status_.is_referee_online && power_referee_data_ != nullptr)
        power_status_.basePowerLimit = power_referee_data_->Robot_Status.chassis_power_limit;
#endif

    //能量环设置底盘功率上限
    if (power_callback_ == nullptr)
    {
        const float change_mode_power_limit = power_status_.basePowerLimit - power_data_.cap_loss_power - power_data_.net_change_power;
        const float rechange_mode_power_limit = power_status_.basePowerLimit - power_data_.cap_loss_power - power_data_.net_rechange_power;
        const float fly_mode_power_limit = power_data_.power_upper_limit;
        const float supercap_power_limit = power_status_.basePowerLimit + energy_controller_.dynamic_power_limit;

        if (power_mode_ == PowerMode_e::CHARGE_MODE)
            power_status_.candidatePowerLimit = change_mode_power_limit;
        else if (power_mode_ == PowerMode_e::RECHARGE_MODE)
            power_status_.candidatePowerLimit = rechange_mode_power_limit;
        else if (power_mode_ == PowerMode_e::FLY_MODE)
            power_status_.candidatePowerLimit = fly_mode_power_limit;
        else if (power_mode_ == PowerMode_e::CAPACITOR_MODE_OFF || power_mode_ == PowerMode_e::CAPACITOR_MODE_ON)
            power_status_.candidatePowerLimit = supercap_power_limit;
        else
            power_status_.candidatePowerLimit = power_status_.basePowerLimit - power_data_.cap_loss_power;
    }
    else //用户自定义功率上限
    {
        //USER随后仍优先使用用户设置值
        power_status_.candidatePowerLimit = power_callback_();
    }

    if (power_mode_ == PowerMode_e::USER_MODE)
        power_status_.candidatePowerLimit = power_status_.userConfiguredMaxPower;

    power_status_.maxPowerLimited = power_status_.candidatePowerLimit;

#if IS_Connect_Super_Cap
    if (!power_status_.is_cap_online)
        power_status_.maxPowerLimited = power_status_.basePowerLimit;
#else
    power_status_.maxPowerLimited = power_status_.basePowerLimit;
#endif
}

float *PowerCtrl_c::Calc(std::vector<float *> &raw_output,
                        std::vector<float *> &current_speed)
{
#if !TORQUE_SCALE_ENABLED
    constexpr float EPS = 1e-6f;
#endif
    const float invalid_power = std::numeric_limits<float>::quiet_NaN();
    float torque_origin[Motors::WheelCount] = {};
    float speed_rad_s[Motors::WheelCount] = {};
#if !TORQUE_SCALE_ENABLED
    float error[Motors::WheelCount] = {};
    float cmdPower[Motors::WheelCount] = {};
#endif
    bool online[Motors::WheelCount] = {};
    bool speed_valid[Motors::WheelCount] = {};
    const bool model_valid = power_status_.initialized &&
        std::isfinite(power_status_.k1) && power_status_.k1 >= 0.0f &&
        std::isfinite(power_status_.k2) && power_status_.k2 > 0.0f &&
        std::isfinite(power_status_.k3) && power_status_.k3 >= 0.0f;
    power_status_.sumPowerCmd_before_clamp = invalid_power;

    auto finish = [&](bool calculation_valid) -> float *
    {
        float predicted_power = 0.0f;
        bool prediction_valid = model_valid;
        for (std::size_t i = 0; i < Motors::WheelCount; i++)
        {
            if (!online[i])
            {
                torque_out_[i] = 0.0f;
                continue;
            }
            if (!calculation_valid)
                torque_out_[i] = torque_origin[i];
            torque_out_[i] = user_maths_c().user_val_limit(
                torque_out_[i], -motors_.max_torque[i], motors_.max_torque[i]);
            if (speed_valid[i] && model_valid)
                predicted_power += MotorPowerFromModel(torque_out_[i], speed_rad_s[i]);
            else
                prediction_valid = false;
        }
        power_status_.predictedPowerAfterLimit = prediction_valid && std::isfinite(predicted_power) ?
            predicted_power : invalid_power;
        power_status_.powerLimitUnreachable = !calculation_valid ||
            !std::isfinite(power_status_.maxPowerLimited) ||
            !std::isfinite(power_status_.predictedPowerAfterLimit) ||
            power_status_.predictedPowerAfterLimit > power_status_.maxPowerLimited;
        return torque_out_;
    };

    if (!power_status_.initialized)
        return finish(false);

    bool input_valid = raw_output.size() == Motors::WheelCount &&
        current_speed.size() == Motors::WheelCount;

    for (std::size_t i = 0; i < Motors::WheelCount; i++)
    {
        online[i] = motors_[i]->motor_online_flag_;
        if (!online[i])
            continue;
        if (i < raw_output.size() && raw_output[i] != nullptr && std::isfinite(*raw_output[i]))
            torque_origin[i] = *raw_output[i];
        else
            input_valid = false;
        speed_valid[i] = i < current_speed.size() && current_speed[i] != nullptr &&
            std::isfinite(*current_speed[i]);
        if (speed_valid[i])
            speed_rad_s[i] = *current_speed[i];
        else
            input_valid = false;
#if !TORQUE_SCALE_ENABLED
        if (std::isfinite(*motors_.target_speed[i]))
            error[i] = fabs(*motors_.target_speed[i] - speed_rad_s[i]);
        else
            input_valid = false;
        if (!std::isfinite(error[i]))
            input_valid = false;
#endif
    }
    if (!input_valid || !model_valid)
        return finish(false);

#if TORQUE_SCALE_ENABLED
    // 同一RLS模型沿公共系数alpha展开：P(alpha)=a*alpha²+b*alpha+c
    double a = 0.0, b = 0.0, c = 0.0;
    double alpha_max = 1.0;
    float sumCmdPower = 0.0f;
    for (std::size_t i = 0; i < Motors::WheelCount; i++)
    {
        if (!online[i])
            continue;
        const double torque = torque_origin[i];
        const double speed = speed_rad_s[i];
        a += double(power_status_.k2) * torque * torque;
        b += torque * speed;
        c += double(power_status_.k1) * fabs(speed) + double(power_status_.k3) / Motors::WheelCount;
        if (torque != 0.0)
            alpha_max = std::fmin(alpha_max, double(motors_.max_torque[i]) / fabs(torque));
        sumCmdPower += MotorPowerFromModel(torque_origin[i], speed_rad_s[i]);
    }
    if (!std::isfinite(sumCmdPower) || !std::isfinite(power_status_.maxPowerLimited))
        return finish(false);
    power_status_.sumPowerCmd_before_clamp = sumCmdPower;

    const double budget = power_status_.maxPowerLimited;
    double alpha = alpha_max;
    if ((a * alpha + b) * alpha + c > budget && a > 0.0)
    {
        //先找允许区间内的最低功率
        const double alpha_min_power = std::fmax(0.0, std::fmin(-b / (2.0 * a), alpha_max));
        const double min_power = (a * alpha_min_power + b) * alpha_min_power + c;
        alpha = alpha_min_power;
        if (min_power <= budget)
        {
            const double delta = std::fmax(b * b - 4.0 * a * (c - budget), 0.0);
            const double root = std::sqrt(delta);
            // 取最大可行系数；b>=0时用等价式避免相近数相减
            if (b >= 0.0)
                alpha = b + root > 0.0 ? 2.0 * (budget - c) / (b + root) : 0.0;
            else
                alpha = (-b + root) / (2.0 * a);
            alpha = std::fmax(alpha_min_power, std::fmin(alpha, alpha_max));
        }
        // 无可行预算时保持公共缩放，取区间最低功率点
    }
    float torque_scale = float(alpha);
    if (double(torque_scale) > alpha)
        torque_scale = std::nextafter(torque_scale, 0.0f);
    for (std::size_t i = 0; i < Motors::WheelCount; i++)
        torque_out_[i] = online[i] ? torque_origin[i] * torque_scale : 0.0f;
    return finish(true);
#else
    float wheel_sumError = 0.0f;
    float wheel_sumPowerRequired = 0.0f;
    float sumCmdPower = 0.0f;
    float sumPowerRequired = 0.0f;
    float allocatablePower = power_status_.maxPowerLimited;
    // 步骤1：计算总需求功率和误差总和
    for (std::size_t i = 0; i < Motors::WheelCount; i++)
    {
        torque_out_[i] = online[i] ? torque_origin[i] : 0.0f;
        if (!online[i])
            continue;
        cmdPower[i] = MotorPowerFromModel(torque_origin[i], speed_rad_s[i]);
        wheel_sumError += error[i];
        wheel_sumPowerRequired += cmdPower[i];
        sumCmdPower += cmdPower[i];
        if (fabs(cmdPower[i]) < 1e-5f || cmdPower[i] < 0.0f)
            allocatablePower += -cmdPower[i];
        else
            sumPowerRequired += cmdPower[i];
    }
    if (!std::isfinite(sumCmdPower) || !std::isfinite(wheel_sumError) ||
        !std::isfinite(sumPowerRequired) || !std::isfinite(allocatablePower))
        return finish(false);
    power_status_.sumPowerCmd_before_clamp = sumCmdPower;

    // 步骤2：判断是否需要限制功率；只读取S13的最终预算，包括USER失联覆盖。
    if (sumCmdPower > power_status_.maxPowerLimited)
    {
        // 防止分配总需求功率过小导致除零；此时退回原PID输出，避免NaN传播。
        if (sumPowerRequired <= EPS)
            return finish(true);

        // 保留原轮组含负功率、总需求仅正功率的分配口径，不约掉此比值。
        float wheel_allocatablePower = allocatablePower * wheel_sumPowerRequired / sumPowerRequired;
        float wheel_errorConfidence;
        if (wheel_sumError > power_data_.wheel_error_power_distribution)
            wheel_errorConfidence = 1.0f;
        else if (wheel_sumError > power_data_.wheel_prop_power_distribution)
            wheel_errorConfidence = user_maths_c().user_val_limit(
                (wheel_sumError - power_data_.wheel_prop_power_distribution) /
                (power_data_.wheel_error_power_distribution - power_data_.wheel_prop_power_distribution), 0.0f, 1.0f);
        else
            wheel_errorConfidence = 0.0f;
        if (!std::isfinite(wheel_allocatablePower))
            return finish(false);

        // 遍历每个电机，重新计算允许的力矩
        for (std::size_t i = 0; i < Motors::WheelCount; i++)
        {
            if (!online[i] || fabs(cmdPower[i]) < 1e-5f || cmdPower[i] < 0.0f)
                continue;
            const float powerWeight_Error = wheel_sumError > EPS ? error[i] / wheel_sumError : 0.0f;
            const float powerWeight_Prop = wheel_sumPowerRequired > EPS ? cmdPower[i] / wheel_sumPowerRequired : 0.0f;
            const float powerWeight = wheel_errorConfidence * powerWeight_Error +
                (1.0f - wheel_errorConfidence) * powerWeight_Prop;
            const float target_power = powerWeight * wheel_allocatablePower;
            if (!std::isfinite(target_power))
                return finish(false);
            torque_out_[i] = SolveTorqueForTargetPower(torque_origin[i], speed_rad_s[i], target_power);
            if (!std::isfinite(torque_out_[i]))
                return finish(false);
        }
    }
    // 无需限制时直接使用PID输出
    // 最后仍受电机N·m上限限制
    return finish(true);
#endif
}
