/************************** Dongguan-University of Technology -ACE**************************
 * @file  c615_driver.cpp
 * @brief Snail电机+C615控速电调
 * @author L_Zero
 * ==============================================================================
 * @verbatim
 * * 配合DJI助手使用https://www.robomaster.com/zh-CN/products/components/detail/2139
 * @attention
 * * PWM配置务必跟CubeMX一致(频率范围30~500Hz)
 * * 主控板输出脉宽不一定准确(Arr分辨率、主控光耦设计)
 * * * 建议所有要用到的电调一起过一次校准然后配置不动
 ************************** Dongguan-University of Technology -ACE***************************/

#include "c615_driver.hpp"

namespace Motor_n
{
    C615_c::C615_c(Motor_n::C615_c::Init_t config)
    {
        if(config.min_pulse < C615_PULSE_MIN || config.max_pulse > C615_PULSE_MAX || config.pwm_freq > C615_FREQ_MAX)
        {
            while (1) {/* error config */}
        }
        pwm_cycle_ = 1000000 / config.pwm_freq;

        BSP_n::PWM_Init_t pwm_config =
            {
                .htim = config.htim,
                .channel = config.channel,
                .pwm_mode = BSP_n::PWM_NORMAL,
            };
        pwm_ = new BSP_n::PWM_c(pwm_config);
        min_ccr_ = config.min_pulse / pwm_cycle_ * (pwm_->PWMGetArr() + 1);
        max_ccr_ = config.max_pulse / pwm_cycle_ * (pwm_->PWMGetArr() + 1);
        if(config.start_mode == StartMode_e::Normal)
        {
            pwm_->PWMSetDuty(C615_PULSE_MIN / pwm_cycle_ * 100);
        }
        else
        {
            pwm_->PWMSetDuty(C615_PULSE_MAX / pwm_cycle_ * 100);
        }
    }

    void C615_c::SetSpeed(float persent)
    {
        if(persent > 99.999f || persent < 0.001f) return;
        uint32_t set_ccr = (max_ccr_ - min_ccr_) * persent * 0.01f + min_ccr_;
        pwm_->PWMSetCcr(set_ccr - 1);
    }

    void C615_c::Disable()
    {
        pwm_->PWMSetDuty(C615_PULSE_MIN / pwm_cycle_ * 100);
    }
}
