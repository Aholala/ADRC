/*************************** Dongguan-University of Technology -ACE**************************
 * @file    ws2812.cpp
 * @author  coconutjuice & L_Zero
 * @brief   ws2812多灯珠驱动
 ******************************************************************************
 * @verbatim
 * * 详细使用看md文件
 * @attention
 * * 使用SPI需要配置 波特率6.4MHz(H7) / 波特率10.5MHz(F4)
 * * 使用PWM需要配置 TIM周期1.25us
 * @version & @date & @author
 * * v26.0.0.0   spi驱动ws2812 ---- %/%/% ---- coconutjuice
 * * v26.1.0.0   加入接口和功能，先支持多种灯效，适用于H704.04.02代主控 ---- 2026/1/13 ---- coconutjuice
 * * v26.2.0.0   重构，将灯效与驱动解耦，灯效有关移植到其他文件，添加PWM通讯，经排查把复位放前面修复了某个问题，于是删除了FUCK_HAL_SPI ---- 2026/1/30 ---- L_Zero
 * * v26.2.1.0   适配32寄存器，0/1码定义CCR改为ARR的占比 ---- 2026/2/2 ---- L_Zero
 * * v26.3.0.0   适配F4的逆天SPI ---- 2026/3/8 ---- L_Zero
 ************************** Dongguan-University of Technology -ACE***************************/

#include "ws2812.hpp"
#include "user_maths.hpp"
#include <cstring>
#include <cmath>

// 复位：低电平时间 > 80us

namespace Ws2812_n
{
#define CCR_PRC_0   .24f  // 编码 0 : 24% * 1.25us
#define CCR_PRC_1   .48f  // 编码 1 : 48% * 1.25us
#ifdef STM32H7
// SPI 6.4MHz 时（1Byte≈1.25us）
#define CODE_0      0xC0  // 编码 0 : 11000000 // 编码 0 : 2 / 8 * 1.25us
#define CODE_1      0xF8  // 编码 1 : 11111000 // 编码 1 : 5 / 8 * 1.25us
#define CODE_BYTE   1.0f
#elifdef STM32F4
// SPI 10.5MHz 时（1Byte≈0.76us）
#define CODE_0      0x0E00  // 编码 0 : 0000 111000000000 // 编码 0 : 3 / 12 * 1.14us
#define CODE_1      0x0FE0  // 编码 1 : 0000 111111100000 // 编码 1 : 7 / 12 * 1.14us
#define CODE_BYTE   1.5f
#endif

#define RGB_BIT     24    // RGB bit
#define RESET_BYTES 72    // 复位码量

/* region ================================ 实例构造 ================================ */
#if (WS2812_COMMUNICATION_MODE & 0xF0) // PWM
    Ws2812_c::Ws2812_c(PWM_Init_t config) : device_num_(config.device_num)
    {
        BSP_n::PWM_Init_t pwm_config = {
                .htim = config.htim,
                .channel = config.channel,
                .pwm_mode = BSP_n::PWM_DMA_SEND,
        };
        pwm_ = new BSP_n::PWM_c(pwm_config);
        is_spi_not_pwm_ = false;
        if(config.is_32reg)
        {
            reg32t_cof_ = 4;
        }
        else
        {
            reg32t_cof_ = 2;
        }
        pwm_buff_ = new uint8_t[(device_num_ * RGB_BIT + RESET_BYTES) * reg32t_cof_](0);
        ccr0_ = (CCR_PRC_0 * pwm_->PWMGetArr() + 0.5f);
        ccr1_ = (CCR_PRC_1 * pwm_->PWMGetArr() + 0.5f);
    }
    Ws2812_c::Ws2812_c(TIM_HandleTypeDef *htim, uint32_t channel, uint16_t device_num, bool is_32reg) : device_num_(device_num)
    {
        BSP_n::PWM_Init_t pwm_config = {
                .htim = htim,
                .channel = channel,
                .pwm_mode = BSP_n::PWM_DMA_SEND,
        };
        pwm_ = new BSP_n::PWM_c(pwm_config);
        is_spi_not_pwm_ = false;
        if(is_32reg)
        {
            reg32t_cof_ = 4;
        }
        else
        {
            reg32t_cof_ = 2;
        }
        pwm_buff_ = new uint8_t[(device_num_ * RGB_BIT + RESET_BYTES) * reg32t_cof_](0);
        ccr0_ = (CCR_PRC_0 * pwm_->PWMGetArr() + 0.5f);
        ccr1_ = (CCR_PRC_1 * pwm_->PWMGetArr() + 0.5f);
    }
#endif
#if (WS2812_COMMUNICATION_MODE & 0x0F) // SPI
    Ws2812_c::Ws2812_c(SPI_Init_t config) : device_num_(config.device_num)
    {
        spi_ = config.hspi;
        is_spi_not_pwm_ = true;
        spi_buff_ = new uint8_t[(uint16_t)((device_num_ * RGB_BIT + RESET_BYTES) * CODE_BYTE)](0);
    }
    Ws2812_c::Ws2812_c(SPI_HandleTypeDef *hspi, uint16_t device_num) : device_num_(device_num)
    {
        spi_ = hspi;
        is_spi_not_pwm_ = true;
        spi_buff_ = new uint8_t[(uint16_t)((device_num_ * RGB_BIT + RESET_BYTES) * CODE_BYTE)](0);
    }
#endif
// endregion 实例构造

/* region ================================ 工具函数 ================================ */

#define user_round(x) (uint8_t)(x + 0.5f)
    // 标准RGB转HSV (R: 255，G: 255，B: 255)
    void RGB_2_HSV(const RGB_t* rgb, HSV_t* hsv)
    {
        float r = rgb->R / 255.0f;
        float g = rgb->G / 255.0f;
        float b = rgb->B / 255.0f;

        float max_val = user_max(r, user_max(g, b));
        float min_val = user_max(r, user_max(g, b));
        float delta = max_val - min_val;

        hsv->value = max_val;
        hsv->saturation = (max_val > 0.0f) ? (delta / max_val) : 0.0f;
        if (delta > 0.0f)
        {
            if (max_val == r)
            {
                hsv->hue = 60.0f * fmodf(((g - b) / delta), 6.0f);
            }
            else if (max_val == g)
            {
                hsv->hue = 60.0f * (((b - r) / delta) + 2.0f);
            }
            else
            {
                hsv->hue = 60.0f * (((r - g) / delta) + 4.0f);
            }
            if (hsv->hue < 0.0f)
            {
                hsv->hue += 360.0f;
            }
        }
        else
        {
            hsv->hue = 0.0f;
        }
    }

    // 标准HSV转RGB (H: 0-360°, S: 0-1, V: 0-1)
    void HSV_2_RGB(const HSV_t* hsv, RGB_t* rgb)
    {
        float h = hsv->hue;
        float s = hsv->saturation;
        float v = hsv->value;

        user_value_limit(h, .0f, 360.0f)
        user_value_limit(s, .0f, 1.0f)
        user_value_limit(v, .0f, 1.0f)

        float c = v * s;  // 色度
        float x = c * (1.0f - user_abs(fmodf(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;

        float r1, g1, b1;
        if (h >= 0 && h < 60) {
            r1 = c; g1 = x; b1 = 0;
        } else if (h >= 60 && h < 120) {
            r1 = x; g1 = c; b1 = 0;
        } else if (h >= 120 && h < 180) {
            r1 = 0; g1 = c; b1 = x;
        } else if (h >= 180 && h < 240) {
            r1 = 0; g1 = x; b1 = c;
        } else if (h >= 240 && h < 300) {
            r1 = x; g1 = 0; b1 = c;
        } else { // h >= 300 && h < 360
            r1 = c; g1 = 0; b1 = x;
        }

        rgb->R = user_round((r1 + m) * 255.0f);
        rgb->G = user_round((g1 + m) * 255.0f);
        rgb->B = user_round((b1 + m) * 255.0f);
    }

    HSV_t RGB_2_HSV(const RGB_t* rgb)
    {
        HSV_t hsv;
        RGB_2_HSV(rgb, &hsv);
        return hsv;
    }
    RGB_t HSV_2_RGB(const HSV_t* hsv)
    {
        RGB_t rgb;
        HSV_2_RGB(hsv, &rgb);
        return rgb;
    }

    void Ws2812_c::RGB_2_Buff(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
    {
        if(is_spi_not_pwm_)
        {
#ifdef STM32H7
            uint8_t *pR = spi_buff_ + (index * RGB_BIT + RESET_BYTES + 8);
            uint8_t *pG = spi_buff_ + (index * RGB_BIT + RESET_BYTES);
            uint8_t *pB = spi_buff_ + (index * RGB_BIT + RESET_BYTES + 16);
            for(uint8_t i = 0; i <  8; i++)
            {
                *pG++ = (g & 0x80) ? CODE_1 : CODE_0;
                *pR++ = (r & 0x80) ? CODE_1 : CODE_0;
                *pB++ = (b & 0x80) ? CODE_1 : CODE_0;
                r <<= 1;
                g <<= 1;
                b <<= 1;
            }
#elifdef STM32F4
            // F4需要1.5个uint8_t凑时序
            uint8_t *pR = spi_buff_ + (uint16_t)((index * RGB_BIT + RESET_BYTES + 8) * CODE_BYTE);
            uint8_t *pG = spi_buff_ + (uint16_t)((index * RGB_BIT + RESET_BYTES) * CODE_BYTE);
            uint8_t *pB = spi_buff_ + (uint16_t)((index * RGB_BIT + RESET_BYTES + 16) * CODE_BYTE);
            for(uint8_t i = 0; i < 4; i++)
            {
                *pG++ = (g & 0x80) ? CODE_1 >> 4 : CODE_0 >> 4;
                *pR++ = (r & 0x80) ? CODE_1 >> 4 : CODE_0 >> 4;
                *pB++ = (b & 0x80) ? CODE_1 >> 4 : CODE_0 >> 4;
                *pG = (g & 0x80) ? CODE_1 << 4 & 0xF0: CODE_0 << 4 & 0xF0;
                *pR = (r & 0x80) ? CODE_1 << 4 & 0xF0: CODE_0 << 4 & 0xF0;
                *pB = (b & 0x80) ? CODE_1 << 4 & 0xF0: CODE_0 << 4 & 0xF0;
                r <<= 1;
                g <<= 1;
                b <<= 1;
                *pG++ |= (g & 0x80) ? CODE_1 >> 8 : CODE_0 >> 8;
                *pR++ |= (r & 0x80) ? CODE_1 >> 8 : CODE_0 >> 8;
                *pB++ |= (b & 0x80) ? CODE_1 >> 8 : CODE_0 >> 8;
                *pG++ = (g & 0x80) ? CODE_1 : CODE_0;
                *pR++ = (r & 0x80) ? CODE_1 : CODE_0;
                *pB++ = (b & 0x80) ? CODE_1 : CODE_0;
                r <<= 1;
                g <<= 1;
                b <<= 1;
            }
#endif
        }
        else
        {
            uint8_t *pR = pwm_buff_ + ((index * RGB_BIT + RESET_BYTES + 8) * reg32t_cof_);
            uint8_t *pG = pwm_buff_ + ((index * RGB_BIT + RESET_BYTES) * reg32t_cof_);
            uint8_t *pB = pwm_buff_ + ((index * RGB_BIT + RESET_BYTES + 16) * reg32t_cof_);
            for(uint8_t i = 0; i < 8; i++)
            {
                *pG = (g & 0x80) ? ccr1_ : ccr0_;
                *pR = (r & 0x80) ? ccr1_ : ccr0_;
                *pB = (b & 0x80) ? ccr1_ : ccr0_;
                pG += reg32t_cof_;
                pR += reg32t_cof_;
                pB += reg32t_cof_;
                r <<= 1;
                g <<= 1;
                b <<= 1;
            }
        }
    }

// endregion 工具函数

/* region ================================ 功能函数 ================================ */

    // 单颜色给多灯
    void Ws2812_c::Set(uint16_t start_index, uint16_t num, const RGB_t rgb)
    {
        for(uint16_t i = 0; i < num; i++)
        {
            RGB_2_Buff(start_index + i, rgb.R, rgb.G, rgb.B);
        }
    }
    void Ws2812_c::Set(uint16_t start_index, uint16_t num, const HSV_t hsv)
    {
        RGB_t rgb;
        HSV_2_RGB(&hsv, &rgb);
        for(uint16_t i = 0; i < num; i++)
        {
            RGB_2_Buff(start_index + i, rgb.R, rgb.G, rgb.B);
        }
    }

    // 多颜色给多灯
    void Ws2812_c::Set(uint16_t start_index, uint16_t num, const RGB_t* rgb)
    {
        for(uint16_t i = 0; i < num; i++)
        {
            RGB_2_Buff(start_index + i, rgb[i].R, rgb[i].G, rgb[i].B);
        }
    }

    void Ws2812_c::Set(uint16_t start_index, uint16_t num, const HSV_t* hsv)
    {
        RGB_t rgb;
        for(uint16_t i = 0; i < num; i++)
        {
            HSV_2_RGB(&hsv[i], &rgb);
            RGB_2_Buff(start_index + i, rgb.R, rgb.G, rgb.B);
        }
    }

    void Ws2812_c::Send()
    {
        if(is_spi_not_pwm_)
        {
            memset(spi_buff_, 0, RESET_BYTES * CODE_BYTE); // RESET_BYTES
#if (WS2812_COMMUNICATION_MODE & 0x0F) // SPI
            HAL_SPI_Transmit_DMA(spi_, spi_buff_, (device_num_ * RGB_BIT + RESET_BYTES) * CODE_BYTE);
#endif
        }
        else
        {
            memset(pwm_buff_, 0, (RESET_BYTES * reg32t_cof_)); // RESET_BYTES
#if (WS2812_COMMUNICATION_MODE & 0xF0) // PWM
            pwm_->PWMDMASendArray((uint8_t*)pwm_buff_, (device_num_ * RGB_BIT + RESET_BYTES));
#endif
        }
    }

// endregion 功能函数
}