/*************************** Dongguan-University of Technology -ACE**************************
 * @file    robo_cmd.cpp
 * @author  zhengNannnn Lann coconutjuice 
 * @version v26.1.4.2
 * @date    2025-11-29
 * @brief   板间通信文件
 ******************************************************************************
 * @verbatim
 * 
 * @attention
 *      依赖文件：
            CAN
            dr16
            referee
            debug_light(可选)
 * @version           time
 * v26.0.0.0  全新的体验  2025-11-29    已测试
 * v26.1.4.2  补回dr16的键鼠数据 2026-1-23
 * v26.2.1.0  添加失联灯语,可以调用****_light_trigger_->Disable/Enable()开关灯语触发 2026-3-3
 ************************** Dongguan-University of Technology -ACE***************************/
#include "robo_cmd.hpp"
#include <cstdio>
/**
 * @brief 总初始化接口
 */
void RoboCmd_c::RoboCmdInit() {
#ifdef USE_DR16
    DR16 = Dr16_n::dr16_c::GetInstance();
    DR16->Dr16Init();
    dt7_data_ = &DR16->dt7_data_;
    onlineflag_.dr16 = &DR16->is_online_;
#ifdef USE_DEBUG_LIGHT
    Light_n::Effect_t dr16_a_light = Light_n::USARTLOST_A_EFFECT;
    Light_n::Effect_t dr16_b_light = Light_n::DR16_B_EFFECT;
    std::snprintf(dr16_a_light.name, sizeof(dr16_a_light.name), "usA_dr16");
    std::snprintf(dr16_b_light.name, sizeof(dr16_b_light.name), "dr16_B");
    Light_n::Effect_t dr16_lost_light[2] = {dr16_a_light, dr16_b_light};
    dr16_light_trigger_ = new Light_n::Debug_c(dr16_lost_light, (bool*)onlineflag_.dr16);
    dr16_light_trigger_->Enable();
#endif // USE_DEBUG_LIGHT
#endif // USE_DR16
#ifdef USE_REFEREE
    REFEREE = Referee_n::referee_c::GetInstance();
    REFEREE->RefereeInit();
    key_data_ = &REFEREE->referee_data_.PHOTO_ctrl;
    onlineflag_.referee = &REFEREE->is_online_re_;
    vt13_data_ = &REFEREE->vt13_data_;
    onlineflag_.photo = &REFEREE->is_online_tc_;
#ifdef USE_DEBUG_LIGHT
    Light_n::Effect_t referee_a_light = Light_n::USARTLOST_A_EFFECT;
    Light_n::Effect_t referee_b_light = Light_n::POWCONF_B_EFFECT;
    std::snprintf(referee_a_light.name, sizeof(referee_a_light.name), "usA_ref");
    std::snprintf(referee_b_light.name, sizeof(referee_b_light.name), "powcf_B");
    Light_n::Effect_t referee_lost_light[2] = {referee_a_light, referee_b_light};
    referee_light_trigger_ = new Light_n::Debug_c(referee_lost_light, (bool*)onlineflag_.referee);
    referee_light_trigger_->Enable();
    Light_n::Effect_t vt13_a_light = Light_n::USARTLOST_A_EFFECT;
    Light_n::Effect_t vt13_b_light = Light_n::VT13_B_EFFECT;
    std::snprintf(vt13_a_light.name, sizeof(vt13_a_light.name), "usA_vt13");
    std::snprintf(vt13_b_light.name, sizeof(vt13_b_light.name), "vt13_B");
    Light_n::Effect_t vt13_lost_light[2] = {vt13_a_light, vt13_b_light};
    vt13_light_trigger_ = new Light_n::Debug_c(vt13_lost_light, (bool*)onlineflag_.photo);
    vt13_light_trigger_->Enable();
#endif // USE_DEBUG_LIGHT
#endif // USE_REFEREE
    // 我是H7发送端
    // Dr16_can1 = new BSP_n::Fdcan_c(&hfdcan2, 0x555, 0x403, FDCAN_STANDARD_ID, DealDr16FromOther);
    // Dr16_can2 = new BSP_n::Fdcan_c(&hfdcan2, 0x556, 0x404, FDCAN_STANDARD_ID, DealDr16FromOther);
    // Key_can1 = new BSP_n::Fdcan_c(&hfdcan2, 0x557, 0x405, FDCAN_STANDARD_ID, DealKeyFromOther);
    // Key_can2 = new BSP_n::Fdcan_c(&hfdcan2, 0x558, 0x406, FDCAN_STANDARD_ID, DealKeyFromOther);
    // 我是H7接收端
    // Dr16_can1 = new BSP_n::Fdcan_c(&hfdcan2, 0x403, 0x555, FDCAN_STANDARD_ID, DealDr16FromOther);
    // Dr16_can2 = new BSP_n::Fdcan_c(&hfdcan2, 0x404, 0x556, FDCAN_STANDARD_ID, DealDr16FromOther);
    // Key_can1 = new BSP_n::Fdcan_c(&hfdcan2, 0x405, 0x557, FDCAN_STANDARD_ID, DealKeyFromOther);
    // Key_can2 = new BSP_n::Fdcan_c(&hfdcan2, 0x406, 0x558, FDCAN_STANDARD_ID, DealKeyFromOther);
    // 我是F4发送端
    // Dr16_can1 = new BSP_n::Can_c(&hcan2, 0x555, 0x403, CAN_ID_STD, DealDr16FromOther);
    // Dr16_can2 = new BSP_n::Can_c(&hcan2, 0x556, 0x404, CAN_ID_STD, DealDr16FromOther);
    // Key_can1 = new BSP_n::Can_c(&hcan2, 0x557, 0x405, CAN_ID_STD, DealKeyFromOther);
    // Key_can2 = new BSP_n::Can_c(&hcan2, 0x558, 0x406, CAN_ID_STD, DealKeyFromOther);
    //我是F4接收端
    // Dr16_can1 = new BSP_n::Can_c(&hcan2, 0x403, 0x555, CAN_ID_STD, DealDr16FromOther);
    // Dr16_can2 = new BSP_n::Can_c(&hcan2, 0x404, 0x556, CAN_ID_STD, DealDr16FromOther);
    // Key_can1 = new BSP_n::Can_c(&hcan2, 0x405, 0x557, CAN_ID_STD, DealKeyFromOther);
    // Key_can2 = new BSP_n::Can_c(&hcan2, 0x406, 0x558, CAN_ID_STD, DealKeyFromOther);
}

/**
 * @brief robocmd主循环
 */
void robot_cmd(void const * argument)
{
    while (1)
    {
        // RoboCmd_c::GetInstance()->SendDr16ToOther();
        // RoboCmd_c::GetInstance()->SendKeyToOther();
        //...自行设计你要的通信
        vTaskDelay(1);
    }
}

/**
 * @brief dr16数据发送到另一个板子
 */
// void RoboCmd_c::SendDr16ToOther()
// {
    // memcpy(Dr16_can1->tx_buff_, &DR16->dt7_data_.ch,8);
    // Dr16_can1->Transmit(0.01);
    // memcpy(Dr16_can2->tx_buff_, &DR16->dt7_data_.ch[4],2);
    // Dr16_can2->tx_buff_[2] = DR16->dt7_data_.s1;
    // Dr16_can2->tx_buff_[3] = DR16->dt7_data_.s2;
    // Dr16_can2->Transmit(0.01);
// }

/**
 * @brief 图传键鼠数据发送到另一个板子
 */
// void RoboCmd_c::SendKeyToOther()
// {
    // memcpy(Key_can1->tx_buff_, &REFEREE->referee_data_.PHOTO_ctrl.mouse,8);
    // Key_can1->Transmit(0.01);
    // memcpy(Key_can2->tx_buff_, &REFEREE->referee_data_.PHOTO_ctrl.kb,2);
    // memcpy(&Key_can2->tx_buff_[2], &REFEREE->referee_data_.PHOTO_ctrl.other, 2);
    // Key_can2->tx_buff_[4] = REFEREE->referee_data_.PHOTO_ctrl.error;
    // Key_can2->Transmit(0.01);
// }

/**
 * @brief 接收另一个板子dr16数据
 */
// #ifdef STM32H723xx
// static void DealDr16FromOther(BSP_n::Fdcan_c* register_instance)
// {
// #else
// static void DealDr16FromOther(BSP_n::Can_c* register_instance)
// {
// #endif
// 
    // uint8_t *rxbuff;
    // RoboCmd_c *robot_instance = RoboCmd_c::GetInstance();
    // rxbuff = register_instance->rx_buff_;
    // if (register_instance->rx_id_ == 0x555)
    // {
        // memcpy(&robot_instance->dt7_data_->ch, rxbuff, 8);
    // }
    // else if (register_instance->rx_id_ == 0x556)
    // {
        // memcpy(&robot_instance->dt7_data_->ch[4], rxbuff, 2);
        // robot_instance->dt7_data_->s1 = rxbuff[2];
        // robot_instance->dt7_data_->s2 = rxbuff[3];
    // }
// }

/**
 * @brief 接收另一个板子图传键鼠数据
 */
// #ifdef STM32H723xx
// static void DealKeyFromOther(BSP_n::Fdcan_c* register_instance)
// {
// #else
// static void DealKeyFromOther(BSP_n::Can_c* register_instance)
// {
// #endif

    // uint8_t *rxbuff;
    // RoboCmd_c *robot_instance = RoboCmd_c::GetInstance();
    // rxbuff = register_instance->rx_buff_;
    // if (register_instance->rx_id_ == 0x557)
    // {
        // memcpy(&robot_instance->key_data_->mouse, rxbuff, 8);
    // }
    // else if (register_instance->rx_id_ == 0x558)
    // {
        // memcpy(&robot_instance->key_data_->kb, rxbuff, 2);
        // memcpy(&robot_instance->key_data_->other, &rxbuff[2], 2);
        // robot_instance->key_data_->error = rxbuff[4];
    // }
// }


