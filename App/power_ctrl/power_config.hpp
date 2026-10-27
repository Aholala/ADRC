#ifndef POWER_CONFIG_HPP
#define POWER_CONFIG_HPP

/************* 功率模式 *************/
enum class PowerMode_e 
{
    CHARGE_MODE = 0,    // 充电模式
    RECHARGE_MODE,      // 补电模式
    FLY_MODE,           // 飞坡模式
    USER_MODE,          // 用户模式，自定义限制
    CAPACITOR_MODE_OFF, // 关电容模式
    CAPACITOR_MODE_ON   // 开电容模式
};

/*************电容组部分*************/
#ifndef IS_Connect_Super_Cap
#define IS_Connect_Super_Cap 1 //是否连接超级电容
#endif

#ifndef PM01_CAP
#define PM01_CAP 0         // pm01控制板
#endif
#ifndef OWNER_CAP
#define OWNER_CAP 1        // 自研控制板
#endif
#ifndef CAP_CTRL_TYPE
#define CAP_CTRL_TYPE OWNER_CAP // 请选择你的控制板
#endif

//电压百分比映射范围 14~23V
#ifndef MIN_VOLTAGE
#define MIN_VOLTAGE 14.0f
#endif
#ifndef MAX_VOLTAGE
#define MAX_VOLTAGE 23.0f
#endif

//电容数量
#ifndef CAP_NUM
#define CAP_NUM 9.0f
#endif
//单体电容容量
#ifndef SINGLE_CAP_CAPACITY
#define SINGLE_CAP_CAPACITY 60.0f // F
#endif

//自研板功率上限
#ifndef OWNER_POWER_UPPER_LIMIT
#define OWNER_POWER_UPPER_LIMIT 140.0f
#endif

//PM01初始化上限
#ifndef PM01_INIT_POWER_UPPER_LIMIT
#define PM01_INIT_POWER_UPPER_LIMIT 120.0f
#endif
//PM01重新设置时的上限
#ifndef PM01_RUN_POWER_UPPER_LIMIT
#define PM01_RUN_POWER_UPPER_LIMIT 140.0f
#endif
 
//初始化时向PM01的请求
#ifndef PM01_INIT_INPUT_POWER
#define PM01_INIT_INPUT_POWER 60U
#endif


/************* 控制策略 *************/
// 力控底盘统一扭矩缩放
// 0沿原RLS逐轮分配，1用公共系数保持四轮扭矩比例
// 仅改变扭矩分配方式
#ifndef TORQUE_SCALE_ENABLED
#define TORQUE_SCALE_ENABLED 0
#endif

//电容电压预算 PID
//底盘功率预计需要增加/减少多少W
#ifndef ENERGY_PID_KP
#define ENERGY_PID_KP (-1.0f)
#endif
#ifndef ENERGY_PID_KI
#define ENERGY_PID_KI 0.0f
#endif
#ifndef ENERGY_PID_KD
#define ENERGY_PID_KD 1.0f
#endif
#ifndef ENERGY_PID_KFA//前馈1
#define ENERGY_PID_KFA 0.0f
#endif
#ifndef ENERGY_PID_KFB//前馈2
#define ENERGY_PID_KFB 0.0f
#endif
#ifndef ENERGY_PID_MAX_OUT
#define ENERGY_PID_MAX_OUT 35.0f
#endif
#ifndef ENERGY_PID_MAX_IERROR
#define ENERGY_PID_MAX_IERROR 8.0f
#endif
#ifndef ENERGY_PID_DEADBAND//死区
#define ENERGY_PID_DEADBAND 0.0f
#endif

//电压 PID 的目标百分比
#ifndef CAP_ON_TARGET_ENERGY
#define CAP_ON_TARGET_ENERGY 40.0f
#endif
#ifndef CAP_OFF_TARGET_ENERGY
#define CAP_OFF_TARGET_ENERGY 80.0f
#endif
#ifndef OTHER_TARGET_ENERGY
#define OTHER_TARGET_ENERGY 100.0f//其他模式
#endif


//功率预算参数
//充电模式：80 - 2 - 15 = 63 W 
//补电模式：80 - 2 - (-15) = 93 W

#ifndef REFEREE_FALLBACK_POWER
#define REFEREE_FALLBACK_POWER 45.0f //无裁判系统基础功率
#endif
//无超电但裁判在线时仍使用裁判基础功率

#ifndef USER_MIN_POWER
#define USER_MIN_POWER 33.0f // 用户功率设置的下限
#endif
#ifndef LOSS_RESERVE_POWER
#define LOSS_RESERVE_POWER 2.0f // 预留的损耗 可能因为各种原因
#endif
#ifndef CHARGE_RESERVE_POWER
#define CHARGE_RESERVE_POWER 15.0f // 充电模式预留15w
#endif
#ifndef RECHARGE_RESERVE_POWER
#define RECHARGE_RESERVE_POWER (-15.0f) // 补电多加15w
#endif

/************* 底盘选择与电机RLS模型 *************/

#ifndef TWO_WHEEL
#define TWO_WHEEL  0 //2轮
#endif
#ifndef FOUR_WHEEL
#define FOUR_WHEEL 1 //4轮
#endif
//舵不计入功率了 再见了8电机 xixi

#ifndef POWER_CTRL_CHASSIS_TYPE
#define POWER_CTRL_CHASSIS_TYPE FOUR_WHEEL // 请选择你de底盘类型
#endif

//四轮速度误差总和的置信度上下阈值
#ifndef WHEEL_ERROR_POWERDISTRIBUT_SET
#define WHEEL_ERROR_POWERDISTRIBUT_SET 20.0f
#endif
#ifndef WHEEL_PROP_POWERDISTRIBUT_SET
#define WHEEL_PROP_POWERDISTRIBUT_SET 15.0f
#endif
// #define RUDDER_ERROR_POWERDISTRIBUT_SET 2.0f;
// #define RUDDER_PROP_POWERDISTRIBUT_SET 1.5f;

//功率控制相关参数
//P_i = τ_i*ω_i + k1*abs(ω_i) + k2*τ_i² + k3/4。
#ifndef RLS_K1
#define RLS_K1 0.251f
#endif
#ifndef RLS_K2
#define RLS_K2 1.784f
#endif
#ifndef RLS_K3
#define RLS_K3 7.03f
#endif

//RLS 协方差矩阵的初始化尺度
#ifndef RLS_DELTA
#define RLS_DELTA 1e-5f
#endif
#ifndef RLS_LAMBDA
#define RLS_LAMBDA 0.99999f
#endif

//RLS开关 默认打开
#ifndef RLS_ENABLED
#define RLS_ENABLED 1
#endif

#if POWER_CTRL_CHASSIS_TYPE == TWO_WHEEL
    enum Wheel_Index 
    {
        MotorL = 0,
        MotorR,
        Wheel_Num
    };
#elif POWER_CTRL_CHASSIS_TYPE == FOUR_WHEEL
    enum Wheel_Index 
    {
        MotorRf = 0,
        MotorRb,
        MotorLb,
        MotorLF,
        Wheel_Num
    };
#endif

#endif
