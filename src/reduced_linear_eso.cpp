#include "adrc/reduced_linear_eso.hpp"

#include <cmath>

namespace adrc
{
    bool ReducedLinearESO1::Init(float b0, float omega_o, float measurement, float disturbance)
    {
        if (!std::isfinite(b0) || b0 == 0.0f ||
            !std::isfinite(omega_o) || omega_o <= 0.0f ||
            !std::isfinite(measurement) || !std::isfinite(disturbance))
            return false;

        const float next_q = disturbance - omega_o * measurement;
        if (!std::isfinite(next_q) || !std::isfinite(next_q + omega_o * measurement))
            return false;

        b0_ = b0;
        omega_o_ = omega_o;
        q_ = next_q;
        return true;
    }

    bool ReducedLinearESO1::Reset(float measurement, float disturbance)
    {
        if (omega_o_ <= 0.0f || !std::isfinite(measurement) || !std::isfinite(disturbance))
            return false;
        const float next_q = disturbance - omega_o_ * measurement;
        if (!std::isfinite(next_q) || !std::isfinite(next_q + omega_o_ * measurement))
            return false;
        q_ = next_q;
        return true;
    }

    bool ReducedLinearESO1::Calc(float measurement, float& disturbance) const
    {
        if (omega_o_ <= 0.0f || !std::isfinite(measurement))
            return false;
        const float estimate = q_ + omega_o_ * measurement;
        if (!std::isfinite(estimate))
            return false;
        disturbance = estimate;
        return true;
    }

    bool ReducedLinearESO1::Update(float input, float measurement, float dt)
    {
        if (omega_o_ <= 0.0f || !std::isfinite(input) ||
            !std::isfinite(dt) || dt <= 0.0f)
            return false;
        const float step = omega_o_ * dt;
        if (!std::isfinite(step) || step <= 0.0f || step >= 2.0f)
            return false;

        float disturbance = 0.0f;
        if (!Calc(measurement, disturbance))
            return false;
        // 与MATLAB保持相同运算次序；先计算候选值，成功后才提交状态。
        const float next_q = q_ - dt * omega_o_ * (disturbance + b0_ * input);
        if (!std::isfinite(next_q))
            return false;
        q_ = next_q;
        return true;
    }
}
