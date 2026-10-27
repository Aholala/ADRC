/*************************** Dongguan-University of Technology -ACE**************************
 * @file    referee.cpp
 * @author  zhengNannnn Lann coconutjuice 
 * @version v26.2.2.0
 * @date    2026-2-4
 * @brief   裁判系统模块文件
 ******************************************************************************
 * @verbatim
 * 
 * @attention
 *      依赖文件：
            USART
            safe_task
            referee_config.hpp
 * @version           time
 * v26.0.0.0  全新的体验  2025-11-29    已测试
 * v26.0.1.0  裁判系统掉线按键值清零，错误状态 2025-12-01
 * v26.0.1.1  修正键盘按键值                 2025-12-15
 * v26.2.0.0  全面更新1.1.0通信协议相关内容，VT13融入裁判系统的回调函数内，不用专门的  2026/2/4
 * v26.2.1.0  通信协议更新内容：0x105修改飞镖此时选定的击打目标的位域为3位，保留位改为8位；0x204没找到修改的地方；0x209修改RFID隧道地形跨越增益位含义；0x20C增加空中机器人特殊标识情况；0x20D哨兵包的保留位变成了17mm剩余发单量；0x309不知道改了啥；0x310自定义数据大包长度从150改成了300；0x304遥控数据包删掉了，用VT13吧 2026/2/27
 * v26.2.2.0  修改0x0303小地图命令内容 2026/4/26
 ************************** Dongguan-University of Technology -ACE***************************/
#include "referee.hpp"
#if defined(USE_JLINK_RTT)
#include "bsp_jlink_rtt.hpp"
#endif

#if defined(USE_JLINK_RTT) && defined(ECF_RTT_LOG_ENABLE_REFEREE) && (ECF_RTT_LOG_ENABLE_REFEREE)
#define REFEREE_LOG_I(fmt, ...) LOG_RTT_I("[REFEREE][LINK] " fmt __VA_OPT__(,) __VA_ARGS__)
#define REFEREE_LOG_W(fmt, ...) LOG_RTT_W("[REFEREE][CRC] " fmt __VA_OPT__(,) __VA_ARGS__)
#define REFEREE_LOG_E(fmt, ...) LOG_RTT_E("[REFEREE][LINK] " fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define REFEREE_LOG_I(fmt, ...) ((void)0)
#define REFEREE_LOG_W(fmt, ...) ((void)0)
#define REFEREE_LOG_E(fmt, ...) ((void)0)
#endif

namespace Referee_n {

/**************************** 裁判系统文档内容 裁判系统用函数 *********************/
#ifdef USE_REFEREE

// 校验用长度
uint16_t kFrameOverhead =
        REFEREE_HEADER_LEN + REFEREE_CMDID_LEN + REFEREE_CRC16_LEN;
static bool s_referee_disconnected_latched = false;
static bool s_vt13_disconnected_latched = false;
static bool s_crc16_fault_latched = false;

/**
* @brief 推包函数,根据包长度推地址
*/
inline uint16_t AdvanceIndex(uint16_t index, uint16_t data_length)
{
    return static_cast<uint16_t>(index + kFrameOverhead + data_length - 1U);
}

/**
* @brief 清VT13数据
*/
void referee_c::VtmClear()
{
    size_t size = reinterpret_cast<uint8_t *>(&vt13_data_.data) - reinterpret_cast<uint8_t *>(&vt13_data_.sof_1);
    memset(&vt13_data_.sof_1, 0, size);
    vt13_data_.mode_sw = 1;
}

/**
* @brief VT13在线函数
*/
void VtmOnline()
{
    if (s_vt13_disconnected_latched)
    {
        REFEREE_LOG_I("vt13 recovered");
        s_vt13_disconnected_latched = false;
    }
    referee_c::GetInstance()->is_online_tc_ = 1;
}

/**
* @brief VT13掉线函数
*/
void VtmDisOnline()
{
    auto *instance = referee_c::GetInstance();
    if (!s_vt13_disconnected_latched)
    {
        REFEREE_LOG_E("vt13 disconnected timeout=%llums", 200ull);
        s_vt13_disconnected_latched = true;
    }
    instance->VtmClear();
    instance->is_online_tc_ = 0;
}

/**
* @brief 裁判系统在线函数
*/
void RefereeOnline()
{
    if (s_referee_disconnected_latched)
    {
        REFEREE_LOG_I("referee recovered");
        s_referee_disconnected_latched = false;
    }
    referee_c::GetInstance()->is_online_re_ = 1;
}

/**
* @brief 裁判系统掉线函数
*/
void RefereeDisOnline()
{
    if (!s_referee_disconnected_latched)
    {
        REFEREE_LOG_E("referee disconnected timeout=%llums", 500ull);
        s_referee_disconnected_latched = true;
    }
    referee_c::GetInstance()->RefereeClear();
}

/**
* @brief 裁判系统回调函数
*/
void RefereeCallback(const uint8_t *pdata, uint16_t psize)
{
    auto *instance = referee_c::GetInstance();
    // const uint16_t copy_len =
    //         (psize > REFEREE_DATA_LEN_MAX) ? REFEREE_DATA_LEN_MAX : psize;
    const uint16_t copy_len = psize;
    memcpy(instance->referee_data_.RefereeData, pdata, copy_len);
    instance->referee_data_.DataLen = static_cast<int16_t>(copy_len);
    instance->RefereeDataProcess();
}

/**
* @brief 裁判系统串口初始化
*/
void referee_c::RefereeInit()
{
    BSP_n::USART_c::Config referee_uart_param = {
            .Handle = &REFEREE_USART,
            .RxBuffSize = REFEREE_RX_BUFFER_NUM,
            .RxType = BSP_n::USART_c::RxType_t::DMA_IDLE,
            .TxType = BSP_n::USART_c::TxType_t::DMA,
            .pTxCallback = nullptr,
            .pRxCallback = &RefereeCallback,
            .pRxBuffer = referee_rx_buffer_[0],
            .pSecRxBuffer = referee_rx_buffer_[1]};
    referee = new BSP_n::USART_c(referee_uart_param);
    referee_safe = new SafeTask_c("RE", 500, RefereeDisOnline, RefereeOnline);
    TC_Safe = new SafeTask_c("TC_NEW", 200, VtmDisOnline, VtmOnline);
    referee->start_receive();
    REFEREE_LOG_I("init ok uart=%s re_timeout_ms=%u vt13_timeout_ms=%u",
                  "REFEREE_USART", 500u, 200u);
}

/**
* @brief 清理裁判系统数据
*/
void referee_c::RefereeClear()
{
    is_online_re_ = 0;
    memset(&referee_data_, 0, sizeof(referee_data_));
}

/**
* @brief CRC校验
*/
void referee_c::RefereeDataCRC16Deal(void *RefereeData, uint8_t *frame_header, uint16_t data_length)
{
    uint8_t *RefereeDataU8 = static_cast<uint8_t *>(RefereeData);
    const uint16_t frame_length =
            REFEREE_HEADER_LEN + REFEREE_CMDID_LEN + data_length + REFEREE_CRC16_LEN;
    const uint16_t cmd_id =
            static_cast<uint16_t>(frame_header[REFEREE_HEADER_LEN]) |
            static_cast<uint16_t>(frame_header[REFEREE_HEADER_LEN + 1] << 8);
    if (Verify_CRC16_Check_Sum(frame_header, frame_length) == 1) 
    {
        if (s_crc16_fault_latched)
        {
            REFEREE_LOG_I("crc16 recovered cmd=0x%04x", static_cast<unsigned>(cmd_id));
            s_crc16_fault_latched = false;
        }
        memcpy(RefereeData, &frame_header[REFEREE_HEADER_LEN + REFEREE_CMDID_LEN], data_length);
        memset(RefereeDataU8 + data_length, 0, sizeof(uint8_t));
        if (referee_safe != nullptr) {
            referee_safe->Online();
        }
    } 
    else 
    {
        if (!s_crc16_fault_latched)
        {
            REFEREE_LOG_W("crc16 failed cmd=0x%04x len=%u",
                          static_cast<unsigned>(cmd_id),
                          static_cast<unsigned>(data_length));
            s_crc16_fault_latched = true;
        }
        memset(RefereeDataU8 + data_length, 1, sizeof(uint8_t));
    }
}

/**
* @brief 回调处理
*/
void referee_c::RefereeDataProcess()
{
    for (uint16_t i = 0; i < static_cast<uint16_t>(referee_data_.DataLen); i++) {
        if (referee_data_.RefereeData[i] == REFEREE_FRAME_TAG) // 包头0xA5
        {
            if (Verify_CRC8_Check_Sum(&referee_data_.RefereeData[i], REFEREE_HEADER_LEN) == 1) { // 帧头校验
                referee_data_.RealLen = (referee_data_.RefereeData[i + 1]) |
                                        (referee_data_.RefereeData[i + 2] << 8);
                referee_data_.Cmd_ID = (referee_data_.RefereeData[i + REFEREE_HEADER_LEN]) |
                        (referee_data_.RefereeData[i + REFEREE_HEADER_LEN + 1] << 8);
                switch (referee_data_.Cmd_ID) { // ID确认
                    case ID_REMOTE_CONTROL:
                        RefereeDataCRC16Deal(&referee_data_.PHOTO_ctrl, &referee_data_.RefereeData[i],
                                            DATA_REMOTE_CONTROL_LEN);
                        i = AdvanceIndex(i, DATA_REMOTE_CONTROL_LEN);
                        break;
                    case ID_GAME_STATUS:
                        RefereeDataCRC16Deal(&referee_data_.Game_Status, &referee_data_.RefereeData[i],
                                            DATA_GAME_STATUS_LEN);
                        i = AdvanceIndex(i, DATA_GAME_STATUS_LEN);
                        break;
                    case ID_GAME_RESULT:
                        RefereeDataCRC16Deal(&referee_data_.Game_Result, &referee_data_.RefereeData[i],
                                            DATA_GAME_RESULT_LEN);
                        i = AdvanceIndex(i, DATA_GAME_RESULT_LEN);
                        break;
                    case ID_ROBOT_HP:
                        RefereeDataCRC16Deal(&referee_data_.Robot_HP, &referee_data_.RefereeData[i],
                                            DATA_ROBOT_HP_LEN);
                        i = AdvanceIndex(i, DATA_ROBOT_HP_LEN);
                        break;
                    case ID_EVENT_DATA:
                        RefereeDataCRC16Deal(&referee_data_.Event_Data, &referee_data_.RefereeData[i],
                                            DATA_EVENT_LEN);
                        i = AdvanceIndex(i, DATA_EVENT_LEN);
                        break;
                    case ID_REFEREE_WARNING:
                        RefereeDataCRC16Deal(&referee_data_.Referee_Warning, &referee_data_.RefereeData[i],
                                            DATA_REFEREE_WARNING_LEN);
                        i = AdvanceIndex(i, DATA_REFEREE_WARNING_LEN);
                        break;
                    case ID_DART_INFO:
                        RefereeDataCRC16Deal(&referee_data_.Dart_info_, &referee_data_.RefereeData[i],
                                            DATA_DART_INFO_LEN);
                        i = AdvanceIndex(i, DATA_DART_INFO_LEN);
                        break;
                    case ID_ROBOT_STATUS:
                        RefereeDataCRC16Deal(&referee_data_.Robot_Status, &referee_data_.RefereeData[i],
                                            DATA_ROBOT_STATUS_LEN);
                        i = AdvanceIndex(i, DATA_ROBOT_STATUS_LEN);
                        break;
                    case ID_POWER_HEAT:
                        RefereeDataCRC16Deal(&referee_data_.Power_Heat, &referee_data_.RefereeData[i],
                                            DATA_POWER_HEAT_LEN);
                        i = AdvanceIndex(i, DATA_POWER_HEAT_LEN);
                        break;
                    case ID_ROBOT_POSITION:
                        RefereeDataCRC16Deal(&referee_data_.Robot_Position, &referee_data_.RefereeData[i],
                                            DATA_ROBOT_POS_LEN);
                        i = AdvanceIndex(i, DATA_ROBOT_POS_LEN);
                        break;
                    case ID_BUFF:
                        RefereeDataCRC16Deal(&referee_data_.Buff, &referee_data_.RefereeData[i],
                                            DATA_BUFF_LEN);
                        i = AdvanceIndex(i, DATA_BUFF_LEN);
                        break;
                    case ID_ROBOT_HURT:
                        RefereeDataCRC16Deal(&referee_data_.Robot_Hurt, &referee_data_.RefereeData[i],
                                            DATA_ROBOT_HURT_LEN);
                        i = AdvanceIndex(i, DATA_ROBOT_HURT_LEN);
                        break;
                    case ID_SHOOT_DATA:
                        RefereeDataCRC16Deal(&referee_data_.Shoot_Data, &referee_data_.RefereeData[i],
                                            DATA_SHOOT_LEN);
                        i = AdvanceIndex(i, DATA_SHOOT_LEN);
                        break;
                    case ID_PROJECTILE_ALLOW:
                        RefereeDataCRC16Deal(&referee_data_.Bullet_Num,
                                            &referee_data_.RefereeData[i],
                                            DATA_PROJECTILE_ALLOW_LEN);
                        i = AdvanceIndex(i, DATA_PROJECTILE_ALLOW_LEN);
                        break;
                    case ID_RFID_STATUS:
                        RefereeDataCRC16Deal(&referee_data_.RFID_Status, &referee_data_.RefereeData[i],
                                            DATA_RFID_STATUS_LEN);
                        i = AdvanceIndex(i, DATA_RFID_STATUS_LEN);
                        break;
                    case ID_DART_CLIENT_CMD:
                        RefereeDataCRC16Deal(&referee_data_.Dart_Client,
                                            &referee_data_.RefereeData[i], DATA_DART_CLIENT_LEN);
                        i = AdvanceIndex(i, DATA_DART_CLIENT_LEN);
                        break;
                    case ID_GROUND_ROBOT_POSITION:
                        RefereeDataCRC16Deal(&referee_data_.Robot_Position_Al,
                                            &referee_data_.RefereeData[i], DATA_GROUND_ROBOT_POS_LEN);
                        i = AdvanceIndex(i, DATA_GROUND_ROBOT_POS_LEN);
                        break;
                    case ID_RADAR_MARK:
                        RefereeDataCRC16Deal(&referee_data_.radar_mark, &referee_data_.RefereeData[i],
                                            DATA_RADAR_MARK_LEN);
                        i = AdvanceIndex(i, DATA_RADAR_MARK_LEN);
                        break;
                    case ID_SENTRY_INFO:
                        RefereeDataCRC16Deal(&referee_data_.sentry, &referee_data_.RefereeData[i],
                                            DATA_SENTRY_INFO_LEN);
                        i = AdvanceIndex(i, DATA_SENTRY_INFO_LEN);
                        break;
                    case ID_RADAR_INFO:
                        RefereeDataCRC16Deal(&referee_data_.radar_info, &referee_data_.RefereeData[i],
                                            DATA_RADAR_INFO_LEN);
                        i = AdvanceIndex(i, DATA_RADAR_INFO_LEN);
                        break;
                    case ID_ROBOT_INTERACTION:
                    {
                        uint16_t date_len = static_cast<uint16_t>(referee_data_.RefereeData[i + 1]) |
                                            (static_cast<uint16_t>(referee_data_.RefereeData[i + 2]) << 8);
                        RefereeDataCRC16Deal(&referee_data_.Interact_Data,
                                             &referee_data_.RefereeData[i], date_len);
                        i = AdvanceIndex(i, date_len);
                        break;
                    }
                    case ID_DIY_CONTROLLER:
                        RefereeDataCRC16Deal(&referee_data_.DIY_control, &referee_data_.RefereeData[i],
                                            DATA_DIY_CONTROLLER_LEN);
                        i = AdvanceIndex(i, DATA_DIY_CONTROLLER_LEN);
                        break;
                    case ID_MAP_DOWNLOAD:
                        RefereeDataCRC16Deal(&referee_data_.map_download_, &referee_data_.RefereeData[i],
                                            DATA_MAP_DOWNLOAD_LEN);
                        i = AdvanceIndex(i, DATA_MAP_DOWNLOAD_LEN);
                        break;
                    case ID_MAP_RECEIVE:
                        RefereeDataCRC16Deal(&referee_data_.map_receive_, &referee_data_.RefereeData[i],
                                            DATA_MAP_RECEIVE_LEN);
                        i = AdvanceIndex(i, DATA_MAP_RECEIVE_LEN);
                        break;
                    case ID_CUSTOM_CLIENT:
                        RefereeDataCRC16Deal(&referee_data_.custom_client_, &referee_data_.RefereeData[i],
                                            DATA_CUSTOM_CLIENT_LEN);
                        i = AdvanceIndex(i, DATA_CUSTOM_CLIENT_LEN);
                        break;
                    case ID_MAP_PATH:
                        RefereeDataCRC16Deal(&referee_data_.map_path_, &referee_data_.RefereeData[i],
                                            DATA_MAP_PATH_LEN);
                        i = AdvanceIndex(i, DATA_MAP_PATH_LEN);
                        break;
                    case ID_CUSTOM_MESSAGE:
                        RefereeDataCRC16Deal(&referee_data_.custom_message_, &referee_data_.RefereeData[i],
                                            DATA_CUSTOM_MESSAGE_LEN);
                        i = AdvanceIndex(i, DATA_CUSTOM_MESSAGE_LEN);
                        break;
                    case ID_CUSTOM_TO_ROBOT:
                        RefereeDataCRC16Deal(&referee_data_.custom_to_robot_, &referee_data_.RefereeData[i],
                                            DATA_CUSTOM_TO_ROBOT_LEN);
                        i = AdvanceIndex(i, DATA_CUSTOM_TO_ROBOT_LEN);
                        break;
                    case ID_CUSTOM_BYTE_BLOCK:
                        RefereeDataCRC16Deal(&referee_data_.custom_byte_block_,
                                            &referee_data_.RefereeData[i],
                                            DATA_CUSTOM_BYTE_BLOCK_LEN);
                        i = AdvanceIndex(i, DATA_CUSTOM_BYTE_BLOCK_LEN);
                        break;
                    case ID_SET_VIDEO_CHANNEL:
                        RefereeDataCRC16Deal(&referee_data_.set_channel_, &referee_data_.RefereeData[i],
                                            DATA_SET_CHANNEL_LEN);
                        i = AdvanceIndex(i, DATA_SET_CHANNEL_LEN);
                        break;
                    case ID_QUERY_VIDEO_CHANNEL:
                        RefereeDataCRC16Deal(&referee_data_.query_channel_, &referee_data_.RefereeData[i],
                                            DATA_QUERY_CHANNEL_LEN);
                        i = AdvanceIndex(i, DATA_QUERY_CHANNEL_LEN);
                        break;
                    default:
                        break;
                }
            }
        }
        else if (i != (referee_data_.DataLen - 1) && referee_data_.RefereeData[i] == 0xA9 && referee_data_.RefereeData[i + 1] == 0x53)// VT13遥控数据解析
        {   
            if (Verify_CRC16_Check_Sum(&referee_data_.RefereeData[i], 21) == 1) 
            {   
                memcpy(&this->vt13_data_, &referee_data_.RefereeData[i], 21);
                vt13_data_.ch[0] = ((int16_t) vt13_data_.ch_0) - VT13_CH_VALUE_OFFSET;
                vt13_data_.ch[1] = ((int16_t) vt13_data_.ch_1) - VT13_CH_VALUE_OFFSET;
                vt13_data_.ch[2] = ((int16_t) vt13_data_.ch_2) - VT13_CH_VALUE_OFFSET;
                vt13_data_.ch[3] = ((int16_t) vt13_data_.ch_3) - VT13_CH_VALUE_OFFSET;
                vt13_data_.ch[4] = ((int16_t) vt13_data_.wheel) - VT13_CH_VALUE_OFFSET;
                TC_Safe->Online();
            }
        }
    }
}
#endif

} // namespace Referee_n
