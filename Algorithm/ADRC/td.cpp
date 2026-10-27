#include "td.hpp"

#include <cmath>

namespace alg_n
{
    bool LinearTD_c::Init(float bandwidth, float value, float rate)
    {
        if (!std::isfinite(bandwidth) || bandwidth <= 0.0f ||
            !std::isfinite(bandwidth * bandwidth) ||
            !std::isfinite(value) || !std::isfinite(rate))
        {
            return false;
        }

        bandwidth_ = bandwidth;
        value_ = value;
        rate_ = rate;
        return true;
    }

    bool LinearTD_c::Reset(float value, float rate)
    {
        if (!std::isfinite(value) || !std::isfinite(rate))
        {
            return false;
        }

        value_ = value;
        rate_ = rate;
        return true;
    }

    bool LinearTD_c::Calc(float input, float dt)
    {
        if (bandwidth_ <= 0.0f || !std::isfinite(input) ||
            !std::isfinite(dt) || dt <= 0.0f)
        {
            return false;
        }

        // 两个离散极点均为 1 - omega * dt，稳定条件为 |1 - omega * dt| < 1。
        const float step = bandwidth_ * dt;
        if (!std::isfinite(step) || step <= 0.0f || step >= 2.0f)
        {
            return false;
        }

        // 两个新状态都使用旧状态计算，保持与 MATLAB 的 sys = f(x,u) 一致。
        const float acceleration = -bandwidth_ * bandwidth_ * (value_ - input)
                                   - 2.0f * bandwidth_ * rate_;
        const float next_value = value_ + dt * rate_;
        const float next_rate = rate_ + dt * acceleration;
        if (!std::isfinite(next_value) || !std::isfinite(next_rate))
        {
            return false;
        }

        value_ = next_value;
        rate_ = next_rate;
        return true;
    }
}
