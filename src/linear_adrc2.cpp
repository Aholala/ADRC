#include "adrc/linear_adrc2.hpp"

#include <cmath>

namespace adrc
{
    bool LinearADRC2::Init(float b0, float omega_c, float omega_o, float output,
                           float rate, float disturbance)
    {
        const float kp = omega_c * omega_c;
        const float kd = 2.0f * omega_c;
        if (!std::isfinite(omega_c) || omega_c <= 0.0f ||
            !std::isfinite(kp) || kp <= 0.0f || !std::isfinite(kd) || kd <= 0.0f)
            return false;

        LinearESO3 observer;
        if (!observer.Init(b0, omega_o, output, rate, disturbance))
            return false;

        b0_ = b0;
        kp_ = kp;
        kd_ = kd;
        observer_ = observer;
        return true;
    }

    bool LinearADRC2::Reset(float output, float rate, float disturbance)
    {
        return observer_.Reset(output, rate, disturbance);
    }

    bool LinearADRC2::Calc(float target, float& command) const
    {
        if (kp_ <= 0.0f || !std::isfinite(target))
            return false;

        const float desired_acceleration = kp_ * (target - observer_.GetOutput())
                                           - kd_ * observer_.GetRate();
        const float next_command = (desired_acceleration - observer_.GetDisturbance()) / b0_;
        if (!std::isfinite(next_command))
            return false;

        command = next_command;
        return true;
    }

    bool LinearADRC2::UpdateObserver(float applied_input, float measurement, float dt)
    {
        return observer_.Update(applied_input, measurement, dt);
    }
}
