#include "adrc/leso3.hpp"

#include <cmath>

namespace adrc
{
    bool LESO3::Init(float b0, float omega_o, float output,
                          float rate, float disturbance)
    {
        const float omega_squared = omega_o * omega_o;
        const float beta1 = 3.0f * omega_o;
        const float beta2 = 3.0f * omega_squared;
        const float beta3 = omega_squared * omega_o;
        if (!std::isfinite(b0) || b0 == 0.0f ||
            !std::isfinite(omega_o) || omega_o <= 0.0f ||
            !std::isfinite(beta1) || beta1 <= 0.0f ||
            !std::isfinite(beta2) || beta2 <= 0.0f ||
            !std::isfinite(beta3) || beta3 <= 0.0f ||
            !std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance))
        {
            return false;
        }

        b0_ = b0;
        omega_o_ = omega_o;
        beta1_ = beta1;
        beta2_ = beta2;
        beta3_ = beta3;
        z1_ = output;
        z2_ = rate;
        z3_ = disturbance;
        return true;
    }

    bool LESO3::Reset(float output, float rate, float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance))
            return false;

        z1_ = output;
        z2_ = rate;
        z3_ = disturbance;
        return true;
    }

    bool LESO3::Update(float input, float measurement, float dt)
    {
        if (omega_o_ <= 0.0f || !std::isfinite(input) ||
            !std::isfinite(measurement) || !std::isfinite(dt) || dt <= 0.0f)
        {
            return false;
        }
        const float step = omega_o_ * dt;
        if (!std::isfinite(step) || step <= 0.0f || step >= 2.0f)
            return false;

        // 三个右侧均使用同一拍旧状态，最后统一保存下一拍预测。
        const float error = z1_ - measurement;
        const float next_z1 = z1_ + dt * (z2_ - beta1_ * error);
        const float next_z2 = z2_ + dt * (z3_ + b0_ * input - beta2_ * error);
        const float next_z3 = z3_ - dt * beta3_ * error;
        if (!std::isfinite(next_z1) || !std::isfinite(next_z2) || !std::isfinite(next_z3))
            return false;

        z1_ = next_z1;
        z2_ = next_z2;
        z3_ = next_z3;
        return true;
    }
}
