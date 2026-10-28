#include "adrc/linear_td2.hpp"

#include <cmath>

namespace adrc
{
    bool LinearTD2::Init(float r, float output, float rate)
    {
        const float r_squared = r * r;
        const float damping = 2.0f * r;
        if (!std::isfinite(r) || r <= 0.0f ||
            !std::isfinite(r_squared) || r_squared <= 0.0f ||
            !std::isfinite(damping) || damping <= 0.0f ||
            !std::isfinite(output) || !std::isfinite(rate))
            return false;

        r_ = r;
        r_squared_ = r_squared;
        damping_ = damping;
        x1_ = output;
        x2_ = rate;
        return true;
    }

    bool LinearTD2::Reset(float output, float rate)
    {
        if (!std::isfinite(output) || !std::isfinite(rate))
            return false;

        x1_ = output;
        x2_ = rate;
        return true;
    }

    bool LinearTD2::Update(float input, float dt)
    {
        if (r_ <= 0.0f || !std::isfinite(input) || !std::isfinite(dt) || dt <= 0.0f)
            return false;
        const float step = r_ * dt;
        if (!std::isfinite(step) || step <= 0.0f || step >= 2.0f)
            return false;

        const float next_x1 = x1_ + dt * x2_;
        const float next_x2 = x2_ + dt * (-r_squared_ * (x1_ - input) - damping_ * x2_);
        if (!std::isfinite(next_x1) || !std::isfinite(next_x2))
            return false;

        x1_ = next_x1;
        x2_ = next_x2;
        return true;
    }
}
