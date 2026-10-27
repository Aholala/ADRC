/******************************** ********************************
 * @file ab_filter.cpp
 * @author ACE 26赛季电控组
 * @version v26.1.0
 * @date 2026-04-27
 * @brief AB 滤波器实现，包含标准版本与外部变化率融合版本
 *
 * @note AB 滤波适合低成本估计一维状态：value 为滤波值，rate 为一阶变化率。
 *       Predict 负责按模型预测，Correct 负责按测量值修正。
 ******************************** ********************************/

#include "ab_filter.hpp"

namespace alg_n
{
    /******************************** 标准 AB 滤波 ********************************/

    ABFilter_c::ABFilter_c(float alpha, float beta)
            : alpha_(Clamp01(alpha)),
              beta_(Clamp01(beta)),
              value_(0.0f),
              rate_(0.0f),
              residual_(0.0f),
              last_dt_(0.001f)
    {}

    void ABFilter_c::Init(float alpha, float beta, float value, float rate)
    {
        SetGain(alpha, beta);
        Reset(value, rate);
    }

    void ABFilter_c::SetGain(float alpha, float beta)
    {
        alpha_ = Clamp01(alpha);
        beta_ = Clamp01(beta);
    }

    void ABFilter_c::Reset(float value, float rate)
    {
        value_ = value;
        rate_ = rate;
        residual_ = 0.0f;
        last_dt_ = 0.001f;
    }

    float ABFilter_c::Predict(float dt)
    {
        last_dt_ = CheckDt(dt);

        // 匀速模型预测：当前值 = 上一滤波值 + 变化率 * 周期
        value_ += rate_ * last_dt_;
        return value_;
    }

    float ABFilter_c::Correct(float measure)
    {
        // residual 是测量值与预测值的差值，也叫创新量。
        residual_ = measure - value_;
        value_ += alpha_ * residual_;
        rate_ += beta_ * residual_ / last_dt_;

        return value_;
    }

    float ABFilter_c::GetValue() const
    {
        return value_;
    }

    float ABFilter_c::GetRate() const
    {
        return rate_;
    }

    float ABFilter_c::GetResidual() const
    {
        return residual_;
    }

    float ABFilter_c::Clamp01(float value)
    {
        if (value < 0.0f)
            return 0.0f;
        if (value > 1.0f)
            return 1.0f;
        return value;
    }

    float ABFilter_c::CheckDt(float dt)
    {
        if (dt > 0.0f)
            return dt;
        return 0.001f;
    }

    /**************************** 外部变化率融合 AB 滤波 ****************************/

    ABRateFusionFilter_c::ABRateFusionFilter_c(float alpha, float beta, float rate_weight)
            : alpha_(Clamp01(alpha)),
              beta_(Clamp01(beta)),
              rate_weight_(Clamp01(rate_weight)),
              value_(0.0f),
              rate_(0.0f),
              residual_(0.0f),
              last_dt_(0.001f)
    {}

    void ABRateFusionFilter_c::Init(float alpha, float beta, float rate_weight, float value, float rate)
    {
        SetGain(alpha, beta, rate_weight);
        Reset(value, rate);
    }

    void ABRateFusionFilter_c::SetGain(float alpha, float beta, float rate_weight)
    {
        alpha_ = Clamp01(alpha);
        beta_ = Clamp01(beta);
        rate_weight_ = Clamp01(rate_weight);
    }

    void ABRateFusionFilter_c::Reset(float value, float rate)
    {
        value_ = value;
        rate_ = rate;
        residual_ = 0.0f;
        last_dt_ = 0.001f;
    }

    float ABRateFusionFilter_c::Predict(float input_rate, float dt)
    {
        last_dt_ = CheckDt(dt);

        // 变化率融合：外部变化率可信时可减小测量滞后，内部变化率用于保持连续性。
        rate_ = rate_weight_ * input_rate + (1.0f - rate_weight_) * rate_;
        value_ += rate_ * last_dt_;

        return value_;
    }

    float ABRateFusionFilter_c::Correct(float measure)
    {
        residual_ = measure - value_;
        value_ += alpha_ * residual_;
        rate_ += beta_ * residual_ / last_dt_;

        return value_;
    }

    float ABRateFusionFilter_c::GetValue() const
    {
        return value_;
    }

    float ABRateFusionFilter_c::GetRate() const
    {
        return rate_;
    }

    float ABRateFusionFilter_c::GetResidual() const
    {
        return residual_;
    }

    float ABRateFusionFilter_c::Clamp01(float value)
    {
        if (value < 0.0f)
            return 0.0f;
        if (value > 1.0f)
            return 1.0f;
        return value;
    }

    float ABRateFusionFilter_c::CheckDt(float dt)
    {
        if (dt > 0.0f)
            return dt;
        return 0.001f;
    }
}
