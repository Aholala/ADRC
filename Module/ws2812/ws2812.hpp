#ifndef __WS2812_HPP
#define __WS2812_HPP

#include "ecf_configList.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#ifdef __cplusplus
}
#endif

#if (WS2812_COMMUNICATION_MODE & 0xF0) // PWM
#include "bsp_pwm.hpp"
#endif

#if (WS2812_COMMUNICATION_MODE & 0x0F) // SPI
#ifdef __cplusplus
extern "C" {
#endif

#include "spi.h"

#ifdef __cplusplus
}
#endif
#endif

namespace Ws2812_n
{
    typedef struct
    {
        uint8_t R;  // 红 0~255
        uint8_t G;  // 绿 0~255
        uint8_t B;  // 蓝 0~255
    } RGB_t;

    typedef struct
    {
        float hue;        // 色相 0~360
        float saturation; // 饱和 0~1
        float value;      // 亮度 0~1
    } HSV_t;

    void RGB_2_HSV(const RGB_t* rgb, HSV_t* hsv);
    void HSV_2_RGB(const HSV_t* hsv, RGB_t* rgb);
    HSV_t RGB_2_HSV(const RGB_t* rgb);
    RGB_t HSV_2_RGB(const HSV_t* hsv);

    class Ws2812_c
    {
/* PWM instance construction */
#if (WS2812_COMMUNICATION_MODE & 0xF0)
    public:
        struct PWM_Init_t
        {
            TIM_HandleTypeDef *htim;
            uint32_t channel;
            uint16_t device_num;
            bool is_32reg = false;
        };

        Ws2812_c(PWM_Init_t config);
        Ws2812_c(TIM_HandleTypeDef *htim, uint32_t channel, uint16_t device_num, bool is_32reg);
    private:
        BSP_n::PWM_c* pwm_ = nullptr;
#endif // PWM

/* SPI instance construction */
#if (WS2812_COMMUNICATION_MODE & 0x0F) // SPI
    public:
        struct SPI_Init_t
        {
            SPI_HandleTypeDef *hspi;
            uint16_t device_num;
        };

        Ws2812_c(SPI_Init_t config);
        Ws2812_c(SPI_HandleTypeDef *hspi, uint16_t device_num);

    private:
        SPI_HandleTypeDef* spi_ = nullptr;
#endif // SPI

    public:
        uint16_t device_num(){ return device_num_;}
        void Set(uint16_t start_index, uint16_t num, const RGB_t rgb);
        void Set(uint16_t start_index, uint16_t num, const HSV_t hsv);
        void Set(uint16_t start_index, uint16_t num, const RGB_t* rgb);
        void Set(uint16_t start_index, uint16_t num, const HSV_t* hsv);
        void Send();

        uint8_t *spi_buff_  = nullptr;
        uint8_t *pwm_buff_  = nullptr;

    private:
        bool is_spi_not_pwm_ = false;
        uint8_t reg32t_cof_ = 2;
        uint16_t device_num_ = 0;
//        uint8_t *spi_buff_  = nullptr;
//        uint8_t *pwm_buff_  = nullptr;
        uint8_t ccr0_ = 0;
        uint8_t ccr1_ = 0;

        void RGB_2_Buff(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
    };
}


#endif //! __WS2812_HPP
