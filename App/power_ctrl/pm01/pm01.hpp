//
// Created by Lann on 2025/3/1.
//

#ifndef SUPERPOWER_CALC_HPP
#define SUPERPOWER_CALC_HPP

#include "pm01_api.h"
#include "safe_task.hpp"
#include "bsp_can.hpp"
#include "dji_motor.hpp"



typedef struct
{
    float *pid_out;
    float *feedback_speed;
    int16_t *feedback_real_current;
} Power_Motor_t;

//底盘轮组编号
enum {
    RF_ = 0,
    RB_ = 1,
    LB_ = 2,
    LF_ = 3, 
}Wheel_Num_e;

typedef struct
{
    float Chassis_Power_All = 0;//底盘总功率
    int16_t speed[4];//转子转速
    int16_t I_out[4];//力矩电流
    float P_out[4];//电机输出功率
    float P_in[4];//电机输入功率
    int16_t F_out[4];//输出力矩
    int16_t I_max[4];//力矩电流
    int16_t give_current[4];//PID计算电流值
    int8_t rever[4] = {1,1,1,1}; //取绝对值系数

    float P_C620 = 0.6f;//电调功率损耗
    float K = 1.99688994e-6f; // (20/16384)  电调电流与真实电流比     *(0.3)*(187/3591)/9.55   M3509扭矩比例参数   电调值转化为力矩
    float K_1 = 1.66300012e-07;                 // k1  已经除 K
    float k2 = 1.90000804e-07;                     // k2
} Power_ctrl_t;

class Superpower_Calc
{
public:
    pm01_od_t pm01_od;
    BSP_CAN_Part_n::CANInstance_c *can_com;
    BSP_CAN_Part_n::CANInstance_c *can_com_I;
    float true_energy;

    Power_Motor_t lf;
    Power_Motor_t lb;
    Power_Motor_t rf;
    Power_Motor_t rb;

    Power_ctrl_t power_ctrl;
    uint8_t chassis_power_limit = 60;
    float Cap_Input_Power = 55;
    uint16_t power_time;
    uint8_t cmd = 0x02;
    uint8_t Max_Power;
    uint8_t New_Power;

    //缓冲能量(当前和预测)
    uint8_t buffer_power[3];

    void Init();
    static Superpower_Calc *getinstance() {
        return &superpower_instance;
    }
    void Get_Speed(float *lf_, float *lb_, float *rf_, float *rb_);
    void Get_PID_Out(float *lf_, float *lb_, float *rf_, float *rb_);
    void Get_Real_Current(int16_t *lf_, int16_t *lb_, int16_t *rf_, int16_t *rb_);
    void Read_SuperPower_Vout();
    void Read_SuperPower_Input();
    void Calc_SuperPower_energy(void);
    uint8_t Calc_SuperPower_Out_power_limit(void);
    uint8_t True_power_limit(uint8_t referee_power_limit = 60);
    void Read_and_Write_Power(uint16_t power_buffer);
    void SetInitInputPower(uint8_t power_limit);
    float getNextPowerBuff(float current_Power,float limit_Power,uint16_t buff_Power);
    void Update_Buffer_Power();
    float Get_Cap_power();
    void Set_Chassis_Power_Limit(uint8_t power_limit);
    
    // pm01_api
    void pm01_cmd_send( uint16_t new_cmd, uint8_t save_flg );
    void pm01_power_set( uint16_t new_power, uint8_t save_flg );
    void pm01_voltage_set( uint16_t new_voltage, uint8_t save_flg );
    void pm01_current_set( uint16_t new_current, uint8_t save_flg );
    void pm01_access_poll(void);
    void pm01_response_handle(CAN_RxHeaderTypeDef  *can_rx_header, uint8_t *can_rx_data );
    pm01_od_t* get_superpower_point(void)
    {
        return &pm01_od;
    }
public:
    volatile uint16_t  pm01_access_id;    /* 正在访问的标识符     */
    volatile uint16_t  pm01_response_flg; /* 控制器响应成功标志位 */

    uint16_t g_cmd_set   = 2;
    uint16_t g_power_set = 5000;
    uint16_t g_vout_set  = 2400;
    uint16_t g_iout_set  = 500;

    Superpower_Calc() = default;
    static Superpower_Calc superpower_instance;
    uint8_t Pm01_is_connect;

    Safe_task_c *pm01_safe_task;
};

#endif //SUPERPOWER_CALC_HPP
