#ifndef ROBO_CMD_HPP
#define ROBO_CMD_HPP

#ifdef USE_DR16
#include "dr16.hpp"
#endif
#ifdef USE_REFEREE
#include "referee.hpp"
#endif
#ifdef USE_DEBUG_LIGHT
#include "debug_light.hpp"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "string.h"
#include "CRC.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
}

// #include "bsp_fdcan.hpp"
// #include "bsp_can.hpp"

class RoboCmd_c {
public:
    typedef struct
    {
        uint8_t *referee = nullptr;
        uint8_t *photo = nullptr;
        uint8_t *dr16 = nullptr;
    } OnlineFlag_t;
    OnlineFlag_t onlineflag_;

#ifdef USE_DR16
    Dr16_n::dr16_c *DR16 = nullptr;
    Dr16_n::DT7Ctrl_t *dt7_data_;
    Dr16_n::DT7Ctrl_t *GetDT7data()
    {
        return &DR16->dt7_data_;
    }
#ifdef USE_DEBUG_LIGHT

    Light_n::Debug_c* dr16_light_trigger_ = nullptr;

#endif // USE_DEBUG_LIGHT
#endif // USE_DR16
#if defined(USE_REFEREE)
    Referee_n::referee_c *REFEREE = nullptr;
    Referee_n::PHOTO_ctrl_t *key_data_;
    Referee_n::VT13data_t *vt13_data_;
    Referee_n::RefereeData_t *GetREdata()
    {
        return &REFEREE->referee_data_;
    }
    Referee_n::VT13data_t *GetVT13data()
    {
        return &REFEREE->vt13_data_;
    }
#ifdef USE_DEBUG_LIGHT
    Light_n::Debug_c* referee_light_trigger_ = nullptr;
    Light_n::Debug_c* vt13_light_trigger_ = nullptr;
#endif // USE_DEBUG_LIGHT
#endif // USE_REFEREE

    void RoboCmdInit();
    static RoboCmd_c *GetInstance()
    {
        static RoboCmd_c robocmd_instance_;//唯一实例指针
        return &robocmd_instance_; // 返回静态成员变量 instance 的地址
    }
protected:
    RoboCmd_c() = default; // 私有构造函数

    // void SendDr16ToOther();
    // void SendKeyToOther();
    // H7
    // BSP_n::Fdcan_c *Dr16_can1; // dr16 12个字节拆成两帧
    // BSP_n::Fdcan_c *Dr16_can2;
    // BSP_n::Fdcan_c *Key_can1; // 键盘 13个字节拆成两帧
    // BSP_n::Fdcan_c *Key_can2;
    // F4
    // BSP_n::Can_c *Dr16_can1; // dr16 12个字节拆成两帧
    // BSP_n::Can_c *Dr16_can2;
    // BSP_n::Can_c *Key_can1; // 键盘 13个字节拆成两帧
    // BSP_n::Can_c *Key_can2;
};
// H7
// static void DealDr16FromOther(BSP_n::Fdcan_c* register_instance);
// static void DealKeyFromOther(BSP_n::Fdcan_c* register_instance);
// F4
// static void DealDr16FromOther(BSP_n::Can_c* register_instance);
// static void DealKeyFromOther(BSP_n::Can_c* register_instance);
#endif

#endif
