#include "adrc/ladrc1.hpp"

#include <cmath>

namespace adrc
{
    bool LADRC1::Init(float b0, float omega_c, float omega_o, float output,
                           float disturbance)
    {
        if (!std::isfinite(omega_c) || omega_c <= 0.0f)
            return false;

        LESO2 observer;
        if (!observer.Init(b0, omega_o, output, disturbance))
            return false;

        b0_ = b0;
        omega_c_ = omega_c;
        observer_ = observer;
        return true;
    }

    bool LADRC1::Reset(float output, float disturbance)
    {
        return observer_.Reset(output, disturbance);
    }

    bool LADRC1::Calc(float target, float& command) const
    {
        if (omega_c_ <= 0.0f || !std::isfinite(target))
            return false;

        const float desired_rate = omega_c_ * (target - observer_.GetOutput());
        const float next_command = (desired_rate - observer_.GetDisturbance()) / b0_;
        if (!std::isfinite(next_command))
            return false;

        command = next_command;
        return true;
    }

    bool LADRC1::UpdateObserver(float applied_input, float measurement, float dt)
    {
        return observer_.Update(applied_input, measurement, dt);
    }
}
