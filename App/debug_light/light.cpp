/*************************** Dongguan-University of Technology -ACE**************************
 * @file    light.cpp
 * @author  coconutjuice & L_Zero
 * @brief   多灯珠灯效
 ******************************************************************************
 * @attention
 * * 一个输出口用同一个实例
 * @version & @date & @author
 * * v26.0.0.0 将灯效解耦到这里了 ---- 2026/01/31 ---- L_Zero
 * * v26.1.0.0 灯效结构体新增闪烁参数修改函数，新增灯效队列，允许按顺序播放多灯效 ---- 2026/04/12 ---- L_Zero
 * * v26.1.1.0 修复灯效队列不同步问题，修复灯效blink配置不被"=操作符"赋值，新增闪烁次数限制 ---- 2026/04/14 ---- L_Zero
 ************************** Dongguan-University of Technology -ACE***************************/

#include "light.hpp"
#include <cmath>

namespace Light_n
{
    // 找找队里要啥灯效
    Effect_t* Light_c::QueueSlot(uint16_t light_index, uint8_t queue_index)
    {
        return &effect_queue_[light_index * queue_size_ + queue_index];
    }

    // 增添
    bool Light_c::PushEffect(uint16_t light_index, const Effect_t& set)
    {
        if(queue_count_[light_index] >= queue_size_)
        {
            return false;
        }

        Effect_t* slot = QueueSlot(light_index, queue_tail_[light_index]);
        *slot = set;
        slot->timer = 0;

        queue_tail_[light_index] = (queue_tail_[light_index] + 1) % queue_size_;
        queue_count_[light_index]++;
        return true;
    }

    // 删除
    bool Light_c::PopEffect(uint16_t light_index, Effect_t* out)
    {
        if(queue_count_[light_index] == 0 || out == nullptr)
        {
            return false;
        }

        Effect_t* slot = QueueSlot(light_index, queue_head_[light_index]);
        *out = *slot;
        out->timer = 0;

        queue_head_[light_index] = (queue_head_[light_index] + 1) % queue_size_;
        queue_count_[light_index]--;
        return true;
    }

    // 清空
    void Light_c::ClearQueue(uint16_t light_index)
    {
        queue_head_[light_index] = 0;
        queue_tail_[light_index] = 0;
        queue_count_[light_index] = 0;
    }

    // 比较优先级
    Priority_e Light_c::RefPriority(uint16_t light_index)
    {
        if(light_effect_list_[light_index].priority != Priority_e::Useless)
        {
            return light_effect_list_[light_index].priority;
        }
        if(queue_count_[light_index] > 0)
        {
            return QueueSlot(light_index, queue_head_[light_index])->priority;
        }
        return Priority_e::Useless;
    }

    // 队列去重
    bool Light_c::HasSameEffect(uint16_t light_index, const Effect_t& set)
    {
        // 当前正在播放的灯效去重
        if(light_effect_list_[light_index].priority != Priority_e::Useless &&
           strcmp(light_effect_list_[light_index].name, set.name) == 0)
        {
            return true;
        }

        // 等待队列中的灯效去重
        uint8_t index = queue_head_[light_index];
        for(uint8_t i = 0; i < queue_count_[light_index]; i++)
        {
            Effect_t* queued = QueueSlot(light_index, index);
            if(strcmp(queued->name, set.name) == 0)
            {
                return true;
            }
            index = (index + 1) % queue_size_;
        }
        return false;
    }

    void Light_c::ClearAllQueue()
    {
        for(uint16_t i = 0; i < ws2812_->device_num(); i++)
        {
            ClearQueue(i);
        }
    }

    Priority_e Light_c::MaxRefPriority()
    {
        Priority_e max_priority = Priority_e::Useless;
        for(uint16_t i = 0; i < ws2812_->device_num(); i++)
        {
            Priority_e p = RefPriority(i);
            if((uint8_t)p > (uint8_t)max_priority)
            {
                max_priority = p;
            }
        }
        return max_priority;
    }

    bool Light_c::HasSameEffectGlobal(const Effect_t* set_list, uint16_t light_num)
    {
        // 正在播放中的整组去重：所有灯珠 name 都一致才算重复
        bool current_same = true;
        for(uint16_t i = 0; i < light_num; i++)
        {
            if(light_effect_list_[i].priority == Priority_e::Useless ||
               strcmp(light_effect_list_[i].name, set_list[i].name) != 0)
            {
                current_same = false;
                break;
            }
        }
        if(current_same)
        {
            return true;
        }

        // 等待队列中的整组去重：同一队列深度上所有灯珠 name 都一致才算重复
        // 组提交会保持各灯队列深度一致，这里按灯0队列深度遍历
        for(uint8_t depth = 0; depth < queue_count_[0]; depth++)
        {
            bool all_match = true;
            for(uint16_t i = 0; i < light_num; i++)
            {
                if(queue_count_[i] <= depth)
                {
                    all_match = false;
                    break;
                }

                uint8_t index = (queue_head_[i] + depth) % queue_size_;
                Effect_t* queued = QueueSlot(i, index);
                if(strcmp(queued->name, set_list[i].name) != 0)
                {
                    all_match = false;
                    break;
                }
            }

            if(all_match)
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief 构造，一个输出口请用同一个实例(不知道怎么限制这个)
     */
    Light_c::Light_c(Ws2812_n::Ws2812_c *ws2812, uint8_t queue_size) : ws2812_(ws2812)
    {
        if(queue_size == 0)
        {
            queue_size = 3;
        }
        queue_size_ = queue_size;

        const uint16_t light_num = ws2812->device_num();
        light_effect_list_ = new Effect_t[light_num];
        effect_queue_ = new Effect_t[light_num * queue_size_];
        queue_head_ = new uint8_t[light_num];
        queue_tail_ = new uint8_t[light_num];
        queue_count_ = new uint8_t[light_num];
        memset(queue_head_, 0, light_num);
        memset(queue_tail_, 0, light_num);
        memset(queue_count_, 0, light_num);

        for(uint16_t i = 0; i < light_num; i++)
        {
            light_effect_list_[i].priority = Priority_e::Useless;
            light_effect_list_[i].duration = 0;
            light_effect_list_[i].timer = 0;
            light_effect_list_[i].color = {0, 0, 0};
        }
        timer_ = BSP_n::DWT_c::Get_DwtInstance();
    }

    Light_c::~Light_c()
    {
        delete[] light_effect_list_;
        delete[] effect_queue_;
        delete[] queue_head_;
        delete[] queue_tail_;
        delete[] queue_count_;
    }

    /**
     * @brief 赋予灯效
     *        赋予后，需要执行Update()才会正确部署设置
     * @return 是否设置成功
     */
    bool Light_c::SetEffect(uint16_t light_index, Light_n::Effect_t set)
    {
        if(light_index >= ws2812_->device_num()) // 防呆
        {
            return false;
        }

        const Priority_e ref_priority = RefPriority(light_index);
        Effect_t* current = &light_effect_list_[light_index];
        set.timer = 0;

        // 同一种灯效不重复入队（包括正在播放和队列中）
        if(HasSameEffect(light_index, set))
        {
            return false;
        }

        // 高优先级抢占：立即生效并清空同灯珠等待队列
        if(set.priority > ref_priority)
        {
            *current = set;
            current->timer = 0;
            ClearQueue(light_index);
            return true;
        }

        // 同优先级按调用顺序排队
        if(set.priority == ref_priority || ref_priority == Priority_e::Useless)
        {
            return PushEffect(light_index, set);
        }

        // 低优先级不接受
        return false;
    }

    bool Light_c::SetEffect(const Effect_t* set_list, uint16_t light_num)
    {
        const uint16_t device_num = ws2812_->device_num();
        if(set_list == nullptr || light_num != device_num)
        {
            return false;
        }

        if(HasSameEffectGlobal(set_list, light_num))
        {
            return false;
        }

        Priority_e in_priority = Priority_e::Useless;
        for(uint16_t i = 0; i < light_num; i++)
        {
            if((uint8_t)set_list[i].priority > (uint8_t)in_priority)
            {
                in_priority = set_list[i].priority;
            }
        }

        const Priority_e ref_priority = MaxRefPriority();

        // 高优先级抢占：整组立即生效并清空所有等待队列
        if((uint8_t)in_priority > (uint8_t)ref_priority)
        {
            for(uint16_t i = 0; i < light_num; i++)
            {
                light_effect_list_[i] = set_list[i];
                light_effect_list_[i].timer = 0;
            }
            ClearAllQueue();
            return true;
        }

        // 同优先级或系统空闲：整组原子入队（要么全入要么全不入）
        if((uint8_t)in_priority == (uint8_t)ref_priority || ref_priority == Priority_e::Useless)
        {
            for(uint16_t i = 0; i < light_num; i++)
            {
                if(queue_count_[i] >= queue_size_)
                {
                    return false;
                }
            }

            for(uint16_t i = 0; i < light_num; i++)
            {
                Effect_t temp = set_list[i];
                temp.timer = 0;
                PushEffect(i, temp);
            }
            return true;
        }

        // 低优先级不接受
        return false;
    }

    void Light_c::Update()
    {
        static uint32_t d_time_cnt = 0;
        float d_time = timer_->GetDeltaT(&d_time_cnt);

        // 同步当前播放组时长：避免同组灯效因duration不同而提前出队
        float synced_duration = 0.0f;
        bool has_active_effect = false;
        for(int i = 0; i < ws2812_->device_num(); i++)
        {
            Effect_t* effect = &light_effect_list_[i];
            if(effect->priority != Priority_e::Useless)
            {
                if(!has_active_effect || effect->duration > synced_duration)
                {
                    synced_duration = effect->duration;
                    has_active_effect = true;
                }
            }
        }
        if(has_active_effect)
        {
            for(int i = 0; i < ws2812_->device_num(); i++)
            {
                Effect_t* effect = &light_effect_list_[i];
                if(effect->priority != Priority_e::Useless)
                {
                    effect->duration = synced_duration;
                }
            }
        }

        for (int i = 0; i < ws2812_->device_num(); i++)
        {
            Effect_t* effect = &light_effect_list_[i];  // 方便看
            Ws2812_n::HSV_t temp_color = {0, 0, 0}; // 临时取值，默认灭灯

            // 当前无正在播放灯效时，尝试从队列取下一个
            if(effect->priority == Priority_e::Useless)
            {
                if(!PopEffect(i, effect))
                {
                    ws2812_->Set(i, 1, &temp_color);
                    continue;
                }
            }

            // 呼吸执行
            if(effect->mode & (uint8_t)EffectMode_e::Breath)
            {
                effect->color.value += effect->breath_speed;
                if(effect->color.value * 100 > effect->breath_max)
                {
                    effect->color.value = effect->breath_max * 0.01;
                    effect->breath_speed = -effect->breath_speed;
                }
                else if(effect->color.value * 100 < effect->breath_min)
                {
                    effect->color.value = effect->breath_min * 0.01;
                    effect->breath_speed = -effect->breath_speed;
                }
            }

            // 颜色变化执行
            if(effect->mode & (uint8_t)EffectMode_e::ColorCycle)
            {
                effect->color.hue += effect->color_speed;
                if(effect->color.hue > 360)
                {
                    effect->color.hue = 0;
                }
            }

            // 饱和度循环执行
            if(effect->mode & (uint8_t)EffectMode_e::SaturationCycle)
            {
                effect->color.saturation += effect->saturation_speed;
                if(effect->color.saturation > 1)
                {
                    effect->color.saturation = 1;
                    effect->saturation_speed = -effect->saturation_speed;
                }
                else if(effect->color.saturation < 0)
                {
                    effect->color.saturation = 0;
                    effect->saturation_speed = -effect->saturation_speed;
                }
            }

            temp_color.hue = effect->color.hue;
            temp_color.saturation = effect->color.saturation;
            temp_color.value = effect->color.value;

            // 闪烁执行
            if(effect->mode & (uint8_t)EffectMode_e::Blink && effect->blink_times && effect->blink_cycle > 0.0f)
            {
                effect->timer += d_time;

                temp_color.value = 0; // 闪烁时默认灭
                const float total_blink_time = effect->blink_times * effect->blink_cycle;
                if(effect->timer < total_blink_time)
                {
                    const float phase = fmodf(effect->timer, effect->blink_cycle);
                    if(phase < effect->blink_cycle * 0.5f)
                    {
                        temp_color.value = effect->color.value; // 亮
                    }
                }
            }

            // 持续时间判断
            effect->duration -= d_time;
            if(effect->duration < 0)
            {
                effect->priority = Priority_e::Useless;
                effect->timer = 0;
                effect->duration = 0;
            }
            ws2812_->Set(i, 1, &temp_color);
        }
        ws2812_->Send();
    }
}
