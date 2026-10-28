#include "adrc/adrc.hpp"
#include <cmath>

// ADRC算法库实现；周期、目标、限幅与实际输入由调用方提供。

// ============================================================================
// 基础函数：Fal
// ============================================================================

namespace adrc
{
    bool Fal(float error, float alpha, float delta, float& value)
    {
        if (!std::isfinite(error) || !std::isfinite(alpha) || alpha <= 0.0f ||
            !std::isfinite(delta) || delta <= 0.0f) return false;
        float result;
        if (std::fabs(error) < delta)
        {
            const float gain = std::pow(delta, alpha-1.0f);
            if (!std::isfinite(gain) || gain <= 0.0f) return false;
            result = error * gain;
        }
        else
            result = std::pow(std::fabs(error), alpha) * (error > 0.0f ? 1.0f : -1.0f);
        if (!std::isfinite(result)) return false;
        value = result;
        return true;
    }
}

// ============================================================================
// 跟踪微分器：LTD2
// ============================================================================

namespace adrc
{
    bool LTD2::Init(float r, float output, float rate)
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

    bool LTD2::Reset(float output, float rate)
    {
        if (!std::isfinite(output) || !std::isfinite(rate))
            return false;

        x1_ = output;
        x2_ = rate;
        return true;
    }

    bool LTD2::Update(float input, float dt)
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

// ============================================================================
// 跟踪微分器：TD2
// ============================================================================

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

// ============================================================================
// 观测器：LESO2
// ============================================================================

namespace adrc
{
    bool LESO2::Init(float b0, float omega_o, float output, float disturbance)
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

    bool LESO2::Reset(float output, float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(disturbance))
            return false;

        z1_ = output;
        z2_ = disturbance;
        return true;
    }

    bool LESO2::Update(float input, float measurement, float dt)
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

// ============================================================================
// 观测器：LESO3
// ============================================================================

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

// ============================================================================
// 观测器：RLESO1
// ============================================================================

namespace adrc
{
    bool RLESO1::Init(float b0, float omega_o, float measurement, float disturbance)
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

    bool RLESO1::Reset(float measurement, float disturbance)
    {
        if (omega_o_ <= 0.0f || !std::isfinite(measurement) || !std::isfinite(disturbance))
            return false;
        const float next_q = disturbance - omega_o_ * measurement;
        if (!std::isfinite(next_q) || !std::isfinite(next_q + omega_o_ * measurement))
            return false;
        q_ = next_q;
        return true;
    }

    bool RLESO1::Calc(float measurement, float& disturbance) const
    {
        if (omega_o_ <= 0.0f || !std::isfinite(measurement))
            return false;
        const float estimate = q_ + omega_o_ * measurement;
        if (!std::isfinite(estimate))
            return false;
        disturbance = estimate;
        return true;
    }

    bool RLESO1::Update(float input, float measurement, float dt)
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

// ============================================================================
// 观测器：ESO3
// ============================================================================

namespace adrc
{
    bool ESO3::Init(const ESO3Config& c,float output,float rate,float disturbance)
    {
        if (!std::isfinite(c.b0) || c.b0==0.0f || !std::isfinite(c.beta1) || c.beta1<=0.0f ||
            !std::isfinite(c.beta2) || c.beta2<=0.0f || !std::isfinite(c.beta3) || c.beta3<=0.0f ||
            !std::isfinite(c.alpha1) || c.alpha1<=0.0f || c.alpha1>1.0f ||
            !std::isfinite(c.alpha2) || c.alpha2<=0.0f || c.alpha2>1.0f ||
            !std::isfinite(c.delta) || c.delta<=0.0f ||
            !std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance)) return false;
        const float local2=c.beta2*std::pow(c.delta,c.alpha1-1.0f);
        const float local3=c.beta3*std::pow(c.delta,c.alpha2-1.0f);
        if (!std::isfinite(local2) || local2<=0.0f || !std::isfinite(local3) || local3<=0.0f) return false;
        config_=c; z1_=output; z2_=rate; z3_=disturbance;
        return true;
    }
    bool ESO3::Reset(float output,float rate,float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance)) return false;
        z1_=output; z2_=rate; z3_=disturbance;
        return true;
    }
    bool ESO3::Update(float input,float measurement,float dt)
    {
        if (config_.beta1<=0.0f || !std::isfinite(input) || !std::isfinite(measurement) ||
            !std::isfinite(dt) || dt<=0.0f) return false;
        const float e=z1_-measurement;
        float f1=0.0f,f2=0.0f;
        if (!Fal(e,config_.alpha1,config_.delta,f1) || !Fal(e,config_.alpha2,config_.delta,f2)) return false;
        const float n1=z1_+dt*(z2_-config_.beta1*e);
        const float n2=z2_+dt*(z3_-config_.beta2*f1+config_.b0*input);
        const float n3=z3_+dt*(-config_.beta3*f2);
        if (!std::isfinite(n1) || !std::isfinite(n2) || !std::isfinite(n3)) return false;
        z1_=n1; z2_=n2; z3_=n3;
        return true;
    }
}

// ============================================================================
// 控制器：LADRC1
// ============================================================================

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

// ============================================================================
// 控制器：LADRC2
// ============================================================================

namespace adrc
{
    bool LADRC2::Init(float b0, float omega_c, float omega_o, float output,
                           float rate, float disturbance)
    {
        const float kp = omega_c * omega_c;
        const float kd = 2.0f * omega_c;
        if (!std::isfinite(omega_c) || omega_c <= 0.0f ||
            !std::isfinite(kp) || kp <= 0.0f || !std::isfinite(kd) || kd <= 0.0f)
            return false;

        LESO3 observer;
        if (!observer.Init(b0, omega_o, output, rate, disturbance))
            return false;

        b0_ = b0;
        kp_ = kp;
        kd_ = kd;
        observer_ = observer;
        return true;
    }

    bool LADRC2::Reset(float output, float rate, float disturbance)
    {
        return observer_.Reset(output, rate, disturbance);
    }

    bool LADRC2::Calc(float target, float& command) const
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

    bool LADRC2::UpdateObserver(float applied_input, float measurement, float dt)
    {
        return observer_.Update(applied_input, measurement, dt);
    }
}

// ============================================================================
// 误差反馈：SEF2
// ============================================================================

namespace adrc
{
    bool SEF2::Init(float b0,float b1,float a1,float b2,float a2,float delta)
    {
        if (!std::isfinite(b0) || b0==0.0f || !std::isfinite(b1) || b1<=0.0f ||
            !std::isfinite(b2) || b2<=0.0f || !std::isfinite(a1) || a1<=0.0f ||
            !std::isfinite(a2) || a2<=0.0f || !std::isfinite(delta) || delta<=0.0f) return false;
        const float local1=b1*std::pow(delta,a1-1.0f),local2=b2*std::pow(delta,a2-1.0f);
        if (!std::isfinite(local1) || local1<=0.0f || !std::isfinite(local2) || local2<=0.0f) return false;
        b0_=b0; beta1_=b1; beta2_=b2; alpha1_=a1; alpha2_=a2; delta_=delta;
        return true;
    }
    bool SEF2::Calc(float target,float target_rate,float output,float rate,
                                   float disturbance,float& command) const
    {
        if (b0_==0.0f || !std::isfinite(target) || !std::isfinite(target_rate) ||
            !std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance)) return false;
        float f1=0.0f,f2=0.0f;
        if (!Fal(target-output,alpha1_,delta_,f1) || !Fal(target_rate-rate,alpha2_,delta_,f2)) return false;
        const float result=(beta1_*f1+beta2_*f2-disturbance)/b0_;
        if (!std::isfinite(result)) return false;
        command=result;
        return true;
    }
}
