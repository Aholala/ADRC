#ifndef __LIGHT_HPP
#define __LIGHT_HPP

#include "ws2812.hpp"
#include "bsp_dwt.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
#include "string.h"
#ifdef __cplusplus
};
#endif


namespace Light_n
{
    typedef enum class Priority_e
    {
        Useless = 0x00u,
        Idle = 0x01u,
        BelowNormal = 0x02u,
        Normal = 0x03u,
        AboveNormal = 0x04u,
        High = 0x05u,
        MostUrgent = 0x06u,
    }Priority_e;

    typedef enum
    {
        Breath = 0b00000001,         // 呼吸(亮度循环)
        ColorCycle = 0b00000010,     // 颜色循环
        SaturationCycle = 0b00000100,// 饱和度循环
        Blink = 0b00001000,          // 闪烁
    }EffectMode_e;

    typedef struct Effect_t
    {
        char name[16] = "NULL";
        Priority_e priority = Priority_e::Idle; // 优先级，同一个灯上同优先级取后注册的
        float duration = 1;                 // 此灯效持续时间
        uint8_t mode = 0;                   // 效果模式
        float breath_speed = 0;             // 呼吸速度(百分比/更新周期)
        float breath_min = 0;               // 亮度最小阈值(百分制)
        float breath_max = 100;             // 亮度最大阈值(百分制)
        float color_speed = 0;              // 色环循环速度(°/更新周期)
        float saturation_speed = 0;         // 饱和度循环速度(百分比/更新周期)
        uint8_t blink_times = 0;            // 闪烁次数
        float blink_cycle = 0.2f;           // 单次闪烁周期
        float timer = 0;                    // 闪烁逻辑计时器

        Ws2812_n::HSV_t color = {0, 0, 0};  // HSV颜色
        void SetRGB(Ws2812_n::RGB_t *rgb) {Ws2812_n::RGB_2_HSV(rgb, &color);}

        void SetBlink(uint8_t blink_times, float blink_cycle = 0.2f)
        {
            uint8_t blink_times_limited = blink_times;
#ifdef DEFAULT_LIGHT_BLINK_MAX
            if(blink_times_limited > DEFAULT_LIGHT_BLINK_MAX) blink_times_limited = DEFAULT_LIGHT_BLINK_MAX;
#else
            if(blink_times_limited > 8) blink_times_limited = 8;
#endif
            this->blink_times = blink_times_limited;
            this->blink_cycle = blink_cycle;
            duration = blink_times_limited * blink_cycle + 1.f;
        }

        // 重载赋值运算符
        Effect_t& operator=(const Effect_t& other)
        {
            if (this != &other) // 防止自赋值
            {
                // 赋值所有成员，除了 timer
                strcpy(name, other.name);
                priority = other.priority;
                duration = other.duration;
                mode = other.mode;
                breath_speed = other.breath_speed;
                breath_min = other.breath_min;
                breath_max = other.breath_max;
                color_speed = other.color_speed;
                saturation_speed = other.saturation_speed;
                blink_times = other.blink_times;
                blink_cycle = other.blink_cycle;
                Ws2812_n::HSV_t temp = other.color;
                if(other.color.hue < 0)
                    temp.hue = color.hue;
                if(other.color.saturation < 0)
                    temp.saturation = color.saturation;
                if(other.color.value < 0)
                    temp.value = color.value;
                color = temp;
            }
            return *this;
        }

        // 重载比较运算符
        bool operator==(const Effect_t& other)
        {
            if(strcmp(other.name, this->name))
            { // 字符串不等返回false
                return false;
            }
            return true;
        }
    } Effect_t;

    class Light_c
    {
    public:
#ifndef DEFAULT_LIGHT_QUEUE_SIZE
        Light_c(Ws2812_n::Ws2812_c *ws2812, uint8_t queue_size = 3);
#else
        Light_c(Ws2812_n::Ws2812_c *ws2812, uint8_t queue_size = DEFAULT_LIGHT_QUEUE_SIZE);
#endif
        
        ~Light_c();
        bool SetEffect(uint16_t light_index, Effect_t set);
        bool SetEffect(const Effect_t* set_list, uint16_t light_num);
        bool ChangeEffect(uint16_t light_index, Effect_t set);
        void Update();

    private:
        Effect_t* QueueSlot(uint16_t light_index, uint8_t queue_index);
        bool PushEffect(uint16_t light_index, const Effect_t& set);
        bool PopEffect(uint16_t light_index, Effect_t* out);
        void ClearQueue(uint16_t light_index);
        Priority_e RefPriority(uint16_t light_index);
        bool HasSameEffect(uint16_t light_index, const Effect_t& set);
        void ClearAllQueue();
        Priority_e MaxRefPriority();
        bool HasSameEffectGlobal(const Effect_t* set_list, uint16_t light_num);

        Ws2812_n::Ws2812_c* ws2812_ = nullptr;
        BSP_n::DWT_c* timer_ = nullptr;
        Effect_t* light_effect_list_ = nullptr;
        Effect_t* effect_queue_ = nullptr;
        uint8_t* queue_head_ = nullptr;
        uint8_t* queue_tail_ = nullptr;
        uint8_t* queue_count_ = nullptr;
        uint8_t queue_size_ = 3;
    };
}

#endif //! __LIGHT_HPP
