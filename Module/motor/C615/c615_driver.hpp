#ifndef __C615_DRIVER_HPP
#define __C615_DRIVER_HPP

#include "bsp_pwm.hpp"

#define C615_PULSE_MIN 400
#define C615_PULSE_MAX 2200
#define C615_FREQ_MAX  500

namespace Motor_n
{
    typedef enum class StartMode_e
    {
        Normal = 0x00u,
        Setting = 0x01u // BB时0速度校准最大最小脉宽，BBB时0速度设置反转
    }StartMode_e;

    class C615_c
    {
    public:
        struct Init_t
        {
            TIM_HandleTypeDef *htim;
            uint32_t channel;
            float pwm_freq;            // 30~500Hz，跟你CubeMX配置需一致
            uint16_t min_pulse = 1090; // 脉宽设置，单位us
            uint16_t max_pulse = 1920; // 务必跟DJI助手里设置的一样
            StartMode_e start_mode = StartMode_e::Normal;
        };

        C615_c(Init_t config);
        void SetSpeed(float persent);
        void Disable();

    private:
        BSP_n::PWM_c* pwm_;
        float pwm_cycle_;
        uint32_t min_ccr_;
        uint32_t max_ccr_;
    };
}

#endif //! __C615_DRIVER_HPP
