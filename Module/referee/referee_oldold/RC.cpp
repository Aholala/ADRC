
/*************************** Dongguan-University of Technology
 *-ACE**************************
 * @file    RC.cpp
 * @author  zhengNannnn & Lann
 * @version V3.0
 * @date    2024/9/13
 * @brief
 ******************************************************************************
 * @verbatim
 *  RC 融合 dr16 裁判系统数据
 * @attention
 *  依赖 safe_task 文件
 * @version           time
 * v1.0   基础版本
 * v2.0   C++升级版本  2024-8-27
 * v3.0   适配新框架 2025-4-16
 ************************** Dongguan-University of Technology
 *-ACE***************************/
#include "RC.hpp"

#include <cstdint>

#include "CRC.h"
#include "RC_config.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#ifdef __cplusplus
}
#endif

#define ABS(NUM) (((NUM) > 0 ? (NUM) : (-NUM)))

#define RE_FRAME_LENGTH 128


#ifdef RECIVE_DT7_CONTROL
void ECF_RC::Dt7_Clear(void)
{
    this->Dt7.kb.key_code = 0;
    this->Dt7.mouse = {0};
    memset(this->Dt7.rc.ch, 0, sizeof(int16_t) * 5);
    this->errorFlag.bit.dr16 = 0;
    this->Dt7.rc.s1 = RC_SW_DOWN;
    this->Dt7.rc.s2 = RC_SW_MID;

    this->Updata_ctrl(false);
}

static void Dt7_Online()
{
    ECF_RC::ECF_RC_instance.errorFlag.bit.dr16 = 1;
}

/*dr16回调函数*/
void DT7callback(uint8_t *pdata, uint16_t psize)
{
    if (psize == RC_FRAME_LENGTH) {
        ECF_RC::getInstance()->DT7_DataProcess(pdata, psize);
    }
}
static void DT7_disonline_clear(void)
{
    ECF_RC::getInstance()->Dt7_Clear();
}
void ECF_RC::ECF_RC_Init()
{
    // dr16串口配置
    USART_N::usart_init_t dr16_uart_param = {
            .usart_handle_ = &DT7_USART,
            .rxbuf_size_ = RC_FRAME_LENGTH * 2,      // 接收区大小
            .rx_type_ = USART_N::USART_RX_DMA_IDLE_D,// 接收类型
            .tx_type_ = USART_N::USART_TX_DMA,       // 发送类型
            .usart_rx_callback_ptr_ = &DT7callback,
            .usart_data_length_ = RC_FRAME_LENGTH,
            .rx_buff_ptr_ = ECF_RC::ECF_RC_instance.Sbus_RX_Buffer[0],
            .secondebuf_ptr_ = ECF_RC::ECF_RC_instance.Sbus_RX_Buffer[1]};
    dr16 = new USART_N::usart_c(dr16_uart_param);
    DT7_Safe = new Safe_task_c("DT7", 200, DT7_disonline_clear, Dt7_Online);
    dr16->USART_rx_start();
}
void ECF_RC_Init()
{
    ECF_RC::ECF_RC_instance.ECF_RC_Init();
}
#endif

// Safe_task_c TC_Safe("TC", 100, TC_disonline_clear, nullptr);

#ifdef RECIVE_REFFEREE
/*裁判系统回调函数*/
void REFEREEcallback(uint8_t *pdata, uint16_t psize)// 裁判系统数据回调函数
{
    memcpy(ECF_RC::ECF_RC_instance.REFFEREE.RefereeData, pdata, psize);
    ECF_RC::ECF_RC_instance.REFFEREE.DataLen = psize;
    ECF_RC::ECF_RC_instance.REFFEREE_DataProcess();
}
static void RE_Online()
{
    ECF_RC::ECF_RC_instance.errorFlag.bit.referee = 1;
}
void ECF_RC::RE_Clear(void)
{
    this->errorFlag.bit.referee = 0;

    this->REFFEREE.PHOTO_ctrl.kb.key_code = 0;
    this->REFFEREE.PHOTO_ctrl.mouse.press_l = 0;
    this->REFFEREE.PHOTO_ctrl.mouse.press_r = 0;
    this->REFFEREE.PHOTO_ctrl.mouse.x = 0;
    this->REFFEREE.PHOTO_ctrl.mouse.y = 0;
    this->REFFEREE.PHOTO_ctrl.mouse.z = 0;
}
static void RE_disonline_clear(void)
{
    ECF_RC::getInstance()->RE_Clear();
}
void ECF_RC::ECF_REFEREE_Init()
{
    // 裁判系统串口配置
    USART_N::usart_init_t referee_uart_param = {
            .usart_handle_ = &REFFEREE_USART,
            .rxbuf_size_ = REFEREE_RX_Buffer_Num,    // 接收区大小
            .rx_type_ = USART_N::USART_RX_DMA_IDLE_D,// 接收类型 USART_RX_DMA_IDLE
            .tx_type_ = USART_N::USART_TX_DMA,       // 发送类型
            .usart_tx_callback_ptr_ = nullptr,
            .usart_rx_callback_ptr_ = &REFEREEcallback,// 接收回调函数指针
            .rx_buff_ptr_ = ECF_RC::ECF_RC_instance.Referee_RX_Buffer[0],
            .secondebuf_ptr_ = ECF_RC::ECF_RC_instance.Referee_RX_Buffer[1]};
    referee = new USART_N::usart_c(referee_uart_param);
    RE_Safe = new Safe_task_c("RE", 500, RE_disonline_clear, RE_Online);
    referee->USART_rx_start();
}
void ECF_REFEREE_Init()
{
    ECF_RC::ECF_RC_instance.ECF_REFEREE_Init();
}
#endif

#ifdef RECIVE_VTM_CONTROL
/*图传接收端回调函数*/
void VTMcallback(uint8_t *pdata, uint16_t psize)
{
    memcpy(ECF_RC::ECF_RC_instance.REFFEREE.RefereeData, pdata, psize);
    ECF_RC::ECF_RC_instance.REFFEREE.DataLen = psize;
    ECF_RC::ECF_RC_instance.REFFEREE_DataProcess();
    // ECF_RC::ECF_RC_instance.VTM_DataProcess();
}
static void VTM_Online()
{
    ECF_RC::getInstance()->errorFlag.bit.photo = 1;
}
void ECF_RC::VTM_Clear(void)
{
    // 计算 ch_0 到 key 的字节数
    size_t size = reinterpret_cast<uint8_t*>(&ECF_RC::getInstance()->VTM.data) - reinterpret_cast<uint8_t*>(&ECF_RC::getInstance()->VTM.sof_1);
    // 使用 memset 清零
    memset(&ECF_RC::getInstance()->VTM.sof_1, 0, size);;
    ECF_RC::getInstance()->VTM.mode_sw = 1;

    this->Updata_ctrl(false);
}
static void VTM_disonline_clear(void)
{
    ECF_RC::getInstance()->VTM_Clear();
    ECF_RC::getInstance()->errorFlag.bit.photo = 0;
}
void ECF_RC::ECF_VTM_Init()
{
    // 视觉通信串口配置
    USART_N::usart_init_t vtm_uart_param = {
            .usart_handle_ = &VTM_USART,
            .rxbuf_size_ = REFEREE_RX_Buffer_Num,    // 接收区大小
            .rx_type_ = USART_N::USART_RX_DMA_IDLE_D,// 接收类型 USART_RX_DMA_IDLE
            .tx_type_ = USART_N::USART_TX_DMA,       // 发送类型
            .usart_tx_callback_ptr_ = nullptr,
            .usart_rx_callback_ptr_ = &VTMcallback,// 接收回调函数指针
            .rx_buff_ptr_ = ECF_RC::ECF_RC_instance.Referee_RX_Buffer[0],
            .secondebuf_ptr_ = ECF_RC::ECF_RC_instance.Referee_RX_Buffer[1]};
    TC_Safe = new Safe_task_c("TC", 200, VTM_disonline_clear, VTM_Online);
    vtm = new USART_N::usart_c(vtm_uart_param);
    vtm->USART_rx_start();
}
void ECF_VTM_Init()
{
    ECF_RC::ECF_RC_instance.ECF_VTM_Init();
}
#endif

#ifdef RECIVE_FORWARD
static void Deal_Recive_Forward(CAN_RxHeaderTypeDef *Rxmessage, uint8_t *data)
{
    if (Rxmessage->StdId != FORWARD_CANID)
        return;
    memcpy(ECF_RC_instance->Forward_ctrl.U8, data, 8);
    ECF_RC_instance->Updata_ctrl(true);
}
#endif

/**
 * @brief 总初始化接口
 *
 */
void ECFRC_Init()
{
#ifdef RECIVE_DT7_CONTROL
    ECF_RC::ECF_RC_instance.ECF_RC_Init();
#endif
#ifdef RECIVE_REFEREE
    ECF_RC::ECF_RC_instance.ECF_REFEREE_Init();
#endif
#ifdef RECIVE_VTM_CONTROL
    ECF_RC::ECF_RC_instance.ECF_VTM_Init();
#endif
}


/**
 * @brief 裁判系统数据段解析 与 整包CRC16校验
 * @param RefereeData 帧头解析出来的CMDID对应的细分数据结构体指针
 * @param frame_header 裁判系统此次通信的帧头指针
 * @param data_length  帧头解析出来的CMDID对应的细分数据结构体长度
 */
void ECF_RC::RefereeDataCRC16Deal(void *RefereeData, uint8_t *frame_header,
                                  uint8_t data_length)
{
    uint8_t *RefereeDataU8 = (uint8_t *) RefereeData;
    if (Verify_CRC16_Check_Sum(frame_header, HEADER_LEN + CMDID_LEN + data_length + CRC16_LEN) == 1)// 整包CRC16校验
    {
        // 校验通过，搬运到数据结构体内
        memcpy(RefereeData, &frame_header[HEADER_LEN + CMDID_LEN], data_length);
        // 操作数据结构体内error，data_length不考虑数据结构体内error占用，因此指针移动后指向error所在字节
        memset(RefereeDataU8 + data_length, 0, sizeof(uint8_t));
    }
    else
        memset(RefereeDataU8 + data_length, 1, sizeof(uint8_t));
}

void ECF_RC::VTMDataCRC16Deal(void *VTMData, uint8_t *frame_header)
{
    uint8_t *VTMDataU8 = (uint8_t *) VTMData;
    if (Verify_CRC16_Check_Sum(frame_header, 21) == 1) {// 整包CRC16校验
        // 校验通过，搬运到数据结构体内
        memcpy(VTMData, &frame_header[2], 19);
    }
}

/**
* @brief 合并DT7与图传链路数据
* @param Recive_forward 该数据来源为 其他板子 转发而来
*/
void ECF_RC::Updata_ctrl(bool Recive_forward)
{
    if (Recive_forward) {
        this->RCData.kb.key_code = this->Dt7.kb.key_code |
                                   this->REFFEREE.PHOTO_ctrl.kb.key_code |
                                   this->VTM.key |
                                   Forward_ctrl.Struct.key_code;
        this->RCData.mouse.x = this->Dt7.mouse.x |
                               this->REFFEREE.PHOTO_ctrl.mouse.x |
                               this->VTM.mouse_x |
                               Forward_ctrl.Struct.mouseX;
        this->RCData.mouse.y = this->Dt7.mouse.y |
                               -(this->REFFEREE.PHOTO_ctrl.mouse.y) |
                               this->VTM.mouse_y |
                               Forward_ctrl.Struct.mouseY;
        this->RCData.mouse.z = this->Dt7.mouse.z |
                               this->REFFEREE.PHOTO_ctrl.mouse.z |
                               this->VTM.mouse_z;
        this->RCData.mouse.press_l = this->Dt7.mouse.press_l |
                                     this->REFFEREE.PHOTO_ctrl.mouse.press_l |
                                     this->VTM.mouse_left |
                                     this->Forward_ctrl.Struct.mouseL_And_ch4_Set;
        this->RCData.mouse.press_r = this->Dt7.mouse.press_r |
                                     this->REFFEREE.PHOTO_ctrl.mouse.press_r |
                                     this->VTM.mouse_right |
                                     this->Forward_ctrl.Struct.mouseR;
        this->RCData.rc = this->Dt7.rc;
        this->RCData.rc.ch[0] |= this->VTM.ch[0];
        this->RCData.rc.ch[1] |= this->VTM.ch[1];
        this->RCData.rc.ch[2] |= this->VTM.ch[3];//勾八通道跟dr16反的，sb吧
        this->RCData.rc.ch[3] |= this->VTM.ch[2];//勾八通道跟dr16反的，sb吧
        this->RCData.rc.ch[4] |= this->VTM.ch[4];
        if (this->Forward_ctrl.Struct.rs2 != RC_SW_ERROR) {
            this->RCData.rc.ch[0] = this->Forward_ctrl.Struct.ch0 - 660;
            this->RCData.rc.ch[1] = this->Forward_ctrl.Struct.ch1 - 660;
            this->RCData.rc.s2 = this->Forward_ctrl.Struct.rs2;
        }

        this->RCData.vtm.button.mode_sw = this->VTM.mode_sw;
        this->RCData.vtm.button.pause = this->VTM.pause;
        this->RCData.vtm.button.fn_1 = this->VTM.fn_1;
        this->RCData.vtm.button.fn_2 = this->VTM.fn_2;
        this->RCData.vtm.button.trigger = this->VTM.trigger;
    } else {
        this->RCData.kb.key_code =
                this->Dt7.kb.key_code | this->REFFEREE.PHOTO_ctrl.kb.key_code;
        this->RCData.mouse.x =
                this->Dt7.mouse.x | this->REFFEREE.PHOTO_ctrl.mouse.x;
        this->RCData.mouse.y =
                -(this->Dt7.mouse.y | this->REFFEREE.PHOTO_ctrl.mouse.y);
        this->RCData.mouse.z =
                this->Dt7.mouse.z | this->REFFEREE.PHOTO_ctrl.mouse.z;
        this->RCData.mouse.press_l =
                this->Dt7.mouse.press_l | this->REFFEREE.PHOTO_ctrl.mouse.press_l;
        this->RCData.mouse.press_r =
                this->Dt7.mouse.press_r | this->REFFEREE.PHOTO_ctrl.mouse.press_r;
        this->RCData.mouse.press_m = this->VTM.mouse_middle;

        this->RCData.rc = this->Dt7.rc;
        this->RCData.rc.ch[0] |= this->VTM.ch[0];
        this->RCData.rc.ch[1] |= this->VTM.ch[1];
        this->RCData.rc.ch[2] |= this->VTM.ch[3];//勾八通道跟dr16反的，sb吧
        this->RCData.rc.ch[3] |= this->VTM.ch[2];//勾八通道跟dr16反的，sb吧
        this->RCData.rc.ch[4] |= -this->VTM.ch[4];

        this->RCData.vtm.button.mode_sw = this->VTM.mode_sw;
        this->RCData.vtm.button.pause = this->VTM.pause;
        this->RCData.vtm.button.fn_1 = this->VTM.fn_1;
        this->RCData.vtm.button.fn_2 = this->VTM.fn_2;
        this->RCData.vtm.button.trigger = this->VTM.trigger;
    }
}

/**
 * @brief DT7数据解析
 * @note  一次转发
 */
void ECF_RC::DT7_DataProcess(uint8_t *pData, uint16_t psize)
{
    uint8_t Dt7buff[RC_FRAME_LENGTH] = {0};
    memcpy(&Dt7buff, pData, psize);

    // uint8_t *pData = Sbus_RX_Buffer[idx];
    this->Dt7.rc.ch[0] = ((int16_t) Dt7buff[0] | ((int16_t) Dt7buff[1] << 8)) &
                         0x07FF;//!< Channel 0
    this->Dt7.rc.ch[1] =
            (((int16_t) Dt7buff[1] >> 3) | ((int16_t) Dt7buff[2] << 5)) &
            0x07FF;//!< Channel 1
    this->Dt7.rc.ch[2] =
            (((int16_t) Dt7buff[2] >> 6) | ((int16_t) Dt7buff[3] << 2) |//!< Channel 2
             ((int16_t) Dt7buff[4] << 10)) &
            0x07FF;

    this->Dt7.rc.ch[3] =
            (((int16_t) Dt7buff[4] >> 1) | ((int16_t) Dt7buff[5] << 7)) &
            0x07FF;//!< Channel 3

    this->Dt7.rc.s1 = ((Dt7buff[5] >> 4) & 0x000C) >> 2;//!< Switch left
    this->Dt7.rc.s2 = ((Dt7buff[5] >> 4) & 0x0003);     //!< Switch right

    this->Dt7.mouse.x =
            ((int16_t) Dt7buff[6]) | ((int16_t) Dt7buff[7] << 8);//!< Mouse X axis
    this->Dt7.mouse.y =
            ((int16_t) Dt7buff[8]) | ((int16_t) Dt7buff[9] << 8);//!< Mouse Y axis
    this->Dt7.mouse.z =
            ((int16_t) Dt7buff[10]) | ((int16_t) Dt7buff[11] << 8);//!< Mouse Z axis

    this->Dt7.mouse.press_l = Dt7buff[12];//!< Mouse Left Is Press ?
    this->Dt7.mouse.press_r = Dt7buff[13];//!< Mouse Right Is Press ?

    this->Dt7.kb.key_code = Dt7buff[14] | (Dt7buff[15] << 8);//!< KeyBoard value
    this->Dt7.rc.ch[4] =
            ((int16_t) Dt7buff[16]) | ((int16_t) Dt7buff[17] << 8);// 左上角滑轮

    this->Dt7.rc.ch[0] -= RC_CH_VALUE_OFFSET;
    this->Dt7.rc.ch[1] -= RC_CH_VALUE_OFFSET;
    this->Dt7.rc.ch[2] -= RC_CH_VALUE_OFFSET;
    this->Dt7.rc.ch[3] -= RC_CH_VALUE_OFFSET;
    this->Dt7.rc.ch[4] -= RC_CH_VALUE_OFFSET;

    // 死区限制
    this->Dt7.rc.ch[0] =
            (ABS(this->Dt7.rc.ch[0]) < this->deadline_limt[0] ? 0
                                                              : this->Dt7.rc.ch[0]);
    this->Dt7.rc.ch[1] =
            (ABS(this->Dt7.rc.ch[1]) < this->deadline_limt[1] ? 0
                                                              : this->Dt7.rc.ch[1]);
    this->Dt7.rc.ch[2] =
            (ABS(this->Dt7.rc.ch[2]) < this->deadline_limt[2] ? 0
                                                              : this->Dt7.rc.ch[2]);
    this->Dt7.rc.ch[3] =
            (ABS(this->Dt7.rc.ch[3]) < this->deadline_limt[3] ? 0
                                                              : this->Dt7.rc.ch[3]);
    this->Dt7.rc.ch[4] =
            (ABS(this->Dt7.rc.ch[4]) < this->deadline_limt[4] ? 0
                                                              : this->Dt7.rc.ch[4]);

    this->Updata_ctrl(false);
#ifdef SEND_FORWARD
    this->Forward_by_Can(false);
#endif
    DT7_Safe->Online();
}

/**
 * @brief 图传控制/裁判系统共用数据解析
 * @note  图传控制一次转发
 */
void ECF_RC::REFFEREE_DataProcess()
{
    uint8_t i;
    for (i = 0; i < REFFEREE.DataLen; i++) {
        if (REFFEREE.RefereeData[i] == 0xA5) {// 帧头
            if (Verify_CRC8_Check_Sum(&REFFEREE.RefereeData[i], HEADER_LEN) ==
                1) {// 帧头CRC8校验
                REFFEREE.RealLen = ((REFFEREE.RefereeData[i + 1]) |
                                    (REFFEREE.RefereeData[i + 2] << 8));
                REFFEREE.Cmd_ID =
                        ((REFFEREE.RefereeData[i + HEADER_LEN]) |
                         (REFFEREE.RefereeData[i + HEADER_LEN + 1] << 8));// 命令码ID
                switch (REFFEREE.Cmd_ID) {
                    case ID_PICTURE_TRANSMISSION://把图传链路放前面，优化解析速度
                        RefereeDataCRC16Deal(&this->REFFEREE.PHOTO_ctrl,
                                             &REFFEREE.RefereeData[i],
                                             DATA_PICTURE_TRANSMISSION_LEN);
                        i = i + (DATA_PICTURE_TRANSMISSION_LEN + 9) - 1;
                        break;
                    case ID_STATE:
                        RefereeDataCRC16Deal(&this->REFFEREE.Game_Status,
                                             &REFFEREE.RefereeData[i], DATA_STATUS_LEN);
                        i = i + (DATA_STATUS_LEN + 9) + 9 - 1;
                        break;
                    case ID_RESULT:
                        RefereeDataCRC16Deal(&this->REFFEREE.Game_Result,
                                             &REFFEREE.RefereeData[i], DATA_RESULT_LEN);
                        i = i + (DATA_RESULT_LEN + 9) - 1;
                        break;
                    case ID_ROBOT_HP:
                        RefereeDataCRC16Deal(&this->REFFEREE.Robot_HP,
                                             &REFFEREE.RefereeData[i], DATA_ROBOT_HP_LEN);
                        i = i + (DATA_ROBOT_HP_LEN + 9) - 1;
                        break;
                    case ID_EVENT_DATA:
                        RefereeDataCRC16Deal(&this->REFFEREE.Event_Data,
                                             &REFFEREE.RefereeData[i], DATA_EVENT_DATA_LEN);
                        i = i + (DATA_EVENT_DATA_LEN + 9) - 1;
                        break;
                    case ID_SUPPLY_PROJECTILE_ACTION:
                        RefereeDataCRC16Deal(&this->REFFEREE.Supply_Action,
                                             &REFFEREE.RefereeData[i],
                                             DATA_SUPPLY_PROJECTILE_ACTION_LEN);
                        i = i + (DATA_SUPPLY_PROJECTILE_ACTION_LEN + 9) - 1;
                        break;
                    case ID_REFEREE_WARNING:
                        RefereeDataCRC16Deal(&this->REFFEREE.Referee_Warning,
                                             &REFFEREE.RefereeData[i],
                                             DATA_REFEREE_WARNING_LEN);
                        i = i + (DATA_REFEREE_WARNING_LEN + 9) - 1;
                        break;
                    case ID_DART_REMAINING_TIME:
                        RefereeDataCRC16Deal(&this->REFFEREE.Dart_Remaining_Time,
                                             &REFFEREE.RefereeData[i],
                                             DATA_DART_REMAINING_TIME_LEN);
                        i = i + (DATA_DART_REMAINING_TIME_LEN + 9) - 1;
                        break;
                    case ID_ROBOT_STATE:
                        RefereeDataCRC16Deal(&this->REFFEREE.Robot_Status,
                                             &REFFEREE.RefereeData[i], DATA_ROBOT_STATUS_LEN);
                        i = i + (DATA_ROBOT_STATUS_LEN + 9) - 1;
                        break;
                    case ID_POWER_HEAT_DATA:
                        RefereeDataCRC16Deal(&this->REFFEREE.Power_Heat,
                                             &REFFEREE.RefereeData[i],
                                             DATA_POWER_HEAT_DATA_LEN);
                        i = i + (DATA_POWER_HEAT_DATA_LEN + 9) - 1;
                        break;
                    case ID_ROBOT_POS:
                        RefereeDataCRC16Deal(&this->REFFEREE.Robot_Position,
                                             &REFFEREE.RefereeData[i], DATA_ROBOT_POS_LEN);
                        i = i + (DATA_ROBOT_POS_LEN + 9) - 1;
                        break;
                    case ID_BUFF:
                        RefereeDataCRC16Deal(&this->REFFEREE.Buff, &REFFEREE.RefereeData[i],
                                             DATA_BUFF_LEN);
                        i = i + (DATA_BUFF_LEN + 9) - 1;
                        break;
                    case ID_AERIAL_ROBOT_ENERGY:
                        RefereeDataCRC16Deal(&this->REFFEREE.Aerial_Energy,
                                             &REFFEREE.RefereeData[i],
                                             DATA_AERIAL_ROBOT_ENERGY_LEN);
                        i = i + (DATA_AERIAL_ROBOT_ENERGY_LEN + 9) - 1;
                        break;
                    case ID_ROBOT_HURT:
                        RefereeDataCRC16Deal(&this->REFFEREE.Robot_Hurt,
                                             &REFFEREE.RefereeData[i], DATA_ROBOT_HURT_LEN);
                        i = i + (DATA_ROBOT_HURT_LEN + 9) - 1;
                        break;
                    case ID_SHOOT_DATA:
                        RefereeDataCRC16Deal(&this->REFFEREE.Shoot_Data,
                                             &REFFEREE.RefereeData[i], DATA_SHOOT_DATA_LEN);
                        i = i + (DATA_SHOOT_DATA_LEN + 9) - 1;
                        break;
                    case ID_BULLET_REMAINING:
                        RefereeDataCRC16Deal(&this->REFFEREE.Bullet_Num,
                                             &REFFEREE.RefereeData[i],
                                             DATA_BULLET_REMAINING_LEN);
                        i = i + (DATA_BULLET_REMAINING_LEN + 9) - 1;
                        break;
                    case ID_RFID_STATUS:
                        RefereeDataCRC16Deal(&this->REFFEREE.RFID_Status,
                                             &REFFEREE.RefereeData[i], DATA_RFID_STATUS_LEN);
                        i = i + (DATA_RFID_STATUS_LEN + 9) - 1;
                        break;
                    case ID_DART_CLIENT_CMD:
                        RefereeDataCRC16Deal(&this->REFFEREE.Dart_Client,
                                             &REFFEREE.RefereeData[i],
                                             DATA_DART_CLIENT_CMD_LEN);
                        i = i + (DATA_DART_CLIENT_CMD_LEN + 9) - 1;
                        break;
                    case ID_GROUND_ROBOT_POSITION:
                        RefereeDataCRC16Deal(&this->REFFEREE.Robot_Position_Al,
                                             &REFFEREE.RefereeData[i],
                                             DATA_ROBOT_POSITION_LEN);
                        i = i + (DATA_ROBOT_POSITION_LEN + 9) - 1;
                        break;
                    case ID_RARD_MRAK_DATA:
                        RefereeDataCRC16Deal(&this->REFFEREE.radar_mark,
                                             &REFFEREE.RefereeData[i], DATA_RADAR_MARK_LEN);
                        i = i + (DATA_RADAR_MARK_LEN + 9) - 1;
                        break;
                    case ID_SENTRY:
                        RefereeDataCRC16Deal(&this->REFFEREE.sentry, &REFFEREE.RefereeData[i],
                                             DATA_SENTRY_INFO_LEN);
                        i = i + (DATA_SENTRY_INFO_LEN + 9) - 1;
                        break;
                    case ID_RADAR:
                        RefereeDataCRC16Deal(&this->REFFEREE.radar_info,
                                             &REFFEREE.RefereeData[i], DATA_RADAR_INFO_LEN);
                        i = i + (DATA_RADAR_INFO_LEN + 9) - 1;
                        break;
                    // case 0x301:
                    case ID_DIY_CONTROLLER:
                        RefereeDataCRC16Deal(&this->REFFEREE.DIY_control,
                                             &REFFEREE.RefereeData[i],
                                             DATA_DIY_CONTROLLER_LEN);
                        i = i + (DATA_DIY_CONTROLLER_LEN + 9) - 1;
                        break;
                    case ID_CLIENT_DOWMLOAD:// 修订
                        RefereeDataCRC16Deal(&this->REFFEREE.ClientMapData,
                                             &REFFEREE.RefereeData[i],
                                             DATA_CLIENT_DOWMLOAD_LEN);
                        i = i + (DATA_CLIENT_DOWMLOAD_LEN + 9) - 1;
                        break;
                    default:
                        break;
                }
            }
        }
#ifdef RECIVE_VTM_CONTROL
        else if (i != (REFFEREE.DataLen - 1) && REFFEREE.RefereeData[i] == 0xA9 &&
                 REFFEREE.RefereeData[i + 1] == 0x53) {//VT13遥控数据解析
            if (Verify_CRC16_Check_Sum(&REFFEREE.RefereeData[i], 21) == 1) {
                memcpy(&this->VTM, &REFFEREE.RefereeData[i], 21);
                VTM.ch[0] = ((int16_t) VTM.ch_0) - RC_CH_VALUE_OFFSET;
                VTM.ch[1] = ((int16_t) VTM.ch_1) - RC_CH_VALUE_OFFSET;
                VTM.ch[2] = ((int16_t) VTM.ch_2) - RC_CH_VALUE_OFFSET;
                VTM.ch[3] = ((int16_t) VTM.ch_3) - RC_CH_VALUE_OFFSET;
                VTM.ch[4] = ((int16_t) VTM.wheel) - RC_CH_VALUE_OFFSET;
                this->Updata_ctrl(false);
                TC_Safe->Online();
            }
        }
#endif
    }
}

/**
 * @brief VT13图传遥控数据段解析
 *
 */
void ECF_RC::VTM_DataProcess()
{
    uint8_t i;
    for (i = 0; i < REFFEREE.DataLen; i++) {
        if (REFFEREE.RefereeData[i] == 0xA5) {                                     // 帧头
            if (Verify_CRC8_Check_Sum(&REFFEREE.RefereeData[i], HEADER_LEN) == 1) {// 帧头CRC8校验
                REFFEREE.RealLen = ((REFFEREE.RefereeData[i + 1]) |
                                    (REFFEREE.RefereeData[i + 2] << 8));
                REFFEREE.Cmd_ID =
                        ((REFFEREE.RefereeData[i + HEADER_LEN]) |
                         (REFFEREE.RefereeData[i + HEADER_LEN + 1] << 8));// 命令码ID
                switch (REFFEREE.Cmd_ID) {
                    case ID_PICTURE_TRANSMISSION:
                        RefereeDataCRC16Deal(&this->REFFEREE.PHOTO_ctrl,
                                             &REFFEREE.RefereeData[i],
                                             DATA_PICTURE_TRANSMISSION_LEN);
                        i = i + (DATA_PICTURE_TRANSMISSION_LEN + 9) - 1;

#ifdef SEND_FORWARD
                        this->Forward_by_Can(true);
#endif
#ifdef RECIVE_VTM_CONTROL
                        this->Updata_ctrl(false);
                        TC_Safe->Online();
#endif
                        break;
                    default:
                        break;
                }
            }
        } else if (i != (REFFEREE.DataLen - 1) && REFFEREE.RefereeData[i] == 0xA9 &&
                   REFFEREE.RefereeData[i + 1] == 0x53) {
            if (Verify_CRC16_Check_Sum(&REFFEREE.RefereeData[i], 21) == 1) {
                memcpy(&this->VTM, &REFFEREE.RefereeData[i], 21);
                VTM.ch[0] = ((int16_t) VTM.ch_0) - RC_CH_VALUE_OFFSET;
                VTM.ch[1] = ((int16_t) VTM.ch_1) - RC_CH_VALUE_OFFSET;
                VTM.ch[2] = ((int16_t) VTM.ch_2) - RC_CH_VALUE_OFFSET;
                VTM.ch[3] = ((int16_t) VTM.ch_3) - RC_CH_VALUE_OFFSET;
                VTM.ch[4] = ((int16_t) VTM.wheel) - RC_CH_VALUE_OFFSET;
                this->Updata_ctrl(false);
                TC_Safe->Online();
            }
        }
    }
}

// 创建单例
ECF_RC ECF_RC::ECF_RC_instance = ECF_RC();
