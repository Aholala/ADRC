#ifndef __BSP_EXTI_HPP
#define __BSP_EXTI_HPP

extern "C"
{
#include "gpio.h"
#include <stdint.h>

#endif
#ifdef __cplusplus
}

namespace BSP_n
{
    using Exti_Callback_p = void (*)(void); // 自定义回调指针

    class Exti_c
    {
    public:
        typedef struct
        {
            GPIO_TypeDef* gpiox;
            uint16_t gpio_pin;
            GPIO_PinState trigger_level; // 触发电平
            Exti_Callback_p exti_callback_ptr;
        } Init_Config_t;

        Exti_c(Init_Config_t config);
        Exti_c(GPIO_TypeDef* gpiox, uint16_t gpio_pin, GPIO_PinState trigger_level, Exti_Callback_p exti_callback_ptr);
        Exti_c* next_instance_ = nullptr;

        static void Callback(uint16_t GPIO_Pin);
    private:
        GPIO_TypeDef* gpiox_;
        uint16_t gpio_pin_;
        GPIO_PinState trigger_level_;
        Exti_Callback_p exti_callback_ptr_;
    };
}

#endif //! __BSP_EXTI_HPP
