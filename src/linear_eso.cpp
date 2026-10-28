#include "adrc/linear_eso.hpp"

#include <cmath>

namespace adrc
{
    bool LinearESO2::Init(float b0, float omega_o, float output, float disturbance)
    {
        const float beta1 = 2.0f * omega_o;
        const float beta2 = omega_o * omega_o;
        if (!std::isfinite(b0) || b0 == 0.0f ||
            !std::isfinite(omega_o) || omega_o <= 0.0f ||
            !std::isfinite(beta1) || !std::isfinite(beta2) || beta2 <= 0.0f ||
            !std::isfinite(output) || !std::isfinite(disturbance))
        {
            return false;
        }

        b0_ = b0;
        omega_o_ = omega_o;
        beta1_ = beta1;
        beta2_ = beta2;
        z1_ = output;
        z2_ = disturbance;
        return true;
    }

    bool LinearESO2::Reset(float output, float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(disturbance))
            return false;

        z1_ = output;
        z2_ = disturbance;
        return true;
    }

    bool LinearESO2::Update(float input, float measurement, float dt)
    {
        if (omega_o_ <= 0.0f || !std::isfinite(input) ||
            !std::isfinite(measurement) || !std::isfinite(dt) || dt <= 0.0f)
        {
            return false;
        }
        const float step = omega_o_ * dt;
        if (!std::isfinite(step) || step <= 0.0f || step >= 2.0f)
            return false;

        // 两个新状态均使用旧 z1、z2；这里的误差方向与参考 MATLAB 一致。
        const float error = z1_ - measurement;
        const float next_z1 = z1_ + dt * (z2_ + b0_ * input - beta1_ * error);
        const float next_z2 = z2_ - dt * beta2_ * error;
        if (!std::isfinite(next_z1) || !std::isfinite(next_z2))
            return false;

        z1_ = next_z1;
        z2_ = next_z2;
        return true;
    }
}
