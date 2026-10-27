/*************************** Dongguan-University of Technology -ACE**************************
 * @file    ws2812.cpp
 * @author  L_Zero
 * @brief   Debug灯语实现
 ******************************************************************************
 * @verbatim
 * * 详细使用看md文件
 * @version & @date & @author
 * * v26.0.0.0 根据灯语规范v26.1.1.1 ---- 2026/02/01 ---- L_Zero
 * * v26.0.0.1 补充CubeMX配置文档 ---- 2026/02/03 ---- L_Zero
 * * v26.0.0.2 EFFECT_TABLE的空指针改为无灯效，部分灯效调暗 ---- 2026/03/03 ---- L_Zero
 * * v26.0.1.0 新增周期触发灯效 ---- 2026/03/05 ---- L_Zero
 * * v26.0.1.1 触发灯效类新增修改配置方法 ---- 2026/03/06 ---- L_Zero
 * * v26.0.1.2 666怎么用到Hal_Delay了 ---- 2026/03/06 ---- L_Zero
 * * v26.0.1.3 修复类成员函数中的静态变量在多对象调用下程序错误问题 ---- 2026/03/24 ---- L_Zero
 * * v26.0.2.0 新增可调节项"闪烁周期" ---- 2026/04/12 ---- L_Zero
 ************************** Dongguan-University of Technology -ACE***************************/
#include "debug_light.hpp"
#include <cstdio>

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
}

#ifndef DEBUG_TIM
#ifdef STM32H723xx
#define DEBUG_TIM &htim1
#define DEBUG_TIM_CHANNEL TIM_CHANNEL_2
#elif STM32F405xx
#define DEBUG_TIM &htim3
#define DEBUG_TIM_CHANNEL TIM_CHANNEL_3
#endif
#endif // !DEBUG_TIM

namespace Light_n
{
    const Effect_t *EFFECT_TABLE[20][2] = {
        {&INIT_EFFECT, &INIT_EFFECT},     // 0 初始化灯效
        {&NORMAL_EFFECT, &NORMAL_EFFECT}, // 1 正常工作灯效
        {&DEBUG_EFFECT, &DEBUG_EFFECT},   // 2 调试灯效
        {&NONE_EFFECT, &NONE_EFFECT},
        {&NONE_EFFECT, &NONE_EFFECT},
        {&CAN1_A_EFFECT, &CANLOST_B_EFFECT},      // 5  CAN1丢失灯效
        {&CAN1_A_EFFECT, &CANERROR_B_EFFECT},     // 6  CAN1错误灯效
        {&CAN2_A_EFFECT, &CANLOST_B_EFFECT},      // 7  CAN2丢失灯效
        {&CAN2_A_EFFECT, &CANERROR_B_EFFECT},     // 8  CAN2错误灯效
        {&CAN3_A_EFFECT, &CANLOST_B_EFFECT},      // 9  CAN3丢失灯效
        {&CAN3_A_EFFECT, &CANERROR_B_EFFECT},     // 10 CAN3错误灯效
        {&USARTLOST_A_EFFECT, &VT13_B_EFFECT},    // 11 图传丢失灯效
        {&USARTLOST_A_EFFECT, &MINIPC_B_EFFECT},  // 12 小电脑离线灯效
        {&USARTLOST_A_EFFECT, &POWCONF_B_EFFECT}, // 13 电源管理丢失灯效
        {&USARTLOST_A_EFFECT, &DR16_B_EFFECT},    // 14 DR16离线灯效
        {&NONE_EFFECT, &NONE_EFFECT},
        {&NONE_EFFECT, &NONE_EFFECT},
        {&RGB_EFFECT, &RGB_EFFECT},
        {&RGB_BLINK_EFFECT, &RGB_BLINK_EFFECT},
        {&RGB_BREATH_EFFECT, &RGB_BREATH_EFFECT},
    };

    Ws2812_n::Ws2812_c* debug_ws2812 = nullptr;
    Light_c* debug_light = nullptr;
    Debug_c* head_instance = nullptr;
    bool init_flag = false;

    void Light_Init()
    {
        debug_ws2812 = new Ws2812_n::Ws2812_c(DEBUG_TIM, DEBUG_TIM_CHANNEL, 2, false);
        debug_light = new Light_c(debug_ws2812);
        init_flag = true;
    }

    /* region ================================ 各种构造 ================================ */
    // 手动触发灯效构造
    Debug_c::Debug_c()
    {
        if(head_instance == nullptr)
        {
            head_instance = this;
            return;
        }
        Debug_c* last_ptr = head_instance;
        while(last_ptr->next_instance_ != nullptr)last_ptr = last_ptr->next_instance_;
        last_ptr->next_instance_ = this;
    }

    // 手动触发灯效构造
    Debug_c::Debug_c(Effect_t effect[2])
    {
        effect_[0] = effect[0];
        effect_[1] = effect[1];
        if(head_instance == nullptr)
        {
            head_instance = this;
            return;
        }
        Debug_c* last_ptr = head_instance;
        while(last_ptr->next_instance_ != nullptr)last_ptr = last_ptr->next_instance_;
        last_ptr->next_instance_ = this;
    }

    // 超阈值触发灯效
    Debug_c::Debug_c(Effect_t effect[2], float *monitor, float min, float max) : monitor_f_(monitor), min_(min), max_(max)
    {
        trigger_   = TriggerType_e::Auto_FloatLimit;
        effect_[0] = effect[0];
        effect_[1] = effect[1];
        if(head_instance == nullptr)
        {
            head_instance = this;
            return;
        }
        Debug_c* last_ptr = head_instance;
        while(last_ptr->next_instance_ != nullptr)last_ptr = last_ptr->next_instance_;
        last_ptr->next_instance_ = this;
    }

    // 布尔值触发灯效
    Debug_c::Debug_c(Effect_t effect[2], bool *monitor) : monitor_b_(monitor)
    {
        trigger_   = TriggerType_e::Auto_Bool;
        effect_[0] = effect[0];
        effect_[1] = effect[1];
        if(head_instance == nullptr)
        {
            head_instance = this;
            return;
        }
        Debug_c* last_ptr = head_instance;
        while(last_ptr->next_instance_ != nullptr)last_ptr = last_ptr->next_instance_;
        last_ptr->next_instance_ = this;
    }

    // 周期触发灯效
    Debug_c::Debug_c(Effect_t effect[2], float *monitor, float d_time) : monitor_t_(monitor), d_time_(d_time)
    {
        trigger_   = TriggerType_e::Auto_Cycle;
        effect_[0] = effect[0];
        effect_[1] = effect[1];
        if(head_instance == nullptr)
        {
            head_instance = this;
            return;
        }
        Debug_c* last_ptr = head_instance;
        while(last_ptr->next_instance_ != nullptr)last_ptr = last_ptr->next_instance_;
        last_ptr->next_instance_ = this;
    }
    // endregion 各种构造

    /* region ================================ 类函数 ================================ */
    void Debug_c::ManualTrigger()
    {
        if(!is_enable_) return;
        debug_light->SetEffect(effect_, 2);
    }
    void Debug_c::ManualTrigger(Effect_t effect[2])
    {
        if(!is_enable_) return;
        debug_light->SetEffect(effect, 2);
    }

    void Debug_c::AutoCheck()
    {
        if(!is_enable_) return;
        if(trigger_ == TriggerType_e::Auto_FloatLimit)
        {
            if(*monitor_f_ > max_ || *monitor_f_ < min_) // 浮点数超阈值触发灯效
            {
                ManualTrigger();
            }
        }
        else if (trigger_ == TriggerType_e::Auto_Bool)
        {
            if (condition_ == *monitor_b_) // 布尔值触发灯效
            {
                ManualTrigger();
            }
        }
        else if(trigger_ == TriggerType_e::Auto_Cycle)
        {
            *monitor_t_ -= BSP_n::DWT_c::Get_DwtInstance()->GetDeltaT(&cycle_timer_cnt_);
            if(*monitor_t_ < 0) // 周期时间触发灯效
            {
                *monitor_t_ = d_time_;
                ManualTrigger();
            }
        }
    }
    // endregion 类函数

    void ShowStatic(StaticEffect_e type)
    {
        if(!init_flag) // 确保初始化
        {
            Light_Init();
        }

        Effect_t effect_group[2] = {
            *(EFFECT_TABLE[(uint8_t)type][0]),
            *(EFFECT_TABLE[(uint8_t)type][1]),
        };
        debug_light->SetEffect(effect_group, 2);
    }

    void ShowBlink(StaticEffect_e type, uint8_t blink_times)
    {
        if(!init_flag) // 确保初始化
        {
            Light_Init();
        }

        Effect_t a_effect = *(EFFECT_TABLE[(uint8_t)type][0]);
        Effect_t blink_effect = *(EFFECT_TABLE[(uint8_t)type][1]);
        blink_effect.mode |= Blink;
        blink_effect.SetBlink(blink_times, blink_effect.blink_cycle);
        const uint16_t blink_cycle_ms = (uint16_t)(blink_effect.blink_cycle * 1000.0f + 0.5f);
        std::snprintf(blink_effect.name, sizeof(blink_effect.name), "sb%u_%u_%u",
                      (uint8_t)type, blink_times, blink_cycle_ms);
        a_effect.duration = blink_effect.duration; // 保证同一组灯效两颗灯时长一致

        Effect_t effect_group[2] = {a_effect, blink_effect};
        debug_light->SetEffect(effect_group, 2);
    }

    void UpdateLoop()
    {
        if(!init_flag)
        {
            Light_Init();
        }
        Debug_c* temp_ptr = head_instance;
        while(temp_ptr != nullptr)
        {
            temp_ptr->AutoCheck();
            temp_ptr = temp_ptr->next_instance_;
        }
        debug_light->Update();
    }
}

void Debug_Task(void const *argument)
{
    while(1)
    {
        Light_n::ShowStatic(Light_n::StaticEffect_e::Normal);
        Light_n::UpdateLoop();
        vTaskDelay(20);
    }
}

