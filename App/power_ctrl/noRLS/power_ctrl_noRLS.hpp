#ifndef POWER_CTRL_NORLS_HPP
#define POWER_CTRL_NORLS_HPP

#include "motor_base_def.hpp"
#include "new_Cap.hpp"
#include "Alg_PID.hpp"
#include <array>
#include <cstdint>
#include <vector>
#include "referee.hpp"

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
#include "string.h"
#include "ecf_configList.h"
#ifdef __cplusplus
}

/************************** 枚举 **************************/
enum class PowerMode_e{ // 功率模式
    CHARGE_MODE  = 0,    //充电模式
    RECHARGE_MODE,       //补电模式
    FLY_MODE,            //飞坡模式
    USER_MODE,           //用户模式，自定义限制
    CAPACITOR_MODE_OFF,  // 关电容模式（现仅有这两个模式）
    CAPACITOR_MODE_ON    // 开电容模式（现仅有这两个模式）
};
/************************** 结构体 **************************/
struct Motors  // 用于重载
{
    const Motor_n::MotorBaseDef_n::MotorBase_c *motor[8];
    float k[8][6];

    const Motor_n::MotorBaseDef_n::MotorBase_c *&operator[](int index) { return motor[index]; }
    const Motor_n::MotorBaseDef_n::MotorBase_c *operator[](int index) const { return motor[index]; }
    float *KAt(int index) { return k[index]; }
    const float *KAt(int index) const { return k[index]; }

    Motors(std::vector<const Motor_n::MotorBaseDef_n::MotorBase_c *> &motors_,
           const std::vector<std::array<float, 6>> &k_list)
    {
        const size_t motor_count = motors_.size();
        const size_t k_count = k_list.size();
        for(int i = 0;i < 8;i++)
        {
            motor[i] = (static_cast<size_t>(i) < motor_count) ? motors_[i] : nullptr;
            for(int j = 0;j < 6;j++)
            {
                k[i][j] = (static_cast<size_t>(i) < k_count) ? k_list[i][j] : 0.0f;
            }
        }
    }
    Motors(std::vector<const Motor_n::MotorBaseDef_n::MotorBase_c *> &motors_)
    {
        const size_t count = motors_.size();
        for(int i = 0;i < 8;i++)
        {
            motor[i] = (static_cast<size_t>(i) < count) ? motors_[i] : nullptr;
            for(int j = 0;j < 6;j++) { k[i][j] = 0.0f; }
        }
    }
    Motors()
    {
        for(int i = 0;i < 8;i++)
        {
            motor[i] = nullptr;
            for(int j = 0;j < 6;j++) { k[i][j] = 0.0f; }
        }
    }
    void Set(int index, const Motor_n::MotorBaseDef_n::MotorBase_c *motor_ptr, const float k_in[6])
    {
        if (index < 0 || index >= 8) { return; }
        motor[index] = motor_ptr;
        for (int j = 0; j < 6; j++) { k[index][j] = k_in ? k_in[j] : 0.0f; }
    }
};

struct EnergyController_t{
    float min_voltage = 0;  // 最低安全电压(V)
    float max_voltage = 0;  // 最高电压(V)
    float min_power_limit = 0;
    float dynamic_power_limit = 0; //经能量误差计算得到的功率
    float actual_energy_percent = 0;   //实际能量百分比，计算能量百分比: (当前电压 - 最低电压) / (最高电压 - 最低电压) * 100
    float target_energy_percent = 0; //目标能量百分比
} ;// 能量环新增

    class PowerCtrl_c
    {
    public:
    /************************** 变量 **************************/
        PowerMode_e power_mode_ = PowerMode_e::CAPACITOR_MODE_OFF; // 我的功率模式
        alg_n::PID_c *energy_pid_alg = nullptr; // 能量环pid
        EnergyController_t energy_controller_; // 能量环结构体
        Cap_Ctrl_n::Cap_Ctrl_c *power_cap_data_; // 超电数据
        Referee_n::RefereeData_t *power_referee_data_ = nullptr; // 裁判系统数据
        float measured_single_power_[CHASSIS_TYPE] = {0}; // 单电机功率
        float measured_steer_sum_power_ = 0; // 舵电机总功率
        float measured_wheel_sum_power_ = 0; // 轮电机总功率
        float wheel_pos = 0.0f; // 轮积极输出的总功率
        float wheel_neg = 0.0f; // 轮消极输出的总功率
        float steer_pos = 0.0f; // 舵积极输出的总功率
        float steer_neg = 0.0f; // 舵消极输出的总功率
        float measured_sum_power_ = 0; // 总功率
        float measured_feedback_single_power_[CHASSIS_TYPE] = {0}; // 反馈力矩/反馈转速估算的单电机功率（仅显示）
        float measured_feedback_steer_sum_power_ = 0; // 反馈力矩/反馈转速估算的舵电机总功率（仅显示）
        float measured_feedback_wheel_sum_power_ = 0; // 反馈力矩/反馈转速估算的轮电机总功率（仅显示）
        float measured_feedback_sum_power_ = 0; // 反馈力矩/反馈转速估算的总功率（仅显示）
        float power_limit_set_ = 0; // 最终限制功率
        Motors motors_; // k值和电机数据
        uint8_t is_cap_online_ = 0; // 电容是否在线
        uint8_t is_re_online_ = 0; // 裁判系统是否在线
    /************************** 函数 **************************/
        // 初始化
        void Init(Motors &motor);
        // 功率分配函数（主要函数）
        float *Calc(std::vector<float *> &raw_output, std::vector<float *> &current_speed);
        // 电容和裁判系统在线处理
        void ReAndCapProcess();
        // 能量环设置
        void SupperCapPowerLimitUpdate();
        // 功率模式设置
        void SetPowerMode(PowerMode_e power_mode);
    /************************** 单例 **************************/
        static PowerCtrl_c *GetInstance()
        {
            static PowerCtrl_c power_ctrl_instance_;
            return &power_ctrl_instance_;
        }
        private:
            PowerCtrl_c() = default;
    };
// 功率衰减法：返回衰减系数rho并输出衰减后的力矩
float PowerDecaySolve(const float *torque_origin,const float *speed_rad_s,const float (*k)[6],int motor_count,float rho,float *torque_out);
// 电流/力矩衰减法：返回衰减系数eta并输出衰减后的力矩
float CurrentDecaySolve(const float *torque_origin,const float *speed_rad_s,const float (*k)[6],int motor_count,float pmax,float *torque_out);


#endif
#endif
