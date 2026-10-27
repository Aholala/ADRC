#ifndef __POWER_CTRL_HPP__
#define __POWER_CTRL_HPP__

//飞坡超电掉压2V左右，可以放心飞


#include "new_Cap.hpp"
#include "RLS.hpp"
#include "dji_driver.hpp"
#include "Alg_PID.hpp"
#include <vector>
#include "referee.hpp"


#define IS_Connect_Super_Cap 1 //是否连接超级电容

#define PM01_CAP 0    //pm01控制板
#define OWNER_CAP 1   //自研控制板
#define CAP_CTRL_TYPE OWNER_CAP   //请选择你的控制板


#define LEG_WHEEL 0          //腿轮
#define OMNIBEARING_WHEEL 1  //全向轮
#define STEERING_WHEEL 2     //舵轮

//底盘类型
#define CHASSIS_TYPE OMNIBEARING_WHEEL


#define WHEEL_ERROR_POWERDISTRIBUT_SET 20.0f;
#define WHEEL_PROP_POWERDISTRIBUT_SET 15.0f;
#define RUDDER_ERROR_POWERDISTRIBUT_SET 2.0f;
#define RUDDER_PROP_POWERDISTRIBUT_SET 1.5f;


// 功率控制相关参数

#define RLS_K1 0.251f
#define RLS_K2 1.784f
#define RLS_K3 7.03f



namespace Core
{
namespace Control
{
namespace Power
{



//超级电容参数
constexpr static float Cap_Max_Vol = 24.0f;    //超电最大耐压
constexpr static float Cap_Min_Vol = 5.0f;     //超电最低电压
constexpr static float Cap_Allow_Vol = 23.0f;  //超电允许最大电压
constexpr static float Cap_Num = 9.0f;  //电容组数量（个）
constexpr static float A_Cap_Capacity = 60.0f; //单个电容能量（F）
constexpr static float A_Cap_Max_Vol = 2.7f;  //单个电容最大容忍电压（V）


//=== pm01控制板参数 ===
constexpr static float Cap_Max_Output_Power = 120.0f; //最大输出功率（W）
constexpr static float Cap_Max_Output_Current = 6.0f; //最大输出电流（A）
//计算当前pm01最大输出功率 P=UI
#define Cal_Cap_Max_Power(Cap_Power)    ((Cap_Power)*6.0f<120.0f?(Cap_Power)*6.0f:120.0f)

//=== 自研控制板参数 ===
constexpr static float Cap_Max_Output_Power_Owner = 280.0f;       //理论最大输出功率（W）
constexpr static float Cap_limit_Max_Output_Power_Owner = 140.0f; //可飞坡的最低功率限制（W）
constexpr static float Cap_Loss_Power = 2.0f;  //功率损耗

//计算超电当前能量(超电当前电压) 1/2CV^2
#define Cal_Cap_Buff(Cap_Power)         0.5f*A_Cap_Capacity*(Cap_Power)*(Cap_Power)/Cap_Num

#define DJI_M3508_RATIO (3591.0f/187.0f) //3508减速比
// #define DJI_M3508_RATIO (268.0f/17.0f) //3508减速比
#define DJI_M3508_Kt 0.3f  //电机力矩常数
#define DJI_GM6020_Kt 0.741f

//定义轮/舵电机参数
constexpr static float wheel_radio = DJI_M3508_RATIO;
constexpr static float wheel_max_ctrl = 16384.0f;
constexpr static float wheel_max_current = 20.0f;
constexpr static float wheel_Kt = DJI_M3508_Kt;
#define Wheel_TorqueConst wheel_Kt*(wheel_max_current/wheel_max_ctrl)
 
constexpr static float rudder_radio = 1.0f;
constexpr static float rudder_max_ctrl = 16384.0f;
constexpr static float rudder_max_current = 3.0f;
constexpr static float rudder_Kt = DJI_GM6020_Kt;
#define Rudder_TorqueConst rudder_Kt*(rudder_max_current/rudder_max_ctrl)

#define CAP_ON_TARGET_ENERGY  40.0f
#define CAP_OFF_TARGET_ENERGY 80.0f

enum class Power_Mode_e{
    CHARGE_MODE  = 0,  //充电模式
    RECHARGE_MODE,     //补电模式
    FLY_MODE,          //飞坡模式
    USER_MODE,          //用户模式，自定义限制
    CAPACITOR_MODE_OFF,  // 关电容模式
    CAPACITOR_MODE_ON        // 开电容模式
};




enum class Division_e
{
    INFANTRY = 0,
    HERO,
    SENTRY
};



#if CHASSIS_TYPE == LEG_WHEEL
enum Wheel_Index{
    Left_motor = 0,
    Right_motor,
    Wheel_Num
};
#elif CHASSIS_TYPE == OMNIBEARING_WHEEL
enum Wheel_Index{
    MotorRf = 0,
    MotorRb,
    MotorLb,
    MotorLF,
    Wheel_Num
};
#elif CHASSIS_TYPE == STEERING_WHEEL
enum Wheel_Index_e{
    Motor_1 = 0,
    Motor_2,
    Motor_3,
    Motor_4,
    Rudder_1,
    Rudder_2,
    Rudder_3,
    Rudder_4,
    Wheel_Num
};
#endif


/**
 * @brief 功率管理器类，负责协调功率预测、分配和限制
 * @note 包含电机模型参数、RLS算法实例和系统状态
 */
struct Manager
{
    enum RLSEnabled_e : bool
    {
        Disable = 0,
        Enable  = 1
    } rlsEnabled;

    enum ErrorFlags_e
    {
        MotorDisconnect   = 1U,
        RefereeDisConnect = 2U,
        CAPDisConnect     = 4U
    };

    struct Motors  // 用于重载
    {
        const  Motor_n::DjiMotor_n::DjiDriver_c *motor[Wheel_Num];

        const  Motor_n::DjiMotor_n::DjiDriver_c *&operator[](int index)
        {
            return motor[index];
        }

        Motors(std::vector<const  Motor_n::DjiMotor_n::DjiDriver_c *> &motors_)
        {
            for(int i = 0;i < Wheel_Num;i++) { motor[i] = motors_[i]; }
        }

        Motors()
        {
            for(int i = 0;i < Wheel_Num;i++) { motor[i] = nullptr; }
        }
    };

    uint8_t error;

    /**
     * @remark 在没有显式数据的情况下初始化
     */
    Manager() = delete;

    /**
     * @brief 功率管理对象的构造函数
     * @param motors_               电机对象
     * @todo                        这将更改为“AbstractFeedbackMotor*”类型
     * @param division_             机器人类型
     * @param rlsEnabled_           启用或禁用RLS自适应参数模式
     * @param torqueConst_          电机的转矩常数(KA)，测量值为(N.m / A)
     * @param k1_                   功率估算电机上的频率耗散参数
     * @param k2_                   功率估算电机上的电流耗散平方参数
     * @param k3_                   常数功率损耗
     * @param lambda_               RLS更新遗忘因子
     */
    Manager(const Motors &motors_,
            const Division_e division_,
            RLSEnabled_e rlsEnabled_ = Enable,
            const float k1_        = RLS_K1,
            const float k2_        = RLS_K2,
            const float k3_        = RLS_K3,
            const float lambda_    = 0.9999f);

    Motors motors;
    Division_e division;


    float userConfiguredMaxPower; // 用户配置的最大功率 
    float (*callback)(void);


    // 超级电容数据
    struct {
        float cap_vol; // 电容电压
        float powerBuff; // 剩余能量
        float measuredPower; // 当前功率
        float estimatedPower;// 估计功率
    }Cap_data;

    struct {
        float powerUpperLimit;  // 电池最大输出功率上限
        float refereeMaxPower;  // 裁判系统最大功率限制
        float min_powerlimit;   // 最低功率限制（安全阈值）
        float power_buff;       // 缓冲能量
        float net_change_power;   // 净充电功率
        float net_rechange_power; // 净补电功率
    }power_data;

    struct {
        float Change_mode_Powerlimit;   //充电模式限制功率
        float Rechange_mode_powerlimit; //补电模式设置功率
        float Fly_mode_powerlimit;      //飞坡模式限制功率
        float SupperCap_powerlimit;      //超级电容的功率
        float Power_limit_set;          //最终功率限制值
    }power_set ;

    /*能量环新增*/
    struct{
        float min_voltage;  // 最低安全电压(V)
        float max_voltage;  // 最高电压(V)
        float min_power_limit;
        float dynamic_power_limit;     //经能量误差计算得到的功率
        float actual_energy_percent;    //实际能量百分比，计算能量百分比: (当前电压 - 最低电压) / (最高电压 - 最低电压) * 100
        float target_energy_percent;  //目标能量百分比
    }energy_controller;

    /*计算值*/
    struct{
        float k0;
        float rudder_k0;
        float sumCmdPower;
        float sumError;
        float wheel_sumError;      // 轮电机误差总和
        float rudder_sumError;     // 舵电机误差总和
        float wheel_sumPowerRequired;  // 轮电机需求功率总和
        float rudder_sumPowerRequired; // 舵电机需求功率总和
        float maxPower;                     //最大能量
        float allocatablePower;             //可分配的能量
        float sumPowerRequired;             //总需求能量
        float wheel_error_powerDistribution_set;   //轮误差置信度上限
        float wheel_prop_powerDistribution_set;    //轮误差置信度下限
        float rudder_error_powerDistribution_set;  //舵误差置信度上限
        float rudder_prop_powerDistribution_set;   //舵误差置信度下限
    }computed_value;

    alg_n::PID_c *energy_pid_alg;


    // 功率相关参数
    float k1;               // 转速损耗系数（|ω|项）
    float k2;               // 力矩平方损耗系数（τ²项）
    float k3;               // 静态损耗常数

    TickType_t lastUpdateTick;
    Math::RLS<2> rls;       // 递归最小二乘法实例（用于动态更新k1/k2）

    Power_Mode_e power_mode_ = Power_Mode_e::USER_MODE;
    void set_power_mode(Power_Mode_e power_mode) { power_mode_ = power_mode; }
};

struct PowerObj
{
    public:
    float pidOutput;     // 扭矩电流命令, [-maxOutput, maxOutput], 无单位
    float curAv;         // 测量角速度rad/s，需要除以减速比
    float setAv;         // 目标角速度rad/s，需要除以减速比
    float Angle_Error;   // 角度差值
    float pidMaxOutput;  // pid最大输出
};

/**
 * @brief 存储底盘的功率状态
 */
struct PowerStatus
{
   public:
    float userConfiguredMaxPower; // 定义的最大功率限制
    float maxPowerLimited; //最大功率限制
    float sumPowerCmd_before_clamp; // 输出前限幅前的输出
    float effectivePower; // 预测实际输出
    float chassis_power;  //实际输出
    float powerLoss; // 功率损耗
    float efficiency; // 功率效率
    float Cap_Vol;    //电容电压
    float change_power; //电池充电功率
    float cap_power; //电容放电功率
    float change_power_set; //设置的充电功率
    float chassis_energy;  //底盘总消耗J
    float k1;
    float k2;
    float estimatedCapEnergy; // 电池能量估计
    Manager::ErrorFlags_e error; // 错误标志
};


/**
 * @brief 根据当前模型获取受控输出扭矩电流
 * @param objs 四个轮子的功率对象集合，记录PID控制器的必要数据
 * @retval 受控输出扭矩电流
 */
float *getControlledOutput(std::vector<PowerObj *> &objs);

/**
 * @brief 返回底盘的功率状态
 * @retval 功率状态对象
 */
const volatile PowerStatus &getPowerStatus();

/**
 * @brief 功率控制器模块初始化函数
 * @param manager 管理器对象
 * @note 在调度器启动之前应调用此函数
 */
void init(const Manager &manager);

/**
 * @brief 设置用户配置的最大功率
 * @param maxPower 最大功率值
 * @note 此函数配置的最大功率将与基本能量限制竞争，以确保系统不会耗尽
 */
void setMaxPowerConfigured(float maxPower);

/**
 * @brief 设置功率模式
 * @param mode 功率模式
 */
void setPowerMode(Power_Mode_e mode);


void registerPowerCallbackFunc(float (*callback)(void));

/**
 * @brief 启用或禁用自动参数更新过程
 * @param isUpdate 禁用为0，启用为1
 * @note  当裁判系统和电容都与电源模块断开连接时，系统将自动禁用更新
 * @retval 无
 */
void setRLSEnabled(uint8_t isUpdate);

//设置充电、补电功率
void set_change_power(float change_power);
void set_rechange_power(float rechange_power);

void Power_Ctrl_Loop();

/*超级电容给的功率上限*/
void SupperCapPowerLimitUpdate();

}  // namespace Power
}  // namespace Control
}  // namespace Core






#endif
