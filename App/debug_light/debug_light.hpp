#ifndef __DEBUG_LIGHT_HPP
#define __DEBUG_LIGHT_HPP

#include "light.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

#include "debug_light.h"

#ifdef __cplusplus
};
#endif

namespace Light_n
{
    typedef enum class StaticEffect_e /* 固定灯效 */
    {
        /* 正常灯效 */
        Init = 0,
        Normal = 1,
        Debug = 2,
        // 3、4 保留

        /* CAN异常灯效 */
        Can1_Lost  = 5,
        Can1_Error = 6,
        Can2_Lost  = 7,
        Can2_Error = 8,
        Can3_Lost  = 9,
        Can3_Error = 10,
        // 8、9、10 保留

        /* 通讯异常灯效 */
        VT13_Lost = 11,
        MiniPC_Lost = 12,
        PowCF_Lost = 13,
        DR16_Lost = 14,
        // 15、16 保留

        /* 酷炫灯效 */
        RGB = 17,
        RGB_Blink = 18,
        RGB_Breath = 19,
    } StaticEffect_e;

    typedef enum class TriggerType_e
    {
        Manual = 0,          // 手动触发
        Auto_FloatLimit = 1, // 超阈值触发
        Auto_Bool = 2,       // 布尔值为0触发
        Auto_Cycle = 3,      // 周期性触发
    } TriggerType_e;

    class Debug_c
    {
    public:
        Debug_c* next_instance_ = nullptr;
        Effect_t effect_[2];

        Debug_c();
        Debug_c(Effect_t effect[2]);
        Debug_c(Effect_t effect[2], float *monitor, float min, float max);
        Debug_c(Effect_t effect[2], bool *monitor);
        Debug_c(Effect_t effect[2], float *monitor, float d_time);

        void Set_B_LightBlink(uint8_t blink_times, float blink_cycle = 0.2f) 
        {
            effect_[1].SetBlink(blink_times, blink_cycle);
            effect_[0].duration = effect_[1].duration;
        }
        void ManualTrigger();
        void ManualTrigger(Effect_t effect[2]);
        void Change_FloatLimit(float min, float max) {min_ = min; max_ = max;}
        void Change_BoolCondition(bool condition) {condition_ = condition;}
        void Change_CycleTime(float d_time) {d_time_ = d_time;}
        void Enable() {is_enable_ = true;}
        void Disable() {is_enable_ = false;}
        friend void UpdateLoop();

    private:
        bool is_enable_ = false;
        TriggerType_e trigger_ = TriggerType_e::Manual;
        float* monitor_f_ = nullptr;
        bool* monitor_b_ = nullptr;
        float* monitor_t_ = nullptr;
        float min_ = 0;
        float max_ = 0;
        bool condition_ = false;
        float d_time_ = 0;
        uint32_t cycle_timer_cnt_ = 0;

        void AutoCheck();
    };
    void ShowStatic(StaticEffect_e type);
    void ShowBlink(StaticEffect_e type, uint8_t times);
    void UpdateLoop();

    const Ws2812_n::HSV_t RED    = {0, 1, 1};
    const Ws2812_n::HSV_t GREEN  = {120, 1, 1};
    const Ws2812_n::HSV_t BLUE   = {240, 1, 1};
    const Ws2812_n::HSV_t YELLOW = {60, 1, 1};
    const Ws2812_n::HSV_t PURPLE = {300, 1, 1};
    const Ws2812_n::HSV_t CYAN   = {180, 1, 1};
    const Ws2812_n::HSV_t ORANGE = {39, 1, 1};
    const Ws2812_n::HSV_t WHITE  = {0, 0, 1};
    const Ws2812_n::HSV_t BLACK  = {0, 0, 0};
    const Effect_t NONE_EFFECT = {
            .name = "none",
            .priority = Priority_e::Idle,
            .color = BLACK,
    };
    const Effect_t CLEAR_EFFECT = {
            .name = "clear",
            .priority = Priority_e::MostUrgent,
            .color = BLACK,
    };
    const Effect_t INIT_EFFECT = {
            .name = "init",
            .priority = Priority_e::BelowNormal,
            .mode = Blink,
            .blink_times = 1,
            .color = GREEN
    };
    const Effect_t NORMAL_EFFECT = {
            .name = "normal",
            .priority = Priority_e::Idle,
            .mode = Breath,
            .breath_speed = 0.006f,
            .breath_max = 40,
            .color = {120, 1, -1}
    };
    const Effect_t DEBUG_EFFECT = {
            .name = "debug",
            .priority = Priority_e::BelowNormal,
            .mode = Breath,
            .breath_speed = 0.10f,
            .breath_min = 10,
            .breath_max = 60,
            .color = {120, 1, -1}
    };
    const Effect_t CAN1_A_EFFECT = {
            .name = "can1_A",
            .priority = Priority_e::Normal,
            .mode = 0,
            .color = BLUE
    };
    const Effect_t CAN2_A_EFFECT = {
            .name = "can2_A",
            .priority = Priority_e::Normal,
            .mode = 0,
            .color = PURPLE
    };
    const Effect_t CAN3_A_EFFECT = {
            .name = "can3_A",
            .priority = Priority_e::Normal,
            .mode = 0,
            .color = WHITE
    };
    const Effect_t CANLOST_B_EFFECT = {
            .name = "canlost_B",
            .priority = Priority_e::Normal,
            .mode = Blink,
            .blink_times = 0,
            .color = RED
    };
    const Effect_t CANERROR_B_EFFECT = {
            .name = "canerror_B",
            .priority = Priority_e::Normal,
            .mode = Blink,
            .blink_times = 0,
            .color = YELLOW
    };
    const Effect_t USARTLOST_A_EFFECT = {
            .name = "usart_A",
            .priority = Priority_e::AboveNormal,
            .mode = 0,
            .color = RED
    };
    const Effect_t VT13_B_EFFECT = {
            .name = "vt13_B",
            .priority = Priority_e::AboveNormal,
            .mode = Blink,
            .blink_times = 1,
            .color = RED
    };
    const Effect_t MINIPC_B_EFFECT = {
            .name = "miniPC_B",
            .priority = Priority_e::AboveNormal,
            .mode = Blink,
            .blink_times = 1,
            .color = YELLOW
    };
    const Effect_t POWCONF_B_EFFECT = {
            .name = "powCF_B",
            .priority = Priority_e::AboveNormal,
            .mode = Blink,
            .blink_times = 1,
            .color = BLUE
    };
    const Effect_t DR16_B_EFFECT = {
            .name = "dr16_B",
            .priority = Priority_e::AboveNormal,
            .mode = Blink,
            .blink_times = 1,
            .color = WHITE
    };
    const Effect_t RGB_EFFECT = {
            .name = "rgb",
            .priority = Priority_e::BelowNormal,
            .duration = 2.5,
            .mode = ColorCycle,
            .color_speed = 3,
            .color = {-1, 1, 0.4f}
    };
    const Effect_t RGB_BLINK_EFFECT = {
            .name = "rgb_blink",
            .priority = Priority_e::BelowNormal,
            .duration = 2.5,
            .mode = ColorCycle | Blink,
            .color_speed = 3,
            .blink_times = 1,
            .color = {-1, 1, 0.4f}
    };
    const Effect_t RGB_BREATH_EFFECT = {
            .name = "rgb_breath",
            .priority = Priority_e::BelowNormal,
            .duration = 2.5,
            .mode = ColorCycle | Breath,
            .breath_speed = 0.06f,
            .breath_min = 10,
            .breath_max = 60,
            .color_speed = 3,
            .color = {-1, 1, 0.4f}
    };
}

#endif //! __DEBUG_LIGHT_HPP