#ifndef POWER_CTRL_HPP
#define POWER_CTRL_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Alg_PID.hpp"
#include "RLS.hpp"
#include "motor_base_def.hpp"

#include "power_config.hpp"

namespace Cap_Ctrl_n { class Cap_Ctrl_c; }
#if IS_Connect_Super_Cap && CAP_CTRL_TYPE == OWNER_CAP
#include "new_Cap.hpp"
#endif
#ifdef USE_REFEREE
#include "referee.hpp"
#endif

class Superpower_Calc;
/************* 电机绑定 *************/
struct Motors 
{
    static constexpr std::size_t Capacity = 8;
    static constexpr std::size_t WheelCount = 4;

    const Motor_n::MotorBaseDef_n::MotorBase_c *motor[Capacity] = {};

    const float *target_speed[Capacity] = {};
    float feedback_torque_scale[Capacity] = {};
    float feedback_speed_scale[Capacity] = {};

    //轴侧扭矩绝对值上限，N·m，正值
    float max_torque[Capacity] = {};

    Motors() = default;

    explicit Motors(const std::vector<const Motor_n::MotorBaseDef_n::MotorBase_c *> &motors) 
    {
        const std::size_t count = motors.size() < Capacity ? motors.size() : Capacity;
        for (std::size_t i = 0; i < count; i++) 
        {
        motor[i] = motors[i];
        }
    }

    const Motor_n::MotorBaseDef_n::MotorBase_c *&operator[](std::size_t index) 
    {
        return motor[index];
    }

    const Motor_n::MotorBaseDef_n::MotorBase_c *
    operator[](std::size_t index) const 
    {
        return motor[index];
    }
};

/************* 电压预算状态 *************/
struct EnergyController_t 
{
    float min_voltage = 0.0f; // 最低安全电压(V)
    float max_voltage = 0.0f; // 最高电压(V)

    //计算能量百分比：
    //(当前电压 - 最低电压) / (最高电压 - 最低电压) * 100
    float actual_energy_percent = 0.0f;
    float target_energy_percent = 0.0f;

    //计算得到的功率W
    float dynamic_power_limit = 0.0f;
};

/************* 功率状态 *************/

//存储底盘的功率状态
struct PowerStatus 
{
    bool initialized = false;

    bool is_cap_online = false;
    bool is_referee_online = false;
    bool is_all_motor_online = false;

    bool rls_enabled = RLS_ENABLED;
    bool rls_updated = false;

    float userConfiguredMaxPower = 0.0f;
    float basePowerLimit = 0.0f;
    float candidatePowerLimit = 0.0f;
    //最大功率限制W。
    float maxPowerLimited = 0.0f;

    //原始目标扭矩对应的预测功率 W
    float sumPowerCmd_before_clamp = 0.0f;

    //最终输出扭矩对应的预测功率 W
    float predictedPowerAfterLimit = 0.0f;

    //明确为反馈扭矩 速度计算的机械功率，W
    float effectivePower = 0.0f;

    //根据反馈和当前模型计算的滤波预测功率，W
    float estimatedPower = 0.0f;

    //实际输出
    float chassis_power = 0.0f;
    bool measuredPowerValid = false;

    float powerLoss = 0.0f;  //功率损耗，W
    float efficiency = 0.0f; //功率效率

    float Cap_Vol = 0.0f; //电容电压，V
    float estimatedCapEnergy = 0.0f; //电容能量估计，J

    //充电功率、电容功率、输入功率请求，W
    float charge_power = 0.0f;
    float cap_power = 0.0f;
    float charge_power_set = 0.0f;

    float chassis_energy = 0.0f; //底盘总消耗，J
                                
    //电机耗电模型系数 用于估计功率
    //P_i = τ_i·ω_i + k1·|ω_i| + k2·τ_i² + k3/4
    float k1 = 0.0f; //转速损耗系数（|ω|项）
    float k2 = 0.0f; //力矩平方损耗系数（τ²项）
    float k3 = 0.0f; //静态损耗常数

    bool powerLimitUnreachable = false;
};

/************* 功率控制 *************/
class PowerCtrl_c 
{
    public:
    void Init(Motors &motors);//初始化

    void Power_Ctrl_Loop();//功率控制主循环

    //功率分配函数：传入四路目标扭矩N·m、当前角速度rad/s
    float *Calc(std::vector<float *> &raw_output,
                std::vector<float *> &current_speed);

    //功率模式设置
    void SetPowerMode(PowerMode_e mode);

    //设置用户配置最大功率
    void SetMaxPowerConfigured(float power);

    //自动参数更新开关
    void SetRLSEnabled(bool enabled);

    //设置净充电、净补电功率，W
    void SetChangePower(float power);
    void SetRechangePower(float power);

    //注册功率控制的回调函数
    void RegisterPowerCallbackFunc(float (*callback)(void));

    //底盘功率状态
    const PowerStatus &GetPowerStatus() const;
    
    //获取实例
    static PowerCtrl_c *GetInstance() 
    {
        static PowerCtrl_c power_ctrl_instance;
        return &power_ctrl_instance;
    }

    private:

    PowerCtrl_c() = default;
    PowerCtrl_c(const PowerCtrl_c &) = delete;
    PowerCtrl_c &operator=(const PowerCtrl_c &) = delete;

    //电容和裁判系统在线处理
    void ReAndCapProcess();

    //超级电容给的功率上限。
    //(当前电压 - 最低电压) / (最高电压 - 最低电压) * 100
    void SupperCapPowerLimitUpdate();

    //本次允许使用的功率
    void UpdatePowerBudget();
    //预测当前功耗更新模型
    void UpdateModel();

    //模型公式计算单电机功率：P = τω + k1|ω| + k2τ² + k3/4
    float MotorPowerFromModel(float torque, float speed_rad_s);

    //按原目标扭矩正负选择加根或减根
    //无实根时取二次函数顶点对应的扭矩
    float SolveTorqueForTargetPower(float torque_origin, float speed_rad_s, float target_power);

    Motors motors_;
    PowerStatus power_status_;
    EnergyController_t energy_controller_;

    PowerMode_e power_mode_ = PowerMode_e::USER_MODE;

    alg_n::PID_c *energy_pid_alg_ = nullptr;

#if IS_Connect_Super_Cap && CAP_CTRL_TYPE == OWNER_CAP
    Cap_Ctrl_n::Cap_Ctrl_c *power_cap_data_ = nullptr;
#elif IS_Connect_Super_Cap && CAP_CTRL_TYPE == PM01_CAP
    Superpower_Calc *pm01_ = nullptr;
#endif

#ifdef USE_REFEREE
    Referee_n::RefereeData_t *power_referee_data_ = nullptr;
#endif

    //递归最小二乘法实例 动态更新k1/k2
    Core::Control::Math::RLS<2> rls_{RLS_DELTA, RLS_LAMBDA};

    float (*power_callback_)(void) = nullptr;

    struct 
    {
        float fallback_power = 0.0f;    //裁判失联基础功率，W
        float power_upper_limit = 0.0f; //最大输出功率上限
        float min_power_limit = 0.0f;   //最低功率限制
        float cap_loss_power = 0.0f;    //功率损耗

        float net_change_power = 0.0f;   //净充电功率
        float net_rechange_power = 0.0f; //净补电功率

        float wheel_error_power_distribution = 0.0f; // 原轮误差置信度上限
        float wheel_prop_power_distribution = 0.0f;  // 原轮误差置信度下限
    } power_data_;

    //motor_disconnect_counter_
    float torque_out_[Motors::WheelCount] = {};
    std::uint16_t motor_disconnect_counter_[Motors::WheelCount] = {};

    //limit_send_inited
#if IS_Connect_Super_Cap && CAP_CTRL_TYPE == OWNER_CAP
    std::uint32_t last_limit_send_tick_ = 0;
    bool limit_send_initialized_ = false;
#endif
};

#endif
