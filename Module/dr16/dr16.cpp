/*************************** Dongguan-University of Technology -ACE**************************
 * @file    dr16.cpp
 * @author  zhengNannnn Lann coconutjuice 
 * @version v26.1.4.2
 * @date    2025-11-29
 * @brief   dr16模块文件
 ******************************************************************************
 * @verbatim
 * 
 * @attention
 *      依赖文件：
            USART
            safe_task
            dr16_config.hpp
 * @version           time
 * v26.0.0.0  全新的体验  2025-11-29    已测试
 * v26.0.1.0  DT7掉线拨杆值置0，错误状态 2025-12-01
 * v26.1.4.2  补回dr16的键鼠数据 2026-1-23
 ************************** Dongguan-University of Technology -ACE***************************/
#include "dr16.hpp"
#if defined(USE_JLINK_RTT)
#include "bsp_jlink_rtt.hpp"
#endif

#define ABS(NUM) (((NUM) > 0 ? (NUM) : (-NUM)))

namespace Dr16_n {

    dr16_c dr16_c::dr16_instance_ = dr16_c();
    static bool s_dr16_disconnected_latched = false;

#if defined(USE_JLINK_RTT) && defined(ECF_RTT_LOG_ENABLE_DR16) && (ECF_RTT_LOG_ENABLE_DR16)
#define DR16_LOG_I(fmt, ...) LOG_RTT_I("[DR16][LINK] " fmt __VA_OPT__(,) __VA_ARGS__)
#define DR16_LOG_W(fmt, ...) LOG_RTT_W("[DR16][FRAME] " fmt __VA_OPT__(,) __VA_ARGS__)
#define DR16_LOG_E(fmt, ...) LOG_RTT_E("[DR16][LINK] " fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define DR16_LOG_I(fmt, ...) ((void)0)
#define DR16_LOG_W(fmt, ...) ((void)0)
#define DR16_LOG_E(fmt, ...) ((void)0)
#endif

    static void dt7_data_Online()
    {
        if (s_dr16_disconnected_latched)
        {
            DR16_LOG_I("recovered");
            s_dr16_disconnected_latched = false;
        }
        dr16_c::GetInstance()->is_online_ = 1;
    }

    static void DT7Disonline(void)
    {
        if (!s_dr16_disconnected_latched)
        {
            DR16_LOG_E("disconnected timeout=%llums", 200ull);
            s_dr16_disconnected_latched = true;
        }
        dr16_c::GetInstance()->Dt7Clear();
    }

    /*dr16回调函数*/
    void DT7callback(const uint8_t *pdata, uint16_t psize)
    {
        if (psize == DT7_FRAME_LENGTH) {
            dr16_c::GetInstance()->Dt7DataProcess(pdata, psize);
        }
        else
        {
            DR16_LOG_W("invalid_frame_size size=%u expected=%u",
                       static_cast<unsigned>(psize),
                       static_cast<unsigned>(DT7_FRAME_LENGTH));
        }
    }

    void dr16_c::Dr16Init()
    {
        // dr16串口配置
        BSP_n::USART_c::Config dr16_uart_param = {
            .Handle = &DT7_USART,
            .RxBuffSize = DT7_FRAME_LENGTH * 2,      // 两块单帧接收区的总大小
            .RxType = BSP_n::USART_c::RxType_t::DMA_IDLE_DOUBLE,// DMA双缓冲
            .TxType = BSP_n::USART_c::TxType_t::DMA,// 发送类型
            .pTxCallback = nullptr,// 发送回调,初始化都为nullptr
            .pRxCallback = &DT7callback,
            .expected_data_length = DT7_FRAME_LENGTH,
            .pRxBuffer = sbus_rx_buffer_[0],
            .pSecRxBuffer = sbus_rx_buffer_[1]};
        dr16 = new BSP_n::USART_c(dr16_uart_param);
        dt7_safe = new SafeTask_c("DT7", 200, DT7Disonline, dt7_data_Online);
        DR16_LOG_I("init ok uart=%s frame_len=%u timeout_ms=%u",
                   "DT7_USART",
                   static_cast<unsigned>(DT7_FRAME_LENGTH),
                   200u);
    }

    void dr16_c::Dt7Clear(void)
    {
        memset(this->dt7_data_.ch, 0, sizeof(int16_t) * 5);
        this->is_online_ = 0;
        this->dt7_data_.s1 = Clear_DT7_SW_R;
        this->dt7_data_.s2 = Clear_DT7_SW_L;
    }

    /**
     * @brief DT7数据解析
     * @note  一次转发
     */
    void dr16_c::Dt7DataProcess(const uint8_t *pData, uint16_t psize)
    {
        uint8_t dt7_data_buff[DT7_FRAME_LENGTH] = {0};
        memcpy(&dt7_data_buff, pData, psize);

        // uint8_t *pData = Sbus_RX_Buffer_[idx];
        this->dt7_data_.ch[0] = ((int16_t) dt7_data_buff[0] | ((int16_t) dt7_data_buff[1] << 8)) &
                             0x07FF;//!< Channel 0
        this->dt7_data_.ch[1] =
                (((int16_t) dt7_data_buff[1] >> 3) | ((int16_t) dt7_data_buff[2] << 5)) &
                0x07FF;//!< Channel 1
        this->dt7_data_.ch[2] =
                (((int16_t) dt7_data_buff[2] >> 6) | ((int16_t) dt7_data_buff[3] << 2) |//!< Channel 2
                 ((int16_t) dt7_data_buff[4] << 10)) &
                0x07FF;

        this->dt7_data_.ch[3] =
                (((int16_t) dt7_data_buff[4] >> 1) | ((int16_t) dt7_data_buff[5] << 7)) &
                0x07FF;//!< Channel 3

        this->dt7_data_.s1 = ((dt7_data_buff[5] >> 4) & 0x000C) >> 2;//!< Switch left
        this->dt7_data_.s2 = ((dt7_data_buff[5] >> 4) & 0x0003);     //!< Switch right

        this->dt7_data_.ch[4] =
                ((int16_t) dt7_data_buff[16]) | ((int16_t) dt7_data_buff[17] << 8);// 左上角滑轮

        this->dt7_data_.ch[0] -= DT7_CH_VALUE_OFFSET;
        this->dt7_data_.ch[1] -= DT7_CH_VALUE_OFFSET;
        this->dt7_data_.ch[2] -= DT7_CH_VALUE_OFFSET;
        this->dt7_data_.ch[3] -= DT7_CH_VALUE_OFFSET;
        this->dt7_data_.ch[4] -= DT7_CH_VALUE_OFFSET;

        // 死区限制
        this->dt7_data_.ch[0] =
                (ABS(this->dt7_data_.ch[0]) < this->deadline_limit_[0] ? 0
                                                              : this->dt7_data_.ch[0]);
        this->dt7_data_.ch[1] =
                (ABS(this->dt7_data_.ch[1]) < this->deadline_limit_[1] ? 0
                                                              : this->dt7_data_.ch[1]);
        this->dt7_data_.ch[2] =
                (ABS(this->dt7_data_.ch[2]) < this->deadline_limit_[2] ? 0
                                                              : this->dt7_data_.ch[2]);
        this->dt7_data_.ch[3] =
                (ABS(this->dt7_data_.ch[3]) < this->deadline_limit_[3] ? 0
                                                              : this->dt7_data_.ch[3]);
        this->dt7_data_.ch[4] =
                (ABS(this->dt7_data_.ch[4]) < this->deadline_limit_[4] ? 0
                                                                  : this->dt7_data_.ch[4]);

        this->dt7_data_.mouse.x =
            ((int16_t) dt7_data_buff[6]) | ((int16_t) dt7_data_buff[7] << 8);//!< Mouse X axis
        this->dt7_data_.mouse.y =
                ((int16_t) dt7_data_buff[8]) | ((int16_t) dt7_data_buff[9] << 8);//!< Mouse Y axis
        this->dt7_data_.mouse.z =
                ((int16_t) dt7_data_buff[10]) | ((int16_t) dt7_data_buff[11] << 8);//!< Mouse Z axis

        this->dt7_data_.mouse.press_l = dt7_data_buff[12];//!< Mouse Left Is Press ?
        this->dt7_data_.mouse.press_r = dt7_data_buff[13];//!< Mouse Right Is Press ?

        this->dt7_data_.kb.key_code = dt7_data_buff[14] | (dt7_data_buff[15] << 8);//!< KeyBoard value
        dt7_safe->Online();
    }
}

