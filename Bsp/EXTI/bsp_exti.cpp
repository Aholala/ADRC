/*************************** Dongguan-University of Technology -ACE**************************
 * @file    bsp_exti.cpp
 * @author  L_Zero
 * @version V1.0
 * @date    2025/11/21
 * @example
 *       BSP_n::Exti_c::Init_Config_t config =
 *               {
 *                       .gpiox = GPIOA,
 *                       .gpio_pin = GPIO_PIN_3,
 *                       .trigger_level = GPIO_PIN_SET,
 *                       .exti_callback_ptr = &Adjust_MicroSwitchCallback // 你自己的回调函数
 *               };
 *       micro_switch_ = new BSP_n::Exti_c(config);
 *
 ************************** Dongguan-University of Technology -ACE***************************/
#include "bsp_exti.hpp"

namespace BSP_n
{
    Exti_c *header = nullptr; // 指向类的指针，做轮询链表

    Exti_c::Exti_c(BSP_n::Exti_c::Init_Config_t config)
    {
        gpiox_ = config.gpiox;
        gpio_pin_ = config.gpio_pin;
        trigger_level_ = config.trigger_level;
        exti_callback_ptr_ = config.exti_callback_ptr;

        if(header == nullptr)
        {
            header = this;
        }
        else
        {
            Exti_c *last_ptr = header;
            while (last_ptr->next_instance_ != nullptr)
            {
                last_ptr = last_ptr->next_instance_;
            }
            last_ptr->next_instance_ = this; // 简易链表，主要用于callback轮询
        }
    }

    Exti_c::Exti_c(GPIO_TypeDef* gpiox, uint16_t gpio_pin, GPIO_PinState trigger_level, BSP_n::Exti_Callback_p exti_callback_ptr):
                    gpiox_(gpiox), gpio_pin_(gpio_pin), trigger_level_(trigger_level), exti_callback_ptr_(exti_callback_ptr)
    {
        if(header == nullptr)
        {
            header = this;
        }
        else
        {
            Exti_c *last_ptr = header;
            while (last_ptr->next_instance_ != nullptr)
            {
                last_ptr = last_ptr->next_instance_;
            }
            last_ptr->next_instance_ = this; // 简易链表，主要用于callback轮询
        }
    }

    void Exti_c::Callback(uint16_t GPIO_Pin)
    {
        Exti_c* temp_ptr = header;
        while(temp_ptr != nullptr) // 链表轮询
        {
            if(temp_ptr->gpio_pin_ == GPIO_Pin && HAL_GPIO_ReadPin(temp_ptr->gpiox_, temp_ptr->gpio_pin_) == temp_ptr->trigger_level_)
            {
                if(temp_ptr->exti_callback_ptr_ != nullptr)
                {
                    temp_ptr->exti_callback_ptr_(); // 进自定义回调处理数据
                }
            }
            temp_ptr = temp_ptr->next_instance_;
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BSP_n::Exti_c::Callback(GPIO_Pin);
}

