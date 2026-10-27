#include "power_ctrl.hpp"
#include "user_maths.hpp"


namespace Core
{
namespace Control
{
namespace Power
{
Manager manager(Manager::Motors(), Division_e::SENTRY);// 功率管理器实例
PowerStatus powerStatus; // 当前功率状态
static bool isCapEnergyOut                = false;
static uint16_t motorDisconnectCounter[Wheel_Num] = {0U};

Referee_n::RefereeData_t *robotMessage = nullptr;

static inline bool floatEqual(float a, float b) { return fabs(a - b) < 1e-5f; }

/**
 * @brief 将电机转速从RPM转换为角速度（rad/s）
 */
static inline float rpm2av(float rpm) { return rpm * (float)M_PI / 30.0f; }

/**
 * @brief 将电机转速从角速度转换为RPM
 */
static inline float av2rpm(float av) { return av * 30.0f / (float)M_PI; }

static inline void setErrorFlag(uint8_t &curFlag, Manager::ErrorFlags_e setFlag) { curFlag |= static_cast<uint8_t>(setFlag); }

static inline void clearErrorFlag(uint8_t &curFlag, Manager::ErrorFlags_e clearFlag) { curFlag &= (~static_cast<uint8_t>(clearFlag)); }

static inline bool isFlagged(uint8_t &curFlag, Manager::ErrorFlags_e flag) { return (curFlag & static_cast<uint8_t>(flag)) != 0; }

/**
 * @brief 检查电机是否连接
 */
static inline bool isMotorConnected(const Motor_n::DjiMotor_n::DjiDriver_c *motor) { return motor != nullptr && motor->motor_online_flag_ == true; }

static inline bool isAllMotorConnected()
{
    for (int i = 0; i < 4; i++)
        if (not isMotorConnected(manager.motors[i]))
            return false;
    return true;
}

/*电容能量PID*/
    alg_n::PidInitConfig_t energy_pid_config_ = {
    .Kp = -1.0f,
    .Ki = 0.0f,
    .Kd = 1.0f,
    .Kfa = 0,
    .Kfb = 0,
    .ActualValueSource = nullptr,
    .mode = Output_Limit ,
    .max_out    = 35.0f,
    .max_Ierror = 8.0f,
    .deadband   = 0.0f,
};




Manager::Manager(
    const Motors &motors_, const Division_e division_, RLSEnabled_e rlsEnabled_, const float k1_, const float k2_, const float k3_, const float lambda_)

    : rlsEnabled(rlsEnabled_),
      error(0UL),
      motors(motors_),
      division(division_),
      userConfiguredMaxPower(0.0f),
      callback(nullptr),
      k1(k1_),
      k2(k2_),
      k3(k3_),
      lastUpdateTick(0),
      rls(1e-5f, 0.99999f)
{
    configASSERT(k1_ >= 0);
    configASSERT(k2_ >= 0);
    configASSERT(k3_ >= 0);

    float initParams[2] = {k1_, k2_};
    rls.setParamVector(Matrixf<2, 1>(initParams));
}



/**
 * @brief 设置系统的最大功率
 * @param maxPower 要设置的最大功率值
 */
void setMaxPowerConfigured(float maxPower)
{
    //value limit
    manager.userConfiguredMaxPower = user_maths_c().user_val_limit(maxPower, manager.power_data.min_powerlimit, manager.power_data.powerUpperLimit);
}

void setPowerMode(Power_Mode_e mode) { manager.power_mode_ = mode; }

/**
 * @brief 注册功率控制的回调函数
 * @param callback 回调函数指针
 */
void registerPowerCallbackFunc(float (*callback)(void)) { manager.callback = callback; }

/**
 * @brief 根据输入启用或禁用RLS（递归最小二乘法）
 * @param enable 启用标志（0 表示禁用，非零表示启用）
 */
void setRLSEnabled(uint8_t enable) { manager.rlsEnabled = static_cast<Manager::RLSEnabled_e>(enable); }

/**
 * @brief 获取当前功率状态
 * @return 当前功率状态
 */
const volatile PowerStatus &getPowerStatus() { return powerStatus; }

/**
 * @brief 设置净充电功率
 */
void set_change_power(float change_power)
{
    manager.power_data.net_change_power = change_power;
}

/**
 * @brief 设置净补电功率
 */
void set_rechange_power(float rechange_power)
{
    manager.power_data.net_rechange_power = rechange_power;
}

/**
 * @brief 根据当前系统组件的状态设置错误标志
 */
static void setErrorFlag()
{
    /*判断错误状态*/
#if IS_Connect_Super_Cap
    #if CAP_CTRL_TYPE==PM01_CAP
        if (Superpower_Calc::getinstance()->Pm01_is_connect==1)
            setErrorFlag(manager.error, Manager::CAPDisConnect);
        else
            clearErrorFlag(manager.error, Manager::CAPDisConnect);
    #elif CAP_CTRL_TYPE==OWNER_CAP
        if (get_cap_static()==0)
            setErrorFlag(manager.error, Manager::CAPDisConnect);
        else
            clearErrorFlag(manager.error, Manager::CAPDisConnect);
    #endif
#else
    setErrorFlag(manager.error, Manager::CAPDisConnect);
#endif

    if (Referee_n::referee_c::GetInstance()->is_online_re_ == 0)
        setErrorFlag(manager.error, Manager::RefereeDisConnect);
    else
        clearErrorFlag(manager.error, Manager::RefereeDisConnect);

    if (not isAllMotorConnected())
        setErrorFlag(manager.error, Manager::MotorDisconnect);
    else
        clearErrorFlag(manager.error, Manager::MotorDisconnect);
}



/**
 * @brief 计算受控输出力矩（核心函数）
 * @param objs 四个电机的PowerObj指针数组
 * @return float* 调整后的力矩指令数组
 * @note 实现功率分配策略：根据误差动态分配功率，求解二次方程限制力矩
 */
float *getControlledOutput(std::vector<PowerObj *> &objs)
{
    static float cmdPower[Wheel_Num];
    static float error[Wheel_Num];
    static float newTorqueCurrent[Wheel_Num];// 存储调整后的力矩指令
    constexpr float EPS = 1e-6f;
    // 参数有误，取消限制
    if(objs.size() != Wheel_Num)
    {
        for (int i = 0; i < Wheel_Num; i++)
        {
            newTorqueCurrent[i] = 0.0f;
        }
        const int n = (objs.size() < Wheel_Num) ? static_cast<int>(objs.size()) : Wheel_Num;
        for (int i = 0; i < n; i++)
        {
            if (objs[i] != nullptr && isMotorConnected(manager.motors[i]))
            {
                newTorqueCurrent[i] = objs[i]->pidOutput;
            }
        }
        return newTorqueCurrent;
    }

    manager.computed_value.wheel_sumError = 0;
    manager.computed_value.wheel_sumPowerRequired = 0;
    manager.computed_value.rudder_sumError = 0 ;
    manager.computed_value.rudder_sumPowerRequired = 0;
    manager.computed_value.sumCmdPower = 0;
    manager.computed_value.sumError = 0;

    // 步骤1：计算总需求功率和误差总和


    if(manager.power_mode_ == Power_Mode_e::USER_MODE)
        // manager.computed_value.maxPower = user_maths_c().user_val_limit(manager.userConfiguredMaxPower, manager.power_data.min_powerlimit, manager.power_data.powerUpperLimit);
        manager.computed_value.maxPower = manager.userConfiguredMaxPower;
    else manager.computed_value.maxPower = manager.power_set.Power_limit_set;

    manager.computed_value.allocatablePower = manager.computed_value.maxPower;
    manager.computed_value.sumPowerRequired = 0.0f;
#if USE_DEBUG
    static float newCmdPower;
#endif
    for (int i = 0; i < Wheel_Num; i++)
    {
        //电机连接
        if (isMotorConnected(manager.motors[i]))
        {
            // 根据模型公式计算单电机功率：P = τω + k1|ω| + k2τ² + k3/4
            PowerObj *p = objs[i];
            if(i<4)
            {
                cmdPower[i] = (p->pidOutput * manager.computed_value.k0 * p->curAv + fabs(p->curAv) * manager.k1 + p->pidOutput * manager.computed_value.k0 * p->pidOutput * manager.computed_value.k0 * manager.k2 +
                          manager.k3 / static_cast<float>(Wheel_Num));
                error[i] = fabs(p->setAv - p->curAv);     //轮电机用速度误差
                manager.computed_value.wheel_sumError += error[i];
                manager.computed_value.wheel_sumPowerRequired += cmdPower[i];
            }
            else
            {
                cmdPower[i] = (p->pidOutput * manager.computed_value.rudder_k0 * p->curAv + fabs(p->curAv) * manager.k1 + p->pidOutput * manager.computed_value.rudder_k0 * p->pidOutput * manager.computed_value.rudder_k0 * manager.k2 +
                          manager.k3 / static_cast<float>(Wheel_Num));
                error[i] = p->Angle_Error ;    //舵电机用角度误差
                manager.computed_value.rudder_sumError += error[i];
                manager.computed_value.rudder_sumPowerRequired += cmdPower[i];
            }
            manager.computed_value.sumCmdPower += cmdPower[i];
            // 计算转速误差（用于动态分配）

            if (floatEqual(cmdPower[i], 0.0f) || cmdPower[i] < 0.0f)
            {
                manager.computed_value.allocatablePower += -cmdPower[i];
            }
            else
            {
                manager.computed_value.sumError += error[i];
                manager.computed_value.sumPowerRequired += cmdPower[i];
            }
        }
        //电机刚失联
        else if (motorDisconnectCounter[i] < 1000U)
        {
            //使用失联前的力矩电流
            float motorTorque = manager.motors[i]->motor_data_.motor_raw_data.feedback_speed * (i < 4 ? Wheel_TorqueConst : Rudder_TorqueConst);
            float motorAv = rpm2av(manager.motors[i]->motor_data_.motor_raw_data.feedback_speed / (i < 4 ? wheel_radio:rudder_radio));
            cmdPower[i] = motorTorque * motorAv +
                          fabs(motorAv) * manager.k1 +
                          motorTorque * motorTorque * manager.k2 + manager.k3 / static_cast<float>(Wheel_Num);
            error[i] = 0.0f;
        }
        //失联
        else
        {
            cmdPower[i] = 0.0f;
            error[i]    = 0.0f;
        }
    }

    // update power status
    powerStatus.maxPowerLimited          = manager.computed_value.maxPower;
    powerStatus.sumPowerCmd_before_clamp = manager.computed_value.sumCmdPower;

    // 步骤2：判断是否需要限制功率
    if (manager.computed_value.sumCmdPower > manager.computed_value.maxPower)
    {
        // 防止分配总需求功率过小导致除零；此时退回原PID输出，避免NaN传播
        if (manager.computed_value.sumPowerRequired <= EPS)
        {
            for (int i = 0; i < Wheel_Num; i++)
            {
                if (isMotorConnected(manager.motors[i]))
                {
                    newTorqueCurrent[i] = objs[i]->pidOutput;
                }
                else
                {
                    newTorqueCurrent[i] = 0.0f;
                }
            }
            return newTorqueCurrent;
        }

        // 分别计算轮电机和舵电机的功率分配权重
        float wheel_allocatablePower = manager.computed_value.allocatablePower * manager.computed_value.wheel_sumPowerRequired
                                / manager.computed_value.sumPowerRequired;
        float rudder_allocatablePower = manager.computed_value.allocatablePower * manager.computed_value.rudder_sumPowerRequired
                                / manager.computed_value.sumPowerRequired;

        float wheel_errorConfidence;
        // 轮电机误差置信度

        if (manager.computed_value.wheel_sumError > manager.computed_value.wheel_error_powerDistribution_set)
        {
            wheel_errorConfidence = 1.0f;
        }
        else if (manager.computed_value.wheel_sumError > manager.computed_value.wheel_prop_powerDistribution_set)
        {
            wheel_errorConfidence = user_maths_c().user_val_limit(
                (manager.computed_value.wheel_sumError - manager.computed_value.wheel_prop_powerDistribution_set) / (manager.computed_value.wheel_error_powerDistribution_set - manager.computed_value.wheel_prop_powerDistribution_set), 0.0f, 1.0f);
        }
        else
        {
            wheel_errorConfidence = 0.0f;
        }

        float rudder_errorConfidence;
        if (manager.computed_value.rudder_sumError > manager.computed_value.rudder_error_powerDistribution_set)
        {
            rudder_errorConfidence = 1.0f;
        }
        else if (manager.computed_value.rudder_sumError > manager.computed_value.rudder_prop_powerDistribution_set)
        {
            rudder_errorConfidence = user_maths_c().user_val_limit(
                (manager.computed_value.rudder_sumError - manager.computed_value.rudder_prop_powerDistribution_set) / (manager.computed_value.rudder_error_powerDistribution_set - manager.computed_value.rudder_prop_powerDistribution_set), 0.0f, 1.0f);
        }
        else
        {
            rudder_errorConfidence = 0.0f;
        }


        // 遍历每个电机，重新计算允许的力矩
        for (int i = 0; i < Wheel_Num; i++)
        {
            PowerObj *p = objs[i];
            if (isMotorConnected(manager.motors[i]))
            {
                if (floatEqual(cmdPower[i], 0.0f) || cmdPower[i] < 0.0f)
                {
                    newTorqueCurrent[i] = p->pidOutput;
                    continue;
                }

                float powerWeight;
                float current_allocatablePower;
                float current_k0;

                if (i < 4)  // 轮电机
                {
                    const float powerWeight_Error = (manager.computed_value.wheel_sumError > EPS) ?
                        (error[i] / manager.computed_value.wheel_sumError) : 0.0f;
                    const float powerWeight_Prop = (manager.computed_value.wheel_sumPowerRequired > EPS) ?
                        (cmdPower[i] / manager.computed_value.wheel_sumPowerRequired) : 0.0f;
                    powerWeight = wheel_errorConfidence * powerWeight_Error + (1.0f - wheel_errorConfidence) * powerWeight_Prop;
                    current_allocatablePower = wheel_allocatablePower;
                    current_k0 = manager.computed_value.k0;
                }
                else  // 舵电机
                {
                    const float powerWeight_Error = (manager.computed_value.rudder_sumError > EPS) ?
                        (error[i] / manager.computed_value.rudder_sumError) : 0.0f;
                    const float powerWeight_Prop = (manager.computed_value.rudder_sumPowerRequired > EPS) ?
                        (cmdPower[i] / manager.computed_value.rudder_sumPowerRequired) : 0.0f;
                    powerWeight = rudder_errorConfidence * powerWeight_Error + (1.0f - rudder_errorConfidence) * powerWeight_Prop;
                    current_allocatablePower = rudder_allocatablePower;
                    current_k0 = manager.computed_value.rudder_k0;
                }

                // 构造二次方程：k2τ² + ωτ + (k1|ω| + k3/4 - P_allocated) = 0
                float delta = p->curAv * p->curAv -
                              4.0f * manager.k2 * (manager.k1 * fabs(p->curAv) + manager.k3 / static_cast<float>(Wheel_Num) - powerWeight * current_allocatablePower);


                // 选择符号与原始PID输出一致的解
                if (floatEqual(delta, 0.0f))  // repeat roots
                {
                    newTorqueCurrent[i] = -p->curAv / (2.0f * manager.k2) / current_k0;
                }
                else if (delta > 0.0f)  // distinct roots
                {
                    newTorqueCurrent[i] = p->pidOutput > 0.0f ? (-p->curAv + sqrtf(delta)) / (2.0f * manager.k2) / current_k0
                                                              : (-p->curAv - sqrtf(delta)) / (2.0f * manager.k2) / current_k0;
                }
                else  // 虚根情况，取中间值
                {
                    newTorqueCurrent[i] = -p->curAv / (2.0f * manager.k2) / current_k0;
                }
                // 限制力矩不超过PID最大输出
                newTorqueCurrent[i] = user_maths_c().user_val_limit(newTorqueCurrent[i], -p->pidMaxOutput, p->pidMaxOutput);
            }
            else
            {
                newTorqueCurrent[i] = 0.0f;
            }
        }
    }
    else
    {
                // 无需限制，直接使用PID输出
        for (int i = 0; i < Wheel_Num; i++)
        {
            if (isMotorConnected(manager.motors[i]))
            {
                newTorqueCurrent[i] = objs[i]->pidOutput;
            }
            else
            {
                newTorqueCurrent[i] = 0.0f;
            }
        }
    }

    return newTorqueCurrent;
}


/**
 * @brief 使用给定的管理器配置初始化功率控制器
 * @param mana 管理器配置
 */
void init(const Manager &mana)
{
    manager = mana;

    //初始化控制板
    #if CAP_CTRL_TYPE==PM01_CAP
    Superpower_Calc::superpower_instance.Init();
    Superpower_Calc::getinstance()->SetInitInputPower(60);
    manager.power_data.powerUpperLimit           = Cap_Max_Output_Power;
    manager.power_data.min_powerlimit = 33.0f;
    #elif CAP_CTRL_TYPE==OWNER_CAP
    Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->init();
    manager.power_data.powerUpperLimit           = Cap_limit_Max_Output_Power_Owner;
    manager.power_data.min_powerlimit = 33.0f;
    #endif
    manager.power_data.net_change_power = 15;
    manager.power_data.net_rechange_power = -15;

    robotMessage = &Referee_n::referee_c::GetInstance()->referee_data_;

    manager.energy_pid_alg = new alg_n::PID_c(energy_pid_config_);    //能量误差
    manager.energy_controller.max_voltage = 23;     //超级电容最高电压
    manager.energy_controller.min_voltage = 14;       //底盘可以动的最低电容电压
    manager.energy_controller.min_power_limit = 40;    //底盘可以动的最低功率

    manager.computed_value.k0 = Wheel_TorqueConst;
    manager.computed_value.rudder_k0 = Rudder_TorqueConst;
    manager.computed_value.wheel_error_powerDistribution_set = WHEEL_ERROR_POWERDISTRIBUT_SET;
    manager.computed_value.wheel_prop_powerDistribution_set = WHEEL_PROP_POWERDISTRIBUT_SET;
    manager.computed_value.rudder_error_powerDistribution_set = RUDDER_ERROR_POWERDISTRIBUT_SET;
    manager.computed_value.rudder_prop_powerDistribution_set = RUDDER_PROP_POWERDISTRIBUT_SET;

}

//进行RLS数据更新、能连环控制
void Power_Ctrl_Loop()
{
    static Matrixf<2, 1> samples;
    static Matrixf<2, 1> params;
    static float last_extpower = 0;
    
    float motorTorque[4] = {0,0,0,0};
    float motorAv[4] = {0,0,0,0};
    float effectivePower = 0;

    // 判断错误标志
    setErrorFlag();

    // 数据更新
    #if CAP_CTRL_TYPE==PM01_CAP
    manager.Cap_data.cap_vol = Superpower_Calc::getinstance()->pm01_od.v_out/100.0f;
    manager.Cap_data.powerBuff = Cal_Cap_Buff(manager.Cap_data.cap_vol);
    manager.power_data.powerUpperLimit = Cap_limit_Max_Output_Power_Owner;//Cal_Cap_Max_Power(manager.Cap_data.cap_vol);
    powerStatus.change_power           =  Superpower_Calc::getinstance()->pm01_od.p_in/100;
    powerStatus.cap_power              =  Superpower_Calc::getinstance()->pm01_od.p_out/100;
    Superpower_Calc::getinstance()->Set_Chassis_Power_Limit(robotMessage->Robot_Status.chassis_power_limit);
    powerStatus.chassis_energy         =  Superpower_Calc::getinstance()->true_energy;

    #elif CAP_CTRL_TYPE==OWNER_CAP
    manager.Cap_data.cap_vol = Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Vcap;
    manager.Cap_data.powerBuff = Cal_Cap_Buff(manager.Cap_data.cap_vol);
    manager.power_data.powerUpperLimit = Cap_limit_Max_Output_Power_Owner;
    powerStatus.change_power           =  Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Pbat;
    powerStatus.cap_power              =  Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Pcap;
    powerStatus.chassis_energy         =   Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Chassis_energy;

    #endif
    //裁判系统功率限制
    manager.power_data.refereeMaxPower = robotMessage->Robot_Status.chassis_power_limit;
    manager.power_data.power_buff = robotMessage->Power_Heat.buffer_energy;


    //超级电容如果断联，禁用rls更新
    if(isFlagged(manager.error, Manager::CAPDisConnect)) setRLSEnabled(0);
    else setRLSEnabled(1);

    //缓冲能量环设置充电功率
    #if CAP_CTRL_TYPE==PM01_CAP
    Superpower_Calc::getinstance()->Read_and_Write_Power(manager.power_data.power_buff);
    powerStatus.change_power_set = Superpower_Calc::getinstance()->Cap_Input_Power;
    #elif CAP_CTRL_TYPE==OWNER_CAP
    powerStatus.change_power_set = manager.power_data.refereeMaxPower + (manager.power_data.power_buff - 70)/10;
    static uint32_t last_limit_send_tick = 0U;
    static bool limit_send_inited = false;
    const uint32_t now_tick = HAL_GetTick();
    // 最小发送间隔：20ms，避免每次循环都重复发送
    if ((!limit_send_inited) || ((now_tick - last_limit_send_tick) >= 20U))
    {
        Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->Send_Limit_Data(powerStatus.change_power_set);
        last_limit_send_tick = now_tick;
        limit_send_inited = true;
    }
    #endif


    SupperCapPowerLimitUpdate();   //能量环计算


    //能量环设置底盘功率上限
    if(manager.callback == nullptr )
    {
        manager.power_set.Change_mode_Powerlimit = manager.power_data.refereeMaxPower - Cap_Loss_Power - manager.power_data.net_change_power;
        manager.power_set.Rechange_mode_powerlimit = manager.power_data.refereeMaxPower - Cap_Loss_Power - manager.power_data.net_rechange_power;
        manager.power_set.Fly_mode_powerlimit = manager.power_data.powerUpperLimit;

        manager.power_set.SupperCap_powerlimit = manager.power_data.refereeMaxPower + manager.energy_controller.dynamic_power_limit;

        if(manager.power_mode_ == Power_Mode_e::CHARGE_MODE) manager.power_set.Power_limit_set = manager.power_set.Change_mode_Powerlimit;
        else if(manager.power_mode_ == Power_Mode_e::RECHARGE_MODE) manager.power_set.Power_limit_set = manager.power_set.Rechange_mode_powerlimit;
        else if(manager.power_mode_ == Power_Mode_e::FLY_MODE) manager.power_set.Power_limit_set = manager.power_set.Fly_mode_powerlimit;

        else if(manager.power_mode_ == Power_Mode_e::CAPACITOR_MODE_OFF || manager.power_mode_ == Power_Mode_e::CAPACITOR_MODE_ON)
            manager.power_set.Power_limit_set = manager.power_set.SupperCap_powerlimit;

        else manager.power_set.Power_limit_set = manager.power_data.refereeMaxPower - Cap_Loss_Power;
    }
    else //用户自定义功率上限
    {
        manager.power_set.Power_limit_set = manager.callback();
    }

    // Estimate the power based on the current model
    effectivePower = 0;
    samples[0][0]  = 0;
    samples[1][0]  = 0;
    for (int i = 0; i < Wheel_Num; i++)
    {
        if (isMotorConnected(manager.motors[i]))
        {
            motorDisconnectCounter[i] = 0U;
        }
        else
        {
            motorDisconnectCounter[i]++;
        }
        if (motorDisconnectCounter[i] < 1000U)  //我们认为刚刚断开的电机仍在使用电力，假设电机保持
                                                //最新输出和RPM持续1秒，否则只使用能量回路是不安全的
        {
            motorTorque[i] = manager.motors[i]->motor_data_.motor_raw_data.force_feedback * (i < 4 ? Wheel_TorqueConst : Rudder_TorqueConst);
            motorAv[i] = rpm2av(manager.motors[i]->motor_data_.motor_raw_data.feedback_speed / (i < 4 ? wheel_radio:rudder_radio));
            effectivePower += (motorTorque[i] * motorAv[i]);
            samples[0][0] += fabsf(motorAv[i]);
            samples[1][0] += motorTorque[i] * motorTorque[i];
        }
        else
        {
            motorDisconnectCounter[i] = 1000U;
        }
    }

    //模型公式计算单电机功率：P = τω + k1|ω| + k2τ² + k3/4
    last_extpower = manager.Cap_data.estimatedPower;
    manager.Cap_data.estimatedPower = (0.9)*last_extpower + 0.1*(manager.k1 * samples[0][0] + manager.k2 * samples[1][0] + effectivePower + manager.k3);


    // 获取当前功率，有超电的情况下读取超电输出功率，否则使用预测的功率
    #if IS_Connect_Super_Cap

        #if CAP_CTRL_TYPE==PM01_CAP
            if (not isFlagged(manager.error, Manager::CAPDisConnect))
            manager.Cap_data.measuredPower = Superpower_Calc::getinstance()->pm01_od.p_out/100.0f;
            else
            manager.Cap_data.measuredPower = manager.Cap_data.estimatedPower;
        #elif CAP_CTRL_TYPE==OWNER_CAP
            if (not isFlagged(manager.error, Manager::CAPDisConnect))
            manager.Cap_data.measuredPower = Cap_Ctrl_n::Cap_Ctrl_c::getInstance()->cap_data.Chassis_Power;
            else
            manager.Cap_data.measuredPower = manager.Cap_data.estimatedPower;
        #endif

    #else
        manager.Cap_data.measuredPower = manager.Cap_data.estimatedPower;
    #endif

    // update power status
    powerStatus.userConfiguredMaxPower = manager.power_set.Power_limit_set;
    powerStatus.effectivePower         = effectivePower;
    powerStatus.chassis_power          = manager.Cap_data.measuredPower;
    powerStatus.powerLoss              = manager.Cap_data.measuredPower - effectivePower;
    powerStatus.efficiency             = (fabsf(manager.Cap_data.measuredPower) > 1e-6f) ?
                                         user_maths_c().user_val_limit(effectivePower / manager.Cap_data.measuredPower, 0.0f, 1.0f) : 0.0f;
    powerStatus.estimatedCapEnergy     = manager.Cap_data.powerBuff;
    powerStatus.Cap_Vol                = manager.Cap_data.cap_vol;
    powerStatus.k1 = manager.k1;
    powerStatus.k2 = manager.k2;

    // Update the RLS parameters AND
    // Add dead zone AND
    // The Referee System could not detect negative power, leading to failure of real measurement.
    // So use estimated power to evaluate this situtation
    if (manager.rlsEnabled == Manager::Enable && fabs(manager.Cap_data.measuredPower) > 5.0f
            && not(isFlagged(manager.error, Manager::CAPDisConnect) && manager.Cap_data.estimatedPower < 0))
    {
        params     = manager.rls.update(samples, manager.Cap_data.measuredPower - effectivePower - manager.k3);
        manager.k1 = fmax(params[0][0], 1e-5f);  // In case the k1 diverge to negative number
        manager.k2 = fmax(params[1][0], 1e-5f);  // In case the k2 diverge to negative number
    }
}

/*超级电容给的功率上限*/
void SupperCapPowerLimitUpdate()
{
    // 计算能量百分比: (当前电压 - 最低电压) / (最高电压 - 最低电压) * 100
    manager.energy_controller.actual_energy_percent = (getPowerStatus().Cap_Vol - manager.energy_controller.min_voltage)/
                                                (manager.energy_controller.max_voltage-manager.energy_controller.min_voltage) *100.0f;

    manager.energy_controller.actual_energy_percent = user_maths_c().user_val_limit(manager.energy_controller.actual_energy_percent, 0.0f, 100.0f);

    switch (manager.power_mode_) {
    case Power_Mode_e::CAPACITOR_MODE_ON:
        manager.energy_controller.target_energy_percent = CAP_ON_TARGET_ENERGY;  // 开电容模式: 40% 目标能量 (激进，多用电)
        break;
    case Power_Mode_e::CAPACITOR_MODE_OFF:
        manager.energy_controller.target_energy_percent = CAP_OFF_TARGET_ENERGY;  // 关电容模式: 80% 目标能量 (保守，多存电)
        break;
    default:
        manager.energy_controller.target_energy_percent = 100.0f;  // 关电容模式: 80% 目标能量 (保守，多存电)
    }

    manager.energy_controller.dynamic_power_limit = manager.energy_pid_alg->Calc(manager.energy_controller.target_energy_percent,manager.energy_controller.actual_energy_percent);

}





}  // namespace Power
}  // namespace Control
}  // namespace Core

