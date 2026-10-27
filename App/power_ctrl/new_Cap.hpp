#ifndef __MEW_CAP_HPP
#define __MEW_CAP_HPP




#include "safe_task.hpp"
#include "ecf_configList.h"
#include "bsp_dwt.hpp"

#ifdef STM32H723xx
#include "bsp_fdcan.hpp"
#elif STM32F405xx
#include "bsp_can.hpp"
#endif



namespace Cap_Ctrl_n
{
    


typedef struct
{
	float Vbat; //电源电压
    float Vcap; //电容电压
    float Cbat; //电源电流
    float Ccap; //电容电流
    float Pbat;   //电源功率
    float Pcap;   //电容功率
    float Chassis_Power; //底盘功率
    float Chassis_energy; //底盘消耗总能量
}cap_t;


class Cap_Ctrl_c
{
public:
    Cap_Ctrl_c(){};
    void init();
    void PollLog();

    //发送限制功率
    void Send_Limit_Data(uint16_t new_power)
    {
        new_power *= 100;
        can_com->tx_buff_[0] = (uint8_t)(new_power >> 8   );
        can_com->tx_buff_[1] = (uint8_t)(new_power & 0x00FF);
        can_com->SetDLC(2);
        // CONTROL_SEND_HZ(990);
        can_com->Transmit(1);
    }

    static Cap_Ctrl_c* getInstance()
    {
        return &cap_instance;
    }

    cap_t cap_data;
    SafeTask_c* cap_safe_task;
    BSP_n::DWT_c *cap_dwt ;


    private:

    #ifdef STM32H723xx
    BSP_n::Fdcan_c *can_com;
    #elif STM32F405xx
    BSP_n::Can_c *can_com;
    #endif
    
    static Cap_Ctrl_c cap_instance;
};



}

uint8_t get_cap_static();








#endif
