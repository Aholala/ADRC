#ifndef __BSP_DR16_HPP__
#define __BSP_DR16_HPP__

#include <cstdint>
#include "RC_config.hpp"
#include "safe_task.hpp"

extern "C"
{
    #include "CRC.h"
    #include "string.h"
}

typedef union
{
    struct
    {
        uint8_t referee : 1;
        uint8_t photo : 1;
        uint8_t dr16 : 1; 
    } bit;
    uint8_t errorCode;
} errorFlag;

class ECF_RC
{
public:
    uint8_t Sbus_RX_Buffer[2][RC_FRAME_LENGTH] = {0};
    uint8_t Referee_RX_Buffer[2][REFEREE_RX_Buffer_Num] = {0};
    uint8_t VTM_RX_Buffer[2][VTM_FRAME_LENGTH*2] = {0};
    RC_ctrl_t RCData = {0};
    RC_ctrl_t Dt7 = {0};
    REFEREE_t REFFEREE = {0};
    remote_data_t VTM = {0};
    Forward_ctrl_t Forward_ctrl = {0};
    uint16_t deadline_limt[5] = {RC_DEAD_LINE, RC_DEAD_LINE, RC_DEAD_LINE, RC_DEAD_LINE, RC_DEAD_LINE}; // 遥控数据死区限制
    USART_N::usart_c *dr16;
    USART_N::usart_c *referee;
    USART_N::usart_c *vtm;
    Safe_task_c *DT7_Safe;
    Safe_task_c *TC_Safe;
    Safe_task_c *RE_Safe;
    errorFlag errorFlag = {0};

    void Dt7_Clear(void);
    void VTM_Clear(void);
    void RE_Clear(void);
    void Updata_ctrl(bool Recive_forward);
    RC_ctrl_t *getRCdata()
    {
        return &RCData;
    }
    REFEREE_t *getREdata()
    {
        return &REFFEREE;
    }
    static ECF_RC *getInstance()
    {
        return &ECF_RC_instance; // 返回静态成员变量 instance 的地址
    }
public:
    ECF_RC() = default; // 私有构造函数
    static ECF_RC ECF_RC_instance;//唯一实例指针
    void ECF_RC_Init();
    void ECF_REFEREE_Init();
    void ECF_VTM_Init();
    void DT7_DataProcess(uint8_t *pData, uint16_t psize);
    void REFFEREE_DataProcess();
    void VTM_DataProcess();
    void RefereeDataCRC16Deal(void *RefereeData, uint8_t*frame_header, uint8_t data_length);
    void VTMDataCRC16Deal(void *VTMData, uint8_t*frame_header);
    void Forward_by_Can(bool From_TC);
};
void DT7callback(uint8_t *pdata, uint16_t psize);
void REFEREEcallback(uint8_t *pdata, uint16_t psize); // 裁判系统数据回调函数
void VTMcallback(uint8_t *pdata, uint16_t psize);
void ECF_RC_Init();
void ECF_REFEREE_Init();
void ECF_VTM_Init();
void ECFRC_Init();
const REFEREE_t* Get_REFFEREE(void);
#endif