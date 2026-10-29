/**************** Dongguan-University of Technology -ACE****************
 * @file    ALG_ADRC.cpp
 * @author  Ahola
 * @version v27.0.0.0
 * @date    2026-10-28
 * @brief   ADRC算法
 ************************************************************************
 * @verbatim
 * 参考https://github.com/TianfaYao/ADRC
 * @attention
 *
 * @version           time
 * v27.0.0.0  全新的体验  2026-10-28
 **************** Dongguan-University of Technology -ACE****************/

#include "ALG_ADRC.hpp"
#include <cmath>

namespace ADRC
{
namespace
{
    bool ValidLimit(float limit)
    {
        return !std::isnan(limit) && limit >= 0.0f;
    }

    float Limit(float value, float limit)
    {
        return value > limit ? limit : (value < -limit ? -limit : value);
    }

    float Sign(float value)
    {
        return value > 0.0f ? 1.0f : (value < 0.0f ? -1.0f : 0.0f);
    }

    // 内部调用的参数已经在Init/SetParameters中检查，不重复走公共Fal接口。
    float FalValue(float error, float alpha, float delta)
    {
        if (std::fabs(error) < delta)
            return error * std::pow(delta, alpha - 1.0f);
        return std::pow(std::fabs(error), alpha) * Sign(error);
    }

}


// ============================================================================
// 基础函数：Fal
// ============================================================================

    bool Fal(float error, float alpha, float delta, float& value)
    {
        if (!std::isfinite(alpha) || alpha <= 0.0f ||
            !std::isfinite(delta) || delta <= 0.0f) return false;
        const float result = FalValue(error, alpha, delta);
        if (!std::isfinite(result)) return false;
        value = result;
        return true;
    }

// ============================================================================
// 线性跟踪微分器：LTD2
// ============================================================================

    bool LTD2::Init(float r, float output, float rate)
    {
        const float r_squared = r * r;
        const float damping = 2.0f * r;
        if (!std::isfinite(r) || r <= 0.0f ||
            !std::isfinite(r_squared) || r_squared <= 0.0f ||
            !std::isfinite(damping) || damping <= 0.0f ||
            !std::isfinite(output) || !std::isfinite(rate) || std::fabs(rate) > max_rate_)
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
        if (!std::isfinite(output) || !std::isfinite(rate) || std::fabs(rate) > max_rate_)
            return false;

        x1_ = output;
        x2_ = rate;
        return true;
    }

    bool LTD2::Update(float input, float dt)
    {
        if (!std::isfinite(dt) || dt <= 0.0f)
            return false;

        float acceleration = 0.0f;
        if (!Calc(input, acceleration))
            return false;
        const float next_x1 = x1_ + dt * x2_;
        const float raw_rate = x2_ + dt * acceleration;
        if (!std::isfinite(next_x1) || !std::isfinite(raw_rate))
            return false;
        const float next_x2 = Limit(raw_rate, max_rate_);

        x1_ = next_x1;
        x2_ = next_x2;
        return true;
    }

    bool LTD2::SetParameters(float r)
    {
        if (r_ <= 0.0f) return false;
        LTD2 candidate = *this;
        if (!candidate.Init(r, x1_, x2_)) return false;
        *this = candidate;
        return true;
    }

    bool LTD2::SetLimits(float max_rate, float max_acceleration)
    {
        if (!ValidLimit(max_rate) || !ValidLimit(max_acceleration) ||
            std::fabs(x2_) > max_rate) return false;
        max_rate_ = max_rate;
        max_acceleration_ = max_acceleration;
        return true;
    }

    bool LTD2::Calc(float input, float& acceleration) const
    {
        if (r_ <= 0.0f) return false;
        const float result = -r_squared_ * (x1_ - input) - damping_ * x2_;
        if (!std::isfinite(result)) return false;
        acceleration = Limit(result, max_acceleration_);
        return true;
    }

// ============================================================================
// 跟踪微分器：TD2
// ============================================================================

    bool TD2::Init(float r, float h0, float output, float rate)
    {
        if (!std::isfinite(r) || r <= 0.0f || !std::isfinite(h0) || h0 <= 0.0f ||
            !std::isfinite(output) || !std::isfinite(rate) || std::fabs(rate) > max_rate_)
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
        if (!std::isfinite(output) || !std::isfinite(rate) || std::fabs(rate) > max_rate_)
            return false;
        x1_ = output; x2_ = rate;
        return true;
    }

    bool TD2::Calc(float input, float& acceleration) const
    {
        if (r_ <= 0.0f)
            return false;
        const float error = x1_ - input;
        const float d = r_ * h0_;
        const float d0 = h0_ * d;
        const float y = error + h0_ * x2_;
        const float radicand = d * d + 8.0f * r_ * std::fabs(y);
        if (!std::isfinite(radicand))
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
        acceleration = Limit(result, max_acceleration_);
        return true;
    }

    bool TD2::Update(float input, float dt)
    {
        if (!std::isfinite(dt) || dt <= 0.0f)
            return false;
        float acceleration = 0.0f;
        if (!Calc(input, acceleration))
            return false;
        const float next_x1 = x1_ + dt * x2_;
        const float raw_rate = x2_ + dt * acceleration;
        if (!std::isfinite(next_x1) || !std::isfinite(raw_rate))
            return false;
        const float next_x2 = Limit(raw_rate, max_rate_);
        x1_ = next_x1; x2_ = next_x2;
        return true;
    }

    bool TD2::SetParameters(float r, float h0)
    {
        if (r_ <= 0.0f) return false;
        TD2 candidate = *this;
        if (!candidate.Init(r, h0, x1_, x2_)) return false;
        *this = candidate;
        return true;
    }

    bool TD2::SetLimits(float max_rate, float max_acceleration)
    {
        if (!ValidLimit(max_rate) || !ValidLimit(max_acceleration) ||
            std::fabs(x2_) > max_rate) return false;
        max_rate_ = max_rate;
        max_acceleration_ = max_acceleration;
        return true;
    }

// ============================================================================
// 观测器：LESO2
// ============================================================================

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
        beta1_ = beta1;
        beta2_ = beta2;
        z1_ = output;
        z2_ = Limit(disturbance, disturbance_limit_);
        return true;
    }

    bool LESO2::Reset(float output, float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(disturbance))
            return false;

        z1_ = output;
        z2_ = Limit(disturbance, disturbance_limit_);
        return true;
    }

    bool LESO2::Update(float input, float measurement, float dt)
    {
        if (b0_ == 0.0f || !std::isfinite(dt) || dt <= 0.0f)
        {
            return false;
        }
        const float error = z1_ - measurement;
        const float next_z1 = z1_ + dt * (z2_ + b0_ * input - beta1_ * error);
        const float next_z2 = z2_ - dt * beta2_ * error;
        if (!std::isfinite(next_z1) || !std::isfinite(next_z2))
            return false;

        z1_ = next_z1;
        z2_ = Limit(next_z2, disturbance_limit_);
        return true;
    }

    bool LESO2::Init(const LESO2Config& c, float output, float disturbance)
    {
        if (!std::isfinite(c.b0) || c.b0 == 0.0f ||
            !std::isfinite(c.beta1) || c.beta1 <= 0.0f ||
            !std::isfinite(c.beta2) || c.beta2 <= 0.0f ||
            !std::isfinite(output) || !std::isfinite(disturbance)) return false;
        b0_ = c.b0; beta1_ = c.beta1; beta2_ = c.beta2;
        z1_ = output; z2_ = Limit(disturbance, disturbance_limit_);
        return true;
    }

    bool LESO2::SetParameters(float b0, float omega_o)
    {
        if (b0_ == 0.0f) return false;
        LESO2 candidate = *this;
        if (!candidate.Init(b0, omega_o, z1_, z2_)) return false;
        candidate.z2_ = z2_; // 改参只改参数，即使限制刚刚收紧也不提前截断状态。
        *this = candidate;
        return true;
    }

    bool LESO2::SetParameters(const LESO2Config& config)
    {
        if (b0_ == 0.0f) return false;
        LESO2 candidate = *this;
        if (!candidate.Init(config, z1_, z2_)) return false;
        candidate.z2_ = z2_;
        *this = candidate;
        return true;
    }

    bool LESO2::SetDisturbanceLimit(float limit)
    {
        if (!ValidLimit(limit)) return false;
        disturbance_limit_ = limit;
        return true;
    }

// ============================================================================
// 观测器：LESO3
// ============================================================================

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
        beta1_ = beta1;
        beta2_ = beta2;
        beta3_ = beta3;
        z1_ = output;
        z2_ = rate;
        z3_ = Limit(disturbance, disturbance_limit_);
        return true;
    }

    bool LESO3::Reset(float output, float rate, float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance))
            return false;

        z1_ = output;
        z2_ = rate;
        z3_ = Limit(disturbance, disturbance_limit_);
        return true;
    }

    bool LESO3::Update(float input, float measurement, float dt)
    {
        if (b0_ == 0.0f || !std::isfinite(dt) || dt <= 0.0f)
        {
            return false;
        }
        const float error = z1_ - measurement;
        const float next_z1 = z1_ + dt * (z2_ - beta1_ * error);
        const float next_z2 = z2_ + dt * (z3_ + b0_ * input - beta2_ * error);
        const float next_z3 = z3_ - dt * beta3_ * error;
        if (!std::isfinite(next_z1) || !std::isfinite(next_z2) || !std::isfinite(next_z3))
            return false;

        z1_ = next_z1;
        z2_ = next_z2;
        z3_ = Limit(next_z3, disturbance_limit_);
        return true;
    }

    bool LESO3::Init(const LESO3Config& c, float output, float rate, float disturbance)
    {
        if (!std::isfinite(c.b0) || c.b0 == 0.0f ||
            !std::isfinite(c.beta1) || c.beta1 <= 0.0f ||
            !std::isfinite(c.beta2) || c.beta2 <= 0.0f ||
            !std::isfinite(c.beta3) || c.beta3 <= 0.0f ||
            !std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance)) return false;
        b0_ = c.b0; beta1_ = c.beta1; beta2_ = c.beta2; beta3_ = c.beta3;
        z1_ = output; z2_ = rate; z3_ = Limit(disturbance, disturbance_limit_);
        return true;
    }

    bool LESO3::SetParameters(float b0, float omega_o)
    {
        if (b0_ == 0.0f) return false;
        LESO3 candidate = *this;
        if (!candidate.Init(b0, omega_o, z1_, z2_, z3_)) return false;
        candidate.z3_ = z3_;
        *this = candidate;
        return true;
    }

    bool LESO3::SetParameters(const LESO3Config& config)
    {
        if (b0_ == 0.0f) return false;
        LESO3 candidate = *this;
        if (!candidate.Init(config, z1_, z2_, z3_)) return false;
        candidate.z3_ = z3_;
        *this = candidate;
        return true;
    }

    bool LESO3::SetDisturbanceLimit(float limit)
    {
        if (!ValidLimit(limit)) return false;
        disturbance_limit_ = limit;
        return true;
    }

// ============================================================================
// 观测器：RLESO1
// ============================================================================

    bool RLESO1::Init(float b0, float omega_o, float measurement, float disturbance)
    {
        if (!std::isfinite(b0) || b0 == 0.0f ||
            !std::isfinite(omega_o) || omega_o <= 0.0f ||
            !std::isfinite(measurement) || !std::isfinite(disturbance))
            return false;

        const float next_q = Limit(disturbance, disturbance_limit_) - omega_o * measurement;
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
        const float next_q = Limit(disturbance, disturbance_limit_) - omega_o_ * measurement;
        if (!std::isfinite(next_q) || !std::isfinite(next_q + omega_o_ * measurement))
            return false;
        q_ = next_q;
        return true;
    }

    bool RLESO1::Calc(float measurement, float& disturbance) const
    {
        if (omega_o_ <= 0.0f)
            return false;
        const float estimate = q_ + omega_o_ * measurement;
        if (!std::isfinite(estimate))
            return false;
        disturbance = Limit(estimate, disturbance_limit_);
        return true;
    }

    bool RLESO1::Update(float input, float measurement, float dt)
    {
        if (!std::isfinite(dt) || dt <= 0.0f)
            return false;
        float disturbance = 0.0f;
        if (!Calc(measurement, disturbance))
            return false;
        // 与MATLAB保持相同运算次序；先计算候选值，成功后才提交状态。
        float next_q = q_ - dt * omega_o_ * (disturbance + b0_ * input);
        if (!std::isfinite(next_q))
            return false;
        if (std::isfinite(disturbance_limit_))
        {
            const float reconstructed = next_q + omega_o_ * measurement;
            if (!std::isfinite(reconstructed)) return false;
            next_q = Limit(reconstructed, disturbance_limit_) - omega_o_ * measurement;
            if (!std::isfinite(next_q)) return false;
        }
        q_ = next_q;
        return true;
    }

    bool RLESO1::SetParameters(float b0, float omega_o, float measurement)
    {
        float disturbance = 0.0f;
        if (!Calc(measurement, disturbance)) return false;
        RLESO1 candidate = *this;
        if (!candidate.Init(b0, omega_o, measurement, disturbance)) return false;
        *this = candidate;
        return true;
    }

    bool RLESO1::SetDisturbanceLimit(float limit)
    {
        if (!ValidLimit(limit)) return false;
        disturbance_limit_ = limit;
        return true;
    }

// ============================================================================
// 观测器：ESO2
// ============================================================================

    bool ESO2::Init(const ESO2Config& c, float output, float disturbance)
    {
        if (!std::isfinite(c.b0) || c.b0==0.0f || !std::isfinite(c.beta1) || c.beta1<=0.0f ||
            !std::isfinite(c.beta2) || c.beta2<=0.0f || !std::isfinite(c.alpha) ||
            c.alpha<=0.0f || c.alpha>1.0f || !std::isfinite(c.delta) || c.delta<=0.0f ||
            !std::isfinite(output) || !std::isfinite(disturbance)) return false;
        const float effective_beta2=c.beta2*std::pow(c.delta,c.alpha-1.0f);
        if (!std::isfinite(effective_beta2) || effective_beta2<=0.0f) return false;
        config_=c; z1_=output; z2_=Limit(disturbance, disturbance_limit_);
        return true;
    }

    bool ESO2::Reset(float output, float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(disturbance)) return false;
        z1_=output; z2_=Limit(disturbance, disturbance_limit_);
        return true;
    }

    bool ESO2::Update(float input, float measurement, float dt)
    {
        if (config_.beta1<=0.0f || !std::isfinite(dt) || dt<=0.0f) return false;
        const float error=z1_-measurement;
        const float correction=FalValue(error,config_.alpha,config_.delta);
        const float n1=z1_+dt*(z2_+config_.b0*input-config_.beta1*error);
        const float n2=z2_-dt*config_.beta2*correction;
        if (!std::isfinite(n1) || !std::isfinite(n2)) return false;
        z1_=n1; z2_=Limit(n2, disturbance_limit_);
        return true;
    }

    bool ESO2::SetParameters(const ESO2Config& config)
    {
        if (config_.beta1 <= 0.0f) return false;
        ESO2 candidate = *this;
        if (!candidate.Init(config, z1_, z2_)) return false;
        candidate.z2_ = z2_;
        *this = candidate;
        return true;
    }

    bool ESO2::SetDisturbanceLimit(float limit)
    {
        if (!ValidLimit(limit)) return false;
        disturbance_limit_ = limit;
        return true;
    }

// ============================================================================
// 观测器：ESO3
// ============================================================================

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
        config_=c; z1_=output; z2_=rate; z3_=Limit(disturbance, disturbance_limit_);
        return true;
    }
    bool ESO3::Reset(float output,float rate,float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance)) return false;
        z1_=output; z2_=rate; z3_=Limit(disturbance, disturbance_limit_);
        return true;
    }
    bool ESO3::Update(float input,float measurement,float dt)
    {
        if (config_.beta1<=0.0f || !std::isfinite(dt) || dt<=0.0f) return false;
        const float e=z1_-measurement;
        const float f1=FalValue(e,config_.alpha1,config_.delta);
        const float f2=FalValue(e,config_.alpha2,config_.delta);
        const float n1=z1_+dt*(z2_-config_.beta1*e);
        const float n2=z2_+dt*(z3_-config_.beta2*f1+config_.b0*input);
        const float n3=z3_+dt*(-config_.beta3*f2);
        if (!std::isfinite(n1) || !std::isfinite(n2) || !std::isfinite(n3)) return false;
        z1_=n1; z2_=n2; z3_=Limit(n3, disturbance_limit_);
        return true;
    }

    bool ESO3::SetParameters(const ESO3Config& config)
    {
        if (config_.beta1 <= 0.0f) return false;
        ESO3 candidate = *this;
        if (!candidate.Init(config, z1_, z2_, z3_)) return false;
        candidate.z3_ = z3_;
        *this = candidate;
        return true;
    }

    bool ESO3::SetDisturbanceLimit(float limit)
    {
        if (!ValidLimit(limit)) return false;
        disturbance_limit_ = limit;
        return true;
    }

// ============================================================================
// 误差反馈：NLESF1
// ============================================================================

    bool NLESF1::Init(float b0, float beta, float alpha, float delta)
    {
        if (!std::isfinite(b0) || b0==0.0f || !std::isfinite(beta) || beta<=0.0f ||
            !std::isfinite(alpha) || alpha<=0.0f || !std::isfinite(delta) || delta<=0.0f) return false;
        const float effective_gain=beta*std::pow(delta,alpha-1.0f);
        if (!std::isfinite(effective_gain) || effective_gain<=0.0f) return false;
        b0_=b0; beta_=beta; alpha_=alpha; delta_=delta;
        return true;
    }

    bool NLESF1::Calc(float target, float output, float disturbance, float& command) const
    {
        if (b0_==0.0f) return false;
        const float correction=FalValue(target-output,alpha_,delta_);
        const float result=(beta_*correction-disturbance)/b0_;
        if (!std::isfinite(result)) return false;
        command=result;
        return true;
    }

    bool NLESF1::SetParameters(float b0, float beta, float alpha, float delta)
    {
        if (b0_ == 0.0f) return false;
        return Init(b0, beta, alpha, delta);
    }

// ============================================================================
// 误差反馈：NLESF2
// ============================================================================

    bool NLESF2::Init(float b0,float b1,float a1,float b2,float a2,float delta)
    {
        if (!std::isfinite(b0) || b0==0.0f || !std::isfinite(b1) || b1<=0.0f ||
            !std::isfinite(b2) || b2<=0.0f || !std::isfinite(a1) || a1<=0.0f ||
            !std::isfinite(a2) || a2<=0.0f || !std::isfinite(delta) || delta<=0.0f) return false;
        const float local1=b1*std::pow(delta,a1-1.0f),local2=b2*std::pow(delta,a2-1.0f);
        if (!std::isfinite(local1) || local1<=0.0f || !std::isfinite(local2) || local2<=0.0f) return false;
        b0_=b0; beta1_=b1; beta2_=b2; alpha1_=a1; alpha2_=a2; delta_=delta;
        return true;
    }
    bool NLESF2::Calc(float target,float target_rate,float output,float rate,
                                   float disturbance,float& command) const
    {
        if (b0_==0.0f) return false;
        const float f1=FalValue(target-output,alpha1_,delta_);
        const float f2=FalValue(target_rate-rate,alpha2_,delta_);
        const float result=(beta1_*f1+beta2_*f2-disturbance)/b0_;
        if (!std::isfinite(result)) return false;
        command=result;
        return true;
    }

    bool NLESF2::SetParameters(float b0,float b1,float a1,float b2,float a2,float delta)
    {
        if (b0_ == 0.0f) return false;
        return Init(b0, b1, a1, b2, a2, delta);
    }



//完整线性控制器
//LADRC1
    bool LADRC1::Init(float b0, float omega_c, float omega_o, float output,
                           float disturbance)
    {
        if (!std::isfinite(omega_c) || omega_c <= 0.0f)
            return false;

        LESO2 observer = observer_;
        if (!observer.Init(b0, omega_o, output, disturbance))
            return false;

        b0_ = b0;
        omega_c_ = omega_c;
        observer_ = observer;
        return true;
    }

    bool LADRC1::Init(const LESO2Config& config, float omega_c, float output, float disturbance)
    {
        if (!std::isfinite(omega_c) || omega_c <= 0.0f) return false;
        LESO2 observer = observer_;
        if (!observer.Init(config, output, disturbance)) return false;
        b0_ = config.b0; omega_c_ = omega_c; observer_ = observer;
        return true;
    }

    bool LADRC1::SetParameters(float b0, float omega_c, float omega_o)
    {
        if (omega_c_ <= 0.0f || !std::isfinite(omega_c) || omega_c <= 0.0f) return false;
        LESO2 observer = observer_;
        if (!observer.SetParameters(b0, omega_o)) return false;
        b0_ = b0; omega_c_ = omega_c; observer_ = observer;
        return true;
    }

    bool LADRC1::SetParameters(const LESO2Config& config, float omega_c)
    {
        if (omega_c_ <= 0.0f || !std::isfinite(omega_c) || omega_c <= 0.0f) return false;
        LESO2 observer = observer_;
        if (!observer.SetParameters(config)) return false;
        b0_ = config.b0; omega_c_ = omega_c; observer_ = observer;
        return true;
    }

    bool LADRC1::SetDisturbanceLimit(float limit)
    {
        return observer_.SetDisturbanceLimit(limit);
    }

    bool LADRC1::Reset(float output, float disturbance)
    {
        return observer_.Reset(output, disturbance);
    }

    bool LADRC1::Calc(float target, float& command) const
    {
        if (omega_c_ <= 0.0f)
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

//LADRC2
    bool LADRC2::Init(float b0, float omega_c, float omega_o, float output,
                           float rate, float disturbance)
    {
        const float kp = omega_c * omega_c;
        const float kd = 2.0f * omega_c;
        if (!std::isfinite(omega_c) || omega_c <= 0.0f ||
            !std::isfinite(kp) || kp <= 0.0f || !std::isfinite(kd) || kd <= 0.0f)
            return false;

        LESO3 observer = observer_;
        if (!observer.Init(b0, omega_o, output, rate, disturbance))
            return false;

        b0_ = b0;
        kp_ = kp;
        kd_ = kd;
        observer_ = observer;
        return true;
    }

    bool LADRC2::Init(const LESO3Config& config, float omega_c, float output,
                      float rate, float disturbance)
    {
        const float kp = omega_c * omega_c, kd = 2.0f * omega_c;
        if (!std::isfinite(omega_c) || omega_c <= 0.0f ||
            !std::isfinite(kp) || kp <= 0.0f || !std::isfinite(kd) || kd <= 0.0f) return false;
        LESO3 observer = observer_;
        if (!observer.Init(config, output, rate, disturbance)) return false;
        b0_ = config.b0; kp_ = kp; kd_ = kd; observer_ = observer;
        return true;
    }

    bool LADRC2::SetParameters(float b0, float omega_c, float omega_o)
    {
        const float kp = omega_c * omega_c, kd = 2.0f * omega_c;
        if (kp_ <= 0.0f || !std::isfinite(omega_c) || omega_c <= 0.0f ||
            !std::isfinite(kp) || kp <= 0.0f || !std::isfinite(kd) || kd <= 0.0f) return false;
        LESO3 observer = observer_;
        if (!observer.SetParameters(b0, omega_o)) return false;
        b0_ = b0; kp_ = kp; kd_ = kd; observer_ = observer;
        return true;
    }

    bool LADRC2::SetParameters(const LESO3Config& config, float omega_c)
    {
        const float kp = omega_c * omega_c, kd = 2.0f * omega_c;
        if (kp_ <= 0.0f || !std::isfinite(omega_c) || omega_c <= 0.0f ||
            !std::isfinite(kp) || kp <= 0.0f || !std::isfinite(kd) || kd <= 0.0f) return false;
        LESO3 observer = observer_;
        if (!observer.SetParameters(config)) return false;
        b0_ = config.b0; kp_ = kp; kd_ = kd; observer_ = observer;
        return true;
    }

    bool LADRC2::SetDisturbanceLimit(float limit)
    {
        return observer_.SetDisturbanceLimit(limit);
    }

    bool LADRC2::Reset(float output, float rate, float disturbance)
    {
        return observer_.Reset(output, rate, disturbance);
    }

    bool LADRC2::Calc(float target, float& command) const
    {
        if (kp_ <= 0.0f)
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

    bool LADRC2::Calc(float target, float target_rate, float& command) const
    {
        if (kp_ <= 0.0f)
            return false;
        const float desired_acceleration = kp_ * (target - observer_.GetOutput())
                                         + kd_ * (target_rate - observer_.GetRate());
        const float next_command = (desired_acceleration - observer_.GetDisturbance()) / b0_;
        if (!std::isfinite(next_command))
            return false;
        command = next_command;
        return true;
    }
} // namespace ADRC
