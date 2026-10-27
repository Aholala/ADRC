/************************** Dongguan-University of Technology -ACE**************************
* @file  motor_base_def.hpp
* @brief 电机基类
* @author 胡炜 coconutjuice
* @version v26.0.7.6
* @date 2025-11-28
* @note
    电机基类定义了电机通用属性和接口，供具体电机继承并实现，该封装使得电机代码的复用性更高。
    继承该基类后去实现具体的电机驱动非常简单，你甚至只需要着重实现解析函数和发送函数！
    修改can回调为分发器机制！不再需要链表轮询寻找电机实例！将数据精确分配到对应电机的数据结构中！

    该文件同时也作为代码规范的参考文件，对各种命名及格式进行规范化。
    为方便区分，使用 //为代码注释，使用/*为格式注释
*
* ==============================================================================
* @verbatim
* 
* @attention
*      依赖文件：
        CAN  
        DWT
        safe_task
* @version           time
* v26.0.0.0   全新的体验    2025-11-28
* v26.0.4.1   修复DWT计时错误的问题 2025-12-02
* v26.0.5.1   需求性更改，达妙电机掉线之后不再发送失能帧，达妙电机mit自锁函数，达妙电机新增数据单圈相对角度归一化到-180~180
* v26.0.5.5   H7的C++类初始化，成员变量如果没有手动清，可能会保存以前下电的值
* v26.0.5.6   H7的C++类初始化(二)，成员变量如果没有手动清，可能会保存以前下电的值
* v26.0.6.6   修改了线速度和线位置相关逻辑;电机的滤波参数做了接口传入;修复了达妙的total_angle的零点是上电位置作为零点这一问题
* v26.0.7.6   修改了达妙原始反馈值单位，feedback_speed由RPM->rad/s
* v26.0.8.0   适配灯语
************************** Dongguan-University of Technology -ACE***************************/

/* 防止重复定义，格式：__大写文件名_HPP */
#ifndef __MOTOR_BASE_DEF_HPP
#define __MOTOR_BASE_DEF_HPP

/* region 头文件包含 */
#include <cstring>
#include <string>
#include <functional>
#include <cstdint>
#include <cstdio>
#include "Alg_PID.hpp"
#include "bsp_dwt.hpp"
#include "safe_task.hpp"
#include "ecf_configList.h"
#if defined(USE_JLINK_RTT)
#include "bsp_jlink_rtt.hpp"
#endif

#ifdef USE_DEBUG_LIGHT
#include "debug_light.hpp"
#endif

#ifdef STM32H723xx
#include "bsp_fdcan.hpp"
#else
#include "bsp_can.hpp"
#endif // USE_H7_if_or_not
/* endregion */

/* region 宏定义 */
#ifndef PI
#define PI 3.1415926535f
#endif

#ifndef PI2
#define PI2 (PI * 2.0f) // 2 pi
#endif

#ifndef RAD_2_DEGREE
#define RAD_2_DEGREE 57.2957795f    // 180/pi
#define DEGREE_2_RAD 0.01745329252f // pi/180
#endif

#ifndef RPM_2_ANGLE_PER_SEC
#define RPM_2_ANGLE_PER_SEC 6.0f       // ×360°/60sec
#define RPM_2_RAD_PER_SEC 0.104719755f // ×2pi/60sec
#endif

#define DJI_MOTOR_CNT 12

/* 滤波系数设置为1的时候即关闭滤波 */
#define SPEED_SMOOTH_COEF 0.85f      // 最好大于0.85 速度滤波系数
#define CURRENT_SMOOTH_COEF 0.9f     // 必须大于0.9  电流滤波系数
#define RPM_2_ANGLE_PER_SEC 6.0f       // ×360°/60sec 角速度转换
// 大疆电机自定义限幅
#define Voltage_limit_H_6020 25000
#define Voltage_limit_L_6020 -25000
#define Current_limit_H_3508 16000
#define Current_limit_L_3508 -16000
#define Current_limit_H_2006 10000
#define Current_limit_L_2006 -10000

#if defined(USE_JLINK_RTT) && defined(ECF_RTT_LOG_ENABLE_MOTOR) && (ECF_RTT_LOG_ENABLE_MOTOR)
#define MOTOR_LOG_I(fmt, ...) LOG_RTT_I("[MOTOR][SAFE] " fmt __VA_OPT__(,) __VA_ARGS__)
#define MOTOR_LOG_E(fmt, ...) LOG_RTT_E("[MOTOR][SAFE] " fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define MOTOR_LOG_I(fmt, ...) ((void)0)
#define MOTOR_LOG_E(fmt, ...) ((void)0)
#endif

//! 根據不同電機的不同上位機調整
// DM6220默认參數範圍
#define DM6220_P_MIN (-12.5f)
#define DM6220_P_MAX 12.5f
#define DM6220_V_MIN (-45.0f)
#define DM6220_V_MAX 45.0f
#define DM6220_T_MIN (-10.0f)
#define DM6220_T_MAX 10.0f
// 这些参数一般不变
#define DM_KP_MIN 0 
#define DM_KP_MAX 500.0f
#define DM_KD_MIN 0
#define DM_KD_MAX 5.0f
#define DM4310_RATIO 10 // 减速比
#define DM4340_RATIO 40
#define DM6220_RATIO 1 
#define DM8009_RATIO 9
#define DM10010_RATIO 10
#define DM2325_RATIO 25
#define DM3519_RATIO 19.2
// 电机峰值扭矩
#define DM6220_T_PEAK_MAX 2.7f // V1.0版本
#define DM4310_1_1_T_PEAK_MAX 7.0f // V1.1版本
#define DM4310_1_2_T_PEAK_MAX 11.0f // V1.2版本（0.8过流）
#define DM4310_1_2_T_PRO_PEAK_MAX 12.5f // V1.2版本（0.98过流）
#define DM4340_1_0_T_PEAK_MAX 27.0f // V1.0版本
#define DM4340_1_1_T_PEAK_MAX 40.0f // V1.1版本
#define DM8009_T_PEAK_MAX 40.0f // V1.0版本
#define DM10010_T_PEAK_MAX 150.0f // V1.0版本
#define DM2325_T_PEAK_MAX 5.0f // V1.0版本
/* endregion */

/* 命名空间设置，顶格无缩进 */
/**
 * 使用命名空间的好处：1、C代码兼容，引用C库时将C函数封装在特定命名空间
 *                   2、防止命名冲突
 *                   3、在没有命名冲突的情况下仍然可直接访问
 *                   4、使用匿名命名空间可以代替静态全局变量
 *
 * 注意：不要在头文件使用using namespace 语句，会导致引入的标识符在所有包含该头文件的代码中可见，增加冲突风险。
 */
namespace Motor_n
{
    static user_maths_c motor_maths; // 数学工具类实例
    /* 两个用于将uint值和float值进行映射的函数,在设定发送值和解析反馈值时使用 */
    /**
     * @brief 将浮点值映射为给定位宽的无符数，用于MIT协议发送
     */
    inline uint16_t float_to_uint(float X_float, float X_min, float X_max, int bits)
    {
        float span = X_max - X_min;
        float offset = X_min;
        return static_cast<uint16_t>((X_float-offset)*((float)((1<<bits)-1))/span);
    }

    /**
     * @brief 将编码值转为浮点量，便于物理量解析
     */
    inline float uint_to_float(int x_int, float x_min, float x_max, int bits)
    {
        float span = x_max - x_min;
        float offset = x_min;
        return static_cast<float>(x_int) * span / ((1 << bits) - 1) + offset;
    }

    namespace MotorBaseDef_n /* 模块内部类统一 */
    {
        // 标志位判断
        /**
         * @brief 判断标志位是否已置位，决定启用哪个闭环
         */
        static inline bool isUseFlag(uint8_t flag_setting, uint8_t flag) { return (flag_setting & flag) != 0; }// 判断flag是否被设置
        static inline void ClearFlag(uint8_t &flag_setting, uint8_t flag) { flag_setting &= ~flag; }// 清除flag
        static inline void SetFlag(uint8_t &flag_setting, uint8_t flag) { flag_setting |= flag; } // 设置flag

        /* 枚举定义，以驼峰+下划线命名，_ex结尾
         *  x为指定底层数据类型，如eb(bool 1bit)、euc(unsigned char 8bit)、eui(unsigned int 32bit)。
         *  枚举成员使用大写+下划线区分
         *
         *  枚举分为强制类型枚举（enum class）和非强制类型枚举（enum）
         *  字段枚举使用非强制类型枚举，其他统一使用强制类型枚举
         */
        // 电机正反转标志
        typedef enum class Motor_Reverse_Flag_e : bool
        {
            MOTOR_DIRECTION_NORMAL = 0,  // 正转
            MOTOR_DIRECTION_REVERSE = 1  // 反转
        } Motor_Reverse_Flag_eb;

        typedef enum class Motor_Stuck_Status_e : bool
        {
            MOTOR_NOT_STUCK = 0, // 未堵转
            MOTOR_IS_STUCK = 1  // 堵转
        } Motor_Stuck_Status_eb;

        // 驱动方式
        typedef enum class Motor_Driver_Way_e : bool
        {
            CURRENT_DRIVE = 0, // 发送电流驱动，如Dji电机
            MIT_DRIVE = 1,     // MIT控制，其他关节电机
        } Motor_Driver_Way_eb;

        // 电机状态枚举
        typedef enum class Motor_Working_Status_e : unsigned char
        {
            MOTOR_STOP = 0,
            MOTOR_ENALBED = 1,
            MOTOR_ONLY_RECEIVE = 2,   // 只用于接收
            MOTOR_SELF_LOCK = 3,      // 电机自锁
            MOTOR_OUTPUT_ONLY_ME = 4, // 发送自己设定的值，不适用PID的计算值
        } Motor_Working_Status_euc;

        // 电机类型枚举
        typedef enum class Motor_Type_e : unsigned char
        {
            MOTOR_TYPE_NONE = 0,
            GM6020,
            M3508,
            M3508NEW,
            M2006,
            DM4310, //
            DM4340, //
            DM6220, //
            DM8009, //  这几个电机协议相同
            DM10010,//
            DM2325, //
            DM3519, //
            CYBERGEAR
        } Motor_Type_euc;

        //===============闭环部分==================
        typedef enum : unsigned char
        {
            OPEN_LOOP = 0b0000,    // 开环
            CURRENT_LOOP = 0b0001, // 电流闭环
            SPEED_LOOP = 0b0010,   // 速度闭环
            ANGLE_LOOP = 0b0100,   // 角度闭环

            // only for checking
            SPEED_AND_CURRENT_LOOP = 0b0011, // 速度和电流闭环
            ANGLE_AND_SPEED_LOOP = 0b0110,   // 角度和速度闭环
            ALL_THREE_LOOP = 0b0111,         // 角度，速度，电流三闭环
        } Closeloop_Type_euc;

        // 反馈来源设定,若设为OTHER_FEED则需要指定数据来源指针
        typedef enum class Motor_Feedback_Source_e : bool
        {
            MOTOR_FEED = 0,
            OTHER_FEED = 1,
        } Motor_Feedback_Source_eb;

        // 反馈量/前馈量正反标志
        typedef enum class Motor_Feed_Reverse_Flag_e : bool
        {
            FEEDBACK_DIRECTION_NORMAL = 0,
            FEEDBACK_DIRECTION_REVERSE = 1
        } Motor_Feed_Reverse_Flag_eb;

        // 前馈类型设置
        typedef enum : unsigned char
        {
            FEEDFORWARD_NONE = 0b0000,
            CURRENT_FEEDFORWARD = 0b0001,
            SPEED_FEEDFORWARD = 0b0010,
            CURRENT_AND_SPEED_FEEDFORWARD = CURRENT_FEEDFORWARD | SPEED_FEEDFORWARD,
        } Feedfoward_Type_euc;

        /* 结构体通过构造函数赋予默认值 */
        /* 结构体定义，以驼峰+下划线命名，_t结尾 */
        // 电机控制设置,包括闭环类型,反转标志和反馈来源
        typedef struct Motor_Control_Setting_t
        {
            uint8_t close_loop_type;                  // 使用的闭环(串级)
            Motor_Reverse_Flag_eb motor_reverse_flag;            // 是否反转
            Motor_Feedback_Source_eb angle_feedback_source;      // 角度反馈类型
            Motor_Feedback_Source_eb speed_feedback_source;      // 速度反馈类型
            Feedfoward_Type_euc feedforward_flag;                // 前馈标志
            Motor_Feed_Reverse_Flag_eb feedback_reverse_flag;    // 反馈是否反向
            Motor_Feed_Reverse_Flag_eb feedforward_reverse_flag; // 前馈是否反向

            // 电机配置器
            Motor_Control_Setting_t(
                Closeloop_Type_euc close_loop_type_ = Closeloop_Type_euc::OPEN_LOOP,
                Motor_Reverse_Flag_eb motor_reverse_flag_ = Motor_Reverse_Flag_eb::MOTOR_DIRECTION_NORMAL,
                Motor_Feedback_Source_eb angle_feedback_source_ = Motor_Feedback_Source_eb::MOTOR_FEED,
                Motor_Feedback_Source_eb speed_feedback_source_ = Motor_Feedback_Source_eb::MOTOR_FEED,
                Feedfoward_Type_euc feedforward_flag_ = SPEED_FEEDFORWARD,
                Motor_Feed_Reverse_Flag_eb feedback_reverse_flag_ = Motor_Feed_Reverse_Flag_eb::FEEDBACK_DIRECTION_NORMAL,
                Motor_Feed_Reverse_Flag_eb feedforward_reverse_flag_ = Motor_Feed_Reverse_Flag_eb::FEEDBACK_DIRECTION_NORMAL)
            {
                close_loop_type = close_loop_type_;
                motor_reverse_flag = motor_reverse_flag_;
                angle_feedback_source = angle_feedback_source_;
                speed_feedback_source = speed_feedback_source_;
                feedforward_flag = feedforward_flag_;
                feedback_reverse_flag = feedback_reverse_flag_;
                feedforward_reverse_flag = feedforward_reverse_flag_;
            }

            /**
             * @brief 调整当前启用的闭环组合
             */
            void SetCloseLoopType(Closeloop_Type_euc type) { close_loop_type = type; }

        } Motor_Control_Setting_t;

        // PID控制器
        typedef struct Motor_Controller_t
        {
            Motor_Control_Setting_t motor_setting;

            alg_n::PID_c *current_PID = nullptr;
            alg_n::PID_c *speed_PID = nullptr;
            alg_n::PID_c *angle_PID = nullptr;

            const float *motor_angle_feedback_ptr = nullptr;
            const float *motor_speed_feedback_ptr = nullptr;
            const float *motor_current_feedback_ptr = nullptr;

            const float *other_angle_feedback_ptr = nullptr; // 其他反馈来源的反馈数据指针
            const float *other_speed_feedback_ptr = nullptr;
            float *speed_feedforward_ptr = nullptr; // 前馈数据指针(速度)
            float *current_feedforward_ptr = nullptr; // 前馈数据指针(电流)
            int16_t *current_go_for_goal_shot_ptr = nullptr; // 临门一脚电流补偿

            float max_output = 0, min_output = 0;
            float pid_ref = 0, pid_output = 0;
            Motor_Controller_t(Motor_Control_Setting_t motor_setting, alg_n::PidInitConfig_t *angle_pid_config,
                               alg_n::PidInitConfig_t *speed_pid_config, alg_n::PidInitConfig_t *current_pid_config,float max_output,float min_output) : motor_setting(motor_setting), max_output(max_output), min_output(min_output), angle_PID(nullptr), speed_PID(nullptr), current_PID(nullptr),
                                                                                                                                   motor_angle_feedback_ptr(nullptr), motor_current_feedback_ptr(nullptr), motor_speed_feedback_ptr(nullptr),
                                                                                                                                   other_angle_feedback_ptr(nullptr), other_speed_feedback_ptr(nullptr), speed_feedforward_ptr(nullptr), current_feedforward_ptr(nullptr)
            {
                // 仅当参数非空时，才创建 PID 实例
                SetAnglePIDConfig(*angle_pid_config);
                SetSpeedPIDConfig(*speed_pid_config);
                SetCurrentPIDConfig(*current_pid_config);

            }

            // PID计算
            /**
             * @brief 三环PID串行计算控制量
             */
            float MotorPIDCalculate(float *SetVal)
            {
                pid_ref = *SetVal;
                const int feedback_reverse_flag = motor_setting.feedback_reverse_flag == Motor_Feed_Reverse_Flag_eb::FEEDBACK_DIRECTION_REVERSE ? -1 : 1;
                const int feedforward_reverse_flag = motor_setting.feedforward_reverse_flag == Motor_Feed_Reverse_Flag_eb::FEEDBACK_DIRECTION_REVERSE ? -1 : 1;
                if (motor_setting.motor_reverse_flag == Motor_Reverse_Flag_eb::MOTOR_DIRECTION_REVERSE)
                    pid_ref *= -1;
                // 角度环
                if (isUseFlag(motor_setting.close_loop_type, Closeloop_Type_euc::ANGLE_LOOP))
                {
                    if (motor_setting.angle_feedback_source == Motor_Feedback_Source_eb::MOTOR_FEED && motor_angle_feedback_ptr != nullptr)
                        pid_ref = angle_PID->Calc(pid_ref, *motor_angle_feedback_ptr * feedback_reverse_flag);
                    else if (motor_setting.angle_feedback_source == Motor_Feedback_Source_eb::OTHER_FEED && other_angle_feedback_ptr != nullptr)
                        pid_ref = angle_PID->Calc(pid_ref, *other_angle_feedback_ptr * feedback_reverse_flag);
                }
                // 速度环
                if (isUseFlag(motor_setting.feedforward_flag, Feedfoward_Type_euc::SPEED_FEEDFORWARD) && speed_feedforward_ptr != nullptr)
                    pid_ref += *speed_feedforward_ptr * feedforward_reverse_flag;
                if (isUseFlag(motor_setting.close_loop_type, Closeloop_Type_euc::SPEED_LOOP))
                {
                    if (motor_setting.speed_feedback_source == Motor_Feedback_Source_eb::MOTOR_FEED && motor_speed_feedback_ptr != nullptr)
                        pid_ref = speed_PID->Calc(pid_ref, *motor_speed_feedback_ptr * feedback_reverse_flag);
                    else if (motor_setting.speed_feedback_source == Motor_Feedback_Source_eb::OTHER_FEED && other_speed_feedback_ptr != nullptr)
                        pid_ref = speed_PID->Calc(pid_ref, *other_speed_feedback_ptr * feedback_reverse_flag);
                }
                // 电流环
                if (isUseFlag(motor_setting.feedforward_flag, Feedfoward_Type_euc::CURRENT_FEEDFORWARD && current_feedforward_ptr != nullptr))
                    pid_ref += *current_feedforward_ptr * feedforward_reverse_flag;
                if (isUseFlag(motor_setting.close_loop_type, Closeloop_Type_euc::CURRENT_LOOP) && motor_current_feedback_ptr != nullptr)
                    pid_ref = current_PID->Calc(pid_ref, *motor_current_feedback_ptr);
                // 开环
                if (motor_setting.close_loop_type == Closeloop_Type_euc::OPEN_LOOP)
                {
                    // 开环时直接输出设定值
                    pid_ref = *SetVal;
                }
                // 注入临门一脚电流
                if (current_go_for_goal_shot_ptr != nullptr) pid_ref += *current_go_for_goal_shot_ptr;
                // 限幅
                if(pid_ref > max_output)
                    pid_ref = max_output;
                else if(pid_ref < min_output)
                    pid_ref = min_output;
                pid_output = pid_ref;
                return pid_output;
            }
            /**
             * @brief 获取近期PID输出
             */
            const float &GetPidOutput() { return pid_output; }
            void SetCloseLoopType(Closeloop_Type_euc type) { motor_setting.close_loop_type = type; }
            /**
             * @brief 指定角度反馈数据来源
             */
            void SetAngleFeedbackPtr(const float *ptr) { motor_angle_feedback_ptr = ptr; }
            /**
             * @brief 指定速度反馈数据来源
             */
            void SetSpeedFeedbackPtr(const float *ptr) { motor_speed_feedback_ptr = ptr; }
            /**
             * @brief 指定电流反馈数据来源
             */
            void SetCurrentFeedbackPtr(const float *ptr) { motor_current_feedback_ptr = ptr; }
            /**
             * @brief 使用外部角度信息
             */
            void SetOtherAngleFeedbackPtr(const float *ptr) { other_angle_feedback_ptr = ptr; }
            /**
             * @brief 使用外部速度信息
             */
            void SetOtherSpeedFeedbackPtr(const float *ptr) { other_speed_feedback_ptr = ptr; }
            /**
             * @brief 提供速度前馈
             */
            void SetSpeedFeedforwardPtr(float *ptr) { speed_feedforward_ptr = ptr; }
            /**
             * @brief 提供电流前馈
             */
            void SetCurrentFeedforwardPtr(float *ptr) { current_feedforward_ptr = ptr; }
            /**
             * @brief 提供临门一脚电流补偿
             */
            void SetCurrentGoForGoalShotPtr(int16_t *ptr) { current_go_for_goal_shot_ptr = ptr; }
            /**
             * @brief 通过引用修改闭环类型
             */
            void SetCloseLoopType(Closeloop_Type_euc &type) { motor_setting.close_loop_type = type; }
            /**
             * @brief 更换角度PID实例
             */
            void ChangeAnglePIDInstance(alg_n::PID_c *instance) { angle_PID = instance; }
            /**
             * @brief 更换速度PID实例
             */
            void ChangeSpeedPIDInstance(alg_n::PID_c *instance) { speed_PID = instance; }
            // void ChangeCurrentPIDInstance(alg_n::PID_c* instance) { current_PID = instance; }
            /**
             * @brief 重设角度PID参数
             */
            void SetAnglePIDConfig(alg_n::PidInitConfig_t config)
            {
                if (angle_PID != nullptr)
                    angle_PID->Init(config);
                else
                    angle_PID = new alg_n::PID_c(config);
            }
            /**
             * @brief 重设速度PID参数
             */
            void SetSpeedPIDConfig(alg_n::PidInitConfig_t config)
            {
                if (speed_PID != nullptr)
                    speed_PID->Init(config);
                else
                    speed_PID = new alg_n::PID_c(config);
            }
            /**
             * @brief 重设电流PID参数
             */
            void SetCurrentPIDConfig(alg_n::PidInitConfig_t config)
            {
                if (current_PID != nullptr)
                    current_PID->Init(config);
                else
                    current_PID = new alg_n::PID_c(config);
            }

        } Motor_Controller_t;

        typedef struct
        {
            float pos = 0;
            float vel = 0;
            float kp = 0;
            float kd = 0;
            float torq = 0;
        } Motor_MIT_Contorl_t;

        typedef struct
        {
            uint16_t position_des;
            uint16_t velocity_des;
            uint16_t torque_des;
            uint16_t Kp;
            uint16_t Kd;
        } Motor_Send_t;

        // 电机数据结构体，通过回调函数设置并处理
        typedef struct Motor_Data_t
        {
            // 电机原始反馈值结构体，(大疆电机返回的都是电机轴数据，达妙电机返回的数据是输出轴数据)
            struct Motor_Raw_Feedback_t
            {
                float feedback_speed = 0;      //速度反馈，DJI单位为RPM，DM单位为rad/s
                float last_feedback_speed = 0; //上次速度，DJI，DM单位为RPM
                int16_t last_ecd = 0;          //DJI专用，单位为编码器计数
                int16_t feedback_ecd = 0;      //DJI专用，单位为编码器计数
                float position = 0;            //DM专用，单位为rad
                float force_feedback = 0;      //力矩反馈，DJI单位为电流分辨率，DM单位为Nm
                float temperature[2] = {0};    //温度反馈，DJI,DM单位为摄氏度
                uint8_t error = 0;             //错误码
                void clear()
                {
                    feedback_speed = 0;
                    last_feedback_speed = 0;
                    feedback_ecd = 0;
                    last_ecd = 0;
                    force_feedback = 0;
                    temperature[0] = 0;
                    temperature[1] = 0;
                    error = 0;
                }
            } motor_raw_data;

            // 处理后的电机数据结构体,以下都是输出轴数据
            struct Motor_Processed_Feedback_t
            {
                float speed = 0;           // 转速，DJI，DM单位为°/s
                float speed_difference = 0;// 转速差，DJI，DM单位为°/s
                float torque = 0;          // 力矩，DJI，DM单位为Nm
                float relative_angle = 0;  // 单圈相对角度，DJI，DM单位为°，0~360°
                float relative_angle_180 = 0;  // 单圈相对角度，归一化到-180~180°
                float absolute_angle = 0;  // 单圈绝对角度，DJI，DM单位为°，0~360°
                float total_ecd = 0;       // 总编码值,DJI专用，单位为编码器计数,这个不能输出轴数据
                float total_angle = 0;     // 总角度，DJI，DM单位为°
                float total_round = 0;     // 总圈数
                // 工程平移关节用
                float linear_speed = 0;        // 线速度，DJI有DM无，单位为m/s
                float linear_displacement = 0; // 线位移，DJI有DM无，单位为m
                // 加速度？感觉暂时不用了

                void clear()
                {
                    speed=0;           // 转速
                    speed_difference=0;// 转速差
                    torque=0;          // 力矩
                    relative_angle=0;  // 单圈相对角度
                    relative_angle_180=0; // 单圈相对角度（-180~180°）
                    absolute_angle=0;  // 单圈绝对角度
                    total_ecd=0;       // 总编码值
                    total_angle=0;     // 总角度
                    total_round=0;     // 总圈数
                    // 工程平移关节用
                    linear_speed=0;        // 线速度
                    linear_displacement=0; // 线位移
                }
            } motor_processed_data;

            // 电机固定参数
            struct Motor_Fixed_Param_t
            {
                float zero_offset; // 零位偏置
                float radius;      // 输出轴半径
                float ecd2length;  // 编码器计数到线性位移的转换比例
                float ratio;       // 减速比
                /* 以下参数给特定电机实现（如Dji），避免冗余 */
                float torque_constant; // 扭矩常数
                float encoder_resolution; // 编码器分辨率
                float current_resolution; // 电流分辨率
            } motor_fixed_param;
            Motor_Data_t(float zero_offset = 0, float radius = 0, float ecd2length = 0,float ratio = 1)
            {
                motor_fixed_param.zero_offset = zero_offset;
                motor_fixed_param.radius = radius;
                motor_fixed_param.ecd2length = ecd2length;
                motor_fixed_param.ratio = ratio;
            }
            void clear()
            {
                motor_raw_data.clear();
                motor_processed_data.clear();
            }
        } Motor_Data_t;


        // 电机基类配置结构体
        typedef struct Motor_Base_Config_t
        {
            std::string motor_name;
            Motor_Type_euc motor_type;
            Motor_Driver_Way_eb motor_driver_way;

            Motor_Control_Setting_t motor_control_setting;

            alg_n::PidInitConfig_t current_PID;
            alg_n::PidInitConfig_t speed_PID;
            alg_n::PidInitConfig_t angle_PID;

            BSP_n::CanInitConfig_s can_init_config;

            float zero_offset = 0;  // 零位偏置
            float ratio = 1;        // 减速比
            float radius = 0;       // 输出轴半径
            float ecd2length = 0;
            float max_output = 0, min_output = 0;
            uint32_t encoder_resolution = 8192; // 编码器分辨率
            float speed_smooth_coef = SPEED_SMOOTH_COEF;   // 速度滤波系数(0-1)，默认使用固定系数
            float current_smooth_coef = CURRENT_SMOOTH_COEF; // 电流滤波系数(0-1)，默认使用固定系数

            // 构造函数
            Motor_Base_Config_t(std::string name, Motor_Type_euc type) : motor_name(std::move(name)), motor_type(type)
            {
                // 设置类型相关的默认值
                switch(motor_type) {
                    case Motor_Type_euc::GM6020:
                    case Motor_Type_euc::M3508:
                    case Motor_Type_euc::M2006:
                        motor_driver_way = Motor_Driver_Way_eb::CURRENT_DRIVE;
                        break;
                    default:
                        motor_driver_way = Motor_Driver_Way_eb::MIT_DRIVE;
                        break;
                }
            }

            // 链式方法
            /**
             * @brief 链式编程写控制参数
             */
            Motor_Base_Config_t& SetControlSetting(Motor_Control_Setting_t setting) {
                motor_control_setting = setting;
                return *this;
            }

            /**
             * @brief 链式设置PID
             */
            Motor_Base_Config_t& SetPIDConfig(alg_n::PidInitConfig_t angle_pid,
                                            alg_n::PidInitConfig_t speed_pid = {},
                                            alg_n::PidInitConfig_t current_pid = {}) {
                angle_PID = angle_pid;
                speed_PID = speed_pid;
                current_PID = current_pid;
                return *this;
            }

            /**
             * @brief 链式指定CAN参数
             */
            Motor_Base_Config_t& SetCANConfig(BSP_n::CanInitConfig_s can_config) {
                can_init_config = can_config;
                return *this;
            }

            /**
             * @brief 链式指定固定参数
             */
            Motor_Base_Config_t& SetMechanicalParams(Motor_Data_t::Motor_Fixed_Param_t params) {
                zero_offset = params.zero_offset;
                radius = params.radius;
                ecd2length = params.ecd2length;
                ratio = params.ratio;
                encoder_resolution = params.encoder_resolution;
                return *this;
            }
            /**
             * @brief 链式设定功率限宽
             */
            Motor_Base_Config_t& SetOutputLimit(float max_output, float min_output = 0) {
                this->max_output = max_output;
                this->min_output = min_output;
                return *this;
            }
            /**
             * @brief 链式设定速度/电流滤波系数
             */
            Motor_Base_Config_t& SetFilterCoef(float speed_coef, float current_coef) {
                this->speed_smooth_coef = speed_coef;
                this->current_smooth_coef = current_coef;
                return *this;
            }
        }Motor_Base_Config_t;

        /* 类定义，以驼峰命名 */
        class MotorBase_c
        {
        public:
            /* 成员变量，以_结尾标识 */
            Motor_Type_euc motor_type_ = Motor_Type_euc::MOTOR_TYPE_NONE;
            char motor_name_[20] = {0};
            uint32_t motor_id_ = 0;
            uint32_t master_id_ = 0;    // 米狗常用
            Motor_Driver_Way_eb motor_driver_way_ = Motor_Driver_Way_eb::CURRENT_DRIVE;
            bool motor_init_flag_ = false;              // 初始化标志位
            bool motor_online_flag_ = false;            // 在线标志位
            bool motor_lost_latched_ = false;           // 掉线锁存标志
            float motor_max_output_ = 0, motor_min_output_ = 0; // 输出限幅
            float dt_ = 0, working_time_ = 0;                   // 反馈时间间隔和持续运行时间
            int64_t stuck_cnt_ = 0;                          // 堵转计数
            Motor_Stuck_Status_e stuck_flag_ = Motor_Stuck_Status_e::MOTOR_NOT_STUCK;            // 堵转标志
            Motor_Working_Status_euc motor_working_status_ = Motor_Working_Status_euc::MOTOR_STOP;
            float speed_smooth_coef_ = SPEED_SMOOTH_COEF;
            float current_smooth_coef_ = CURRENT_SMOOTH_COEF;
            Motor_Data_t motor_data_ = 0;
            Motor_Controller_t motor_controller_;
            Motor_MIT_Contorl_t motor_mit_contorl_data_;
            #ifdef STM32H723xx
                        BSP_n::Fdcan_c motor_can_instance_;
            #else
                        BSP_n::Can_c motor_can_instance_;
            #endif
            SafeTask_c motor_safe_task_;
            #ifdef USE_DEBUG_LIGHT
            Light_n::Debug_c* lost_trigger_ = nullptr;
            Light_n::Debug_c* error_trigger_ = nullptr;
            #endif
            std::array<uint8_t, 8> tx_buffer_;
            std::array<uint8_t, 8> rx_buffer_;

        public:
            /* 构造函数 */
            /**
             * @brief 构造基类并设备CAN/PID
             */
            MotorBase_c(Motor_Base_Config_t motor_config) : motor_type_(motor_config.motor_type), motor_driver_way_(motor_config.motor_driver_way),
                                                             motor_max_output_(motor_config.max_output),
                                                             motor_min_output_(motor_config.min_output),
                                                             speed_smooth_coef_((motor_config.speed_smooth_coef > 0.0f) ? motor_config.speed_smooth_coef : SPEED_SMOOTH_COEF),
                                                             current_smooth_coef_((motor_config.current_smooth_coef > 0.0f) ? motor_config.current_smooth_coef : CURRENT_SMOOTH_COEF),
                                                             motor_data_(motor_config.zero_offset, motor_config.radius, motor_config.ecd2length, motor_config.ratio),
                                                             motor_controller_(motor_config.motor_control_setting, &motor_config.angle_PID,
                                                                               &motor_config.speed_PID, &motor_config.current_PID, motor_config.max_output, motor_config.min_output), motor_can_instance_(motor_config.can_init_config),
                                                             motor_safe_task_((motor_config.motor_name + "SafeTask").c_str(), 50, [this]()
                                                                              { this->MotorLost(); },nullptr)
            {
                strcpy(motor_name_, motor_config.motor_name.c_str());
                motor_id_ = motor_config.can_init_config.rx_id;
                master_id_ = motor_config.can_init_config.tx_id;

#ifdef USE_DEBUG_LIGHT // 注册灯效触发类
                Light_n::Effect_t a_light;
                Light_n::Effect_t lost_light = Light_n::CANLOST_B_EFFECT;
                Light_n::Effect_t error_light = Light_n::CANERROR_B_EFFECT;
                lost_light.SetBlink(master_id_, lost_light.blink_cycle);
                error_light.SetBlink(master_id_, error_light.blink_cycle);
                std::snprintf(lost_light.name, sizeof(lost_light.name), "canL_%u", master_id_);
                std::snprintf(error_light.name, sizeof(error_light.name), "canE_%u", master_id_);
#ifdef USE_CAN1
                if(motor_can_instance_.GetCanhandle() == &hcan1)
                {
                    a_light = Light_n::CAN1_A_EFFECT;
                }
#endif
#ifdef USE_CAN2
                if(motor_can_instance_.GetCanhandle() == &hcan2)
                {
                    a_light = Light_n::CAN2_A_EFFECT;
                }
#endif
#ifdef USE_FDCAN1
                if(motor_can_instance_.GetFdcanhandle() == &hfdcan1)
                {
                    a_light = Light_n::CAN1_A_EFFECT;
                }
#endif
#ifdef USE_FDCAN2
                if(motor_can_instance_.GetFdcanhandle() == &hfdcan2)
                {
                    a_light = Light_n::CAN2_A_EFFECT;
                }
#endif
#ifdef USE_FDCAN3
                if(motor_can_instance_.GetFdcanhandle() == &hfdcan3)
                {
                    a_light = Light_n::CAN3_A_EFFECT;
                }
#endif
                // 同一组A/B灯效持续时间保持一致，避免两颗灯播放不同步
                a_light.duration = lost_light.duration;
                Light_n::Effect_t lost_effect[2] = {
                        a_light,
                        lost_light
                };
                Light_n::Effect_t error_effect[2] = {
                        a_light,
                        error_light
                };
                lost_trigger_ = new Light_n::Debug_c(lost_effect, &motor_online_flag_);
                error_trigger_ = new Light_n::Debug_c(error_effect, (bool*)&motor_data_.motor_raw_data.error);
                error_trigger_->Change_BoolCondition(true);
                lost_trigger_->Enable();
                error_trigger_->Enable();
#endif
            }

            // 设置回调函数
            template <typename Callable>
            /**
             * @brief 注册CAN回调
             */
            void SetCallback(Callable &&callback) noexcept
            {
                motor_can_instance_.SetRxCallBack(std::forward<Callable>(callback));
            }
            #ifdef STM32H723xx
                        /**
                         * @brief 处理FdCAN数据并弹急
                         */
                        void CallBack(BSP_n::Fdcan_c *can_instance) noexcept
                        {
                            // 获取CAN数据
                            can_instance->GetData(rx_buffer_.data());
                            ParseCANData(rx_buffer_);
                            Online();
                        }
            #else
                        /**
                         * @brief 处理CAN数据并刷新在线
                         */
                        void CallBack(BSP_n::Can_c *can_instance) noexcept
                        {
                            // 获取CAN数据
                            can_instance->GetData(rx_buffer_.data());
                            ParseCANData(rx_buffer_);
                            Online();
                        }
            #endif // USE_H7_if_or_not

            /* 函数定义，以驼峰格式命名，命名可以从简，省得写代码来回查 */
            /* 虚函数，可选择重写 */
            // void Init();
            /**
             * @brief 将电机设置为可输出状态
             */
            virtual void Enable() { motor_working_status_ = Motor_Working_Status_euc::MOTOR_ENALBED; motor_init_flag_ = true; }
            /**
             * @brief 将电机拉回停止
             */
            virtual void Disable() { motor_working_status_ = Motor_Working_Status_euc::MOTOR_STOP; }
            /**
             * @brief 设置只接受模式
             */
            virtual void OnlyReceive() { motor_working_status_ = Motor_Working_Status_euc::MOTOR_ONLY_RECEIVE; }
            /**
             * @brief 跳过PID下发固定功率
             */
            virtual void SetMotorOutputFix(float output) {
                motor_working_status_ = Motor_Working_Status_euc::MOTOR_OUTPUT_ONLY_ME;
                motor_controller_.pid_output = output;
            }
            /**
             * @brief 提供阻塞检测接口
             */
            virtual bool Is_MotorStuck(MotorBase_c *motor_ptr , float bulk_I , float bulk_speed , uint16_t bulk_cnt){ return false; }// 堵转检测
            /**
             * @brief 将外部数据拷贝后发送
             */
            virtual void Transmit(const std::array<uint8_t, 8> &data, float outtime)
            {
                std::copy(data.begin(), data.end(), tx_buffer_.begin());
                std::copy(data.begin(), data.end(), motor_can_instance_.tx_buff_);
                motor_can_instance_.Transmit(outtime);
            }
            // 重载函数，直接设置tx_buffer
            virtual void Transmit(float outtime)
            {
                // 根据发送协议设置tx_buffer
                std::copy(tx_buffer_.begin(), tx_buffer_.end(), motor_can_instance_.tx_buff_);
                motor_can_instance_.Transmit(outtime);
            }
            /**
             * @brief 设置MIT三参数和力矩
             */
            virtual void SetMITData(float pos, float vel, float kp, float kd, float t)
            {
                // 可添加一些通用参数的限幅，可参考dm_mit_mode.cpp
                motor_mit_contorl_data_.pos = pos;
                motor_mit_contorl_data_.vel = vel;
                motor_mit_contorl_data_.kp = kp;
                motor_mit_contorl_data_.kd = kd;
                motor_mit_contorl_data_.torq = t;
            }

            /* 纯虚函数，子类必须实现 */
            // 子类只需重新这个解析数据函数，can分发器可以根据其id精确分配数据给子类
            virtual void ParseCANData(const std::array<uint8_t, 8> data) = 0;

            /**
             * @brief 对外提供PID算法接口
             */
            float PIDCalculate(float *SetVal) { return motor_controller_.MotorPIDCalculate(SetVal); }
            /**
             * @brief 提供电机数据引用
             */
            const Motor_Data_t &GetMotorData() const { return motor_data_; }

            // 清除电机数据
            void MotorDataClear() { motor_data_.clear(); }

            void SetMaxOutput(float max_output) { motor_max_output_ = max_output; }
            /**
             * @brief 设定底线输出
             */
            void SetMinOutput(float min_output) { motor_min_output_ = min_output; }

        public:
            void Online() noexcept // 放到回调函数中
            {
                static uint32_t last_time = 0;
                dt_ = BSP_n::DWT_c::Get_DwtInstance()->GetDeltaT(&last_time);
                working_time_ += dt_;
                if (motor_lost_latched_)
                {
                    MOTOR_LOG_I("feedback_recovered name=%s rx_id=0x%lx",
                                motor_name_,
                                static_cast<unsigned long>(motor_id_));
                    motor_lost_latched_ = false;
                }
                motor_online_flag_ = true;
                motor_safe_task_.Online();
            }
            void MotorLost()
            {
                if (motor_online_flag_)
                {
                    MOTOR_LOG_E("feedback_lost name=%s rx_id=0x%lx timeout=%llums",
                                motor_name_,
                                static_cast<unsigned long>(motor_id_),
                                static_cast<unsigned long long>(motor_safe_task_.disconnection_threshold));
                    motor_lost_latched_ = true;
                }
                motor_online_flag_ = false;
                motor_init_flag_ = false;
                dt_ = working_time_ = 0;
            }
        };
    }
}

#endif

