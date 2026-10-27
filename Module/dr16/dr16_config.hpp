#ifndef DR16_CONFIG_HPP
#define DR16_CONFIG_HPP
#include <cstdint>

namespace Dr16_n {
    /* 遥控文档内容-BEGIN */
    /* ----------------------- DT7 Channel Definition---------------------------- */
#define DT7_CH_VALUE_MIN ((uint16_t) 364)    // 通道最小值
#define DT7_CH_VALUE_OFFSET ((uint16_t) 1024)// 通道中间值
#define DT7_CH_VALUE_MAX ((uint16_t) 1684)   // 通道最大值
    /* ----------------------- DT7 Switch Definition----------------------------- */
#define DT7_SW_ERROR ((uint16_t) 0)// 出现严重错误
#define DT7_SW_UP ((uint16_t) 1)
#define DT7_SW_MID ((uint16_t) 3)
#define DT7_SW_DOWN ((uint16_t) 2)
#define DT7_R_JOY_LR 0x00U
#define DT7_R_JOY_UD 0x01U
#define DT7_L_JOY_LR 0x02U
#define DT7_L_JOY_UD 0x03U
#define DT7_RATCHET  0x04U
    /* ----------------------- Data Struct ------------------------------------- */
#define Sbus_RX_Buffer_Num 36
#define DT7_FRAME_LENGTH 18
#define DT7_DEAD_LINE 5

    /**
     * @brief DT7接收数据结构体
     */
    typedef struct
    {
        int16_t ch[5];
        uint8_t s1;
        uint8_t s2;
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
    } DT7Ctrl_t;

}


#endif
