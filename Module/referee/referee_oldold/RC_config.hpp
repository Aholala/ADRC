#ifndef __RC_CONFIG_HPP__
#define __RC_CONFIG_HPP__

#include "referee_config.hpp"
#include <stdint.h>
#include <string.h>


#ifdef STM32F405xx
#include "bsp_usart_F4.hpp"
#include "stm32f4xx_hal.h"

#elif STM32H723xx
#include "bsp_usart_H7.hpp"
#include "stm32h7xx_hal.h"

#endif
// Dr16的数据来源：Dt7串口接收机，图传通道(TC)，接收转发
#define RECIVE_DT7_CONTROL
#define RECIVE_VTM_CONTROL
// #define RECIVE_REFFEREE
#define SEND_CONTROL_FORWARD
#define RECIVE_CONTROL_FORWARD

#define FORWARD_CANID 0X01
#define FORWAED_CAN hcan2
#define FORWAED_BSPCAN bsp_can_e::Bsp_Can2

//串口宏定义
#ifdef RECIVE_DT7_CONTROL
#ifdef STM32H723xx
#define DT7_USART huart5
#elif STM32F405xx
#define DT7_USART huart3
#endif
#endif

#ifdef RECIVE_REFFEREE
#ifdef STM32H723xx
#define REFFEREE_USART huart2
#elif STM32F405xx
#define REFFEREE_USART huart1
#endif
#endif

#ifdef RECIVE_VTM_CONTROL
#ifdef STM32H723xx
#define VTM_USART huart2
#elif STM32F405xx
#define VTM_USART huart1
#endif
#endif

/* 遥控文档内容-BEGIN */
/* ----------------------- RC Channel Definition---------------------------- */
#define RC_CH_VALUE_MIN ((uint16_t) 364)    // 通道最小值
#define RC_CH_VALUE_OFFSET ((uint16_t) 1024)// 通道中间值
#define RC_CH_VALUE_MAX ((uint16_t) 1684)   // 通道最大值
/* ----------------------- RC Switch Definition----------------------------- */
#define RC_SW_ERROR ((uint16_t) 0)// 出现严重错误
#define RC_SW_UP ((uint16_t) 1)
#define RC_SW_MID ((uint16_t) 3)
#define RC_SW_DOWN ((uint16_t) 2)
/* ----------------------- PC Key Definition-------------------------------- */
#define KEY_PRESSED_OFFSET_W ((uint16_t) 1 << 0)
#define KEY_PRESSED_OFFSET_S ((uint16_t) 1 << 1)
#define KEY_PRESSED_OFFSET_A ((uint16_t) 1 << 2)
#define KEY_PRESSED_OFFSET_D ((uint16_t) 1 << 3)
#define KEY_PRESSED_OFFSET_SHIFT ((uint16_t) 1 << 4)
#define KEY_PRESSED_OFFSET_CTRL ((uint16_t) 1 << 5)
#define KEY_PRESSED_OFFSET_Q ((uint16_t) 1 << 6)
#define KEY_PRESSED_OFFSET_E ((uint16_t) 1 << 7)
#define KEY_PRESSED_OFFSET_R ((uint16_t) 1 << 8)
#define KEY_PRESSED_OFFSET_F ((uint16_t) 1 << 9)
#define KEY_PRESSED_OFFSET_G ((uint16_t) 1 << 10)
#define KEY_PRESSED_OFFSET_Z ((uint16_t) 1 << 11)
#define KEY_PRESSED_OFFSET_X ((uint16_t) 1 << 12)
#define KEY_PRESSED_OFFSET_C ((uint16_t) 1 << 13)
#define KEY_PRESSED_OFFSET_V ((uint16_t) 1 << 14)
#define KEY_PRESSED_OFFSET_B ((uint16_t) 1 << 15)
/* ----------------------- Data Struct ------------------------------------- */
#define Sbus_RX_Buffer_Num 36
#define REFEREE_RX_Buffer_Num 255
#define RC_FRAME_LENGTH 18
#define RC_DEAD_LINE 5

/* DT7接收数据结构体*/
/**
 * @brief 遥控键鼠数据结构体
 * @details 新增支持VT13接收端的数据，包括一些新按键
 * @details dr16波轮融合了VT13的波轮（考虑到这两玩意不会同时使用）
 */
typedef struct
{
    struct
    {
        int16_t ch[5];
        uint8_t s1;
        uint8_t s2;
    } rc;
    struct
    {
        int16_t x;
        int16_t y;
        int16_t z;
        uint8_t press_l;
        uint8_t press_r;
        uint8_t press_m;
    } mouse;
    /* keyboard key information */
    union {
        uint16_t key_code;
        struct
        {
            uint16_t W : 1;
            uint16_t S : 1;
            uint16_t A : 1;
            uint16_t D : 1;
            uint16_t SHIFT : 1;
            uint16_t CTRL : 1;
            uint16_t Q : 1;
            uint16_t E : 1;
            uint16_t R : 1;
            uint16_t F : 1;
            uint16_t G : 1;
            uint16_t Z : 1;// uint16_t M : 1;
            uint16_t X : 1;// uint16_t N : 1;
            uint16_t C : 1;
            uint16_t V : 1;
            uint16_t B : 1;
        } bit;
    } kb;
    union {
        uint8_t vtm_code;
        struct
        {
            uint64_t mode_sw : 2;//接收端挡位切换开关位置：C:0, N:1, S:2
            uint64_t pause : 1;  //接收端暂停按键
            uint64_t fn_1 : 1;   //接收端自定义按键（左）
            uint64_t fn_2 : 1;   //接收端自定义按键（右）
            // uint64_t wheel : 11;    //接收端波轮位置
            uint64_t trigger : 1;//接收端扳机键
        } button;
    } vtm;

    int8_t Flag;
} RC_ctrl_t;

#define VTM_FRAME_LENGTH 21

/*VT13 接收数据结构体*/
typedef __packed struct
{
    uint8_t sof_1;
    uint8_t sof_2;
    uint64_t ch_0 : 11;
    uint64_t ch_1 : 11;
    uint64_t ch_2 : 11;
    uint64_t ch_3 : 11;
    uint64_t mode_sw : 2;//接收端挡位切换开关位置：C:0, N:1, S:2
    uint64_t pause : 1;  //接收端暂停按键
    uint64_t fn_1 : 1;   //接收端自定义按键（左）
    uint64_t fn_2 : 1;   //接收端自定义按键（右）
    uint64_t wheel : 11; //接收端波轮位置
    uint64_t trigger : 1;//接收端扳机键

    int16_t mouse_x;
    int16_t mouse_y;
    int16_t mouse_z;
    uint8_t mouse_left : 2;
    uint8_t mouse_right : 2;
    uint8_t mouse_middle : 2;
    uint16_t key;
    uint16_t crc16;

    uint8_t data[256];
    int16_t dataLen;
    int16_t ch[5];
} remote_data_t;

typedef union {
    struct
    {
        uint16_t ch0 : 11;
        uint16_t ch1 : 11;
        uint16_t rs2 : 2;
        uint16_t mouseX : 11;
        uint16_t mouseY : 11;
        uint16_t mouseR : 1;
        uint16_t mouseL_And_ch4_Set : 1;
        uint16_t key_code : 16;
    } Struct;
    uint8_t U8[8];
} Forward_ctrl_t;


#endif