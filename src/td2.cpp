#include "adrc/td2.hpp"

#include <cmath>

namespace
{
    float Sign(float value)
    {
        return value > 0.0f ? 1.0f : (value < 0.0f ? -1.0f : 0.0f);
    }
}

namespace adrc
{
    bool TD2::Init(float r, float h0, float output, float rate)
    {
        if (!std::isfinite(r) || r <= 0.0f || !std::isfinite(h0) || h0 <= 0.0f ||
            !std::isfinite(output) || !std::isfinite(rate))
            return false;
        const float d = r * h0;
        const float d0 = h0 * d;
        const float d_squared = d * d;
        const float eight_r = 8.0f * r;
        if (!std::isfinite(d) || d <= 0.0f || !std::isfinite(d0) || d0 <= 0.0f ||
            !std::isfinite(d_squared) || d_squared <= 0.0f || !std::isfinite(eight_r))
            return false;

        r_ = r; h0_ = h0;
        x1_ = output; x2_ = rate;
        return true;
    }

    bool TD2::Reset(float output, float rate)
    {
        if (!std::isfinite(output) || !std::isfinite(rate))
            return false;
        x1_ = output; x2_ = rate;
        return true;
    }

    bool TD2::Calc(float input, float& acceleration) const
    {
        if (r_ <= 0.0f || !std::isfinite(input))
            return false;
        // 逐式保留原fhan；局部h接收外部h0，不能换成实际dt。
        const float error = x1_ - input;
        const float d = r_ * h0_;
        const float d0 = h0_ * d;
        const float y = error + h0_ * x2_;
        const float radicand = d * d + 8.0f * r_ * std::fabs(y);
        if (!std::isfinite(error) || !std::isfinite(y) || !std::isfinite(radicand))
            return false;
        const float a0 = std::sqrt(radicand);
        float a = 0.0f;
        if (std::fabs(y) > d0)
            a = x2_ + (a0 - d) * Sign(y) / 2.0f;
        else
            a = x2_ + y / h0_;
        if (!std::isfinite(a))
            return false;
        const float result = std::fabs(a) > d ? -r_ * Sign(a) : -r_ * a / d;
        if (!std::isfinite(result))
            return false;
        acceleration = result;
        return true;
    }

    bool TD2::Update(float input, float dt)
    {
        if (r_ <= 0.0f || !std::isfinite(dt) || dt <= 0.0f)
            return false;
        const float ratio = dt / h0_;
        if (!std::isfinite(ratio) || ratio <= 0.0f || ratio >= 2.0f)
            return false;
        float acceleration = 0.0f;
        if (!Calc(input, acceleration))
            return false;
        const float next_x1 = x1_ + dt * x2_;
        const float next_x2 = x2_ + dt * acceleration;
        if (!std::isfinite(next_x1) || !std::isfinite(next_x2))
            return false;
        x1_ = next_x1; x2_ = next_x2;
        return true;
    }
}
