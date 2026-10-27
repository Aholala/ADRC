/*************************** Dongguan-University of Technology -ACE**************************
 * @file    tof_fusion.cpp
 * @author  OrangeHan
 * @version v26.1.0.0
 * @date    2026-04-26
 * @brief   Two TFmini Plus fusion algorithm
 ************************** Dongguan-University of Technology -ACE***************************/
#include "tof_fusion.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <string.h>

#ifdef __cplusplus
}
#endif

namespace Tof_n
{

static constexpr float TOF_FUSION_RAD_TO_DEG = 57.295779513f;
static constexpr float TOF_FUSION_DEFAULT_DT_S = 0.001f;

void TofFusion_c::Init(TFminiPlus_c *left_tof, TFminiPlus_c *right_tof, float sensor_spacing_cm)
{
    left_tof_ = left_tof;
    right_tof_ = right_tof;
    SetSensorSpacing(sensor_spacing_cm);
    Clear();
    data_.sensor_spacing_cm = sensor_spacing_cm_;
}

void TofFusion_c::Init(UART_HandleTypeDef *left_handle,
                       UART_HandleTypeDef *right_handle,
                       float sensor_spacing_cm,
                       const char *left_name,
                       const char *right_name)
{
    left_tof_owned_.TFminiPlusInit(left_handle, left_name);
    right_tof_owned_.TFminiPlusInit(right_handle, right_name);
    Init(&left_tof_owned_, &right_tof_owned_, sensor_spacing_cm);
}

void TofFusion_c::SetSensors(TFminiPlus_c *left_tof, TFminiPlus_c *right_tof)
{
    left_tof_ = left_tof;
    right_tof_ = right_tof;
}

void TofFusion_c::SetSensorSpacing(float sensor_spacing_cm)
{
    sensor_spacing_cm_ = (sensor_spacing_cm > 0.0f) ? sensor_spacing_cm : 0.0f;
    data_.sensor_spacing_cm = sensor_spacing_cm_;
}

void TofFusion_c::SetBodyVelocity(float body_vx_mps, float body_vy_mps, float body_wz_radps)
{
    body_vx_mps_ = body_vx_mps;
    body_vy_mps_ = body_vy_mps;
    body_wz_radps_ = body_wz_radps;
}

void TofFusion_c::SetBodyPitchRad(float body_pitch_rad)
{
    body_pitch_rad_ = body_pitch_rad;
}

void TofFusion_c::SetABFilterGain(float alpha, float beta, float rate_weight)
{
    ab_alpha_ = Clamp01(alpha);
    ab_beta_ = Clamp01(beta);
    ab_rate_weight_ = Clamp01(rate_weight);

    // 修改参数不清空当前状态，便于调参时平滑过渡。
    center_filter_.SetGain(ab_alpha_, ab_beta_, ab_rate_weight_);
    angle_filter_.SetGain(ab_alpha_, ab_beta_, ab_rate_weight_);
}

void TofFusion_c::SetDistanceLowPassAlpha(float alpha)
{
    // 兼容旧接口：融合层已剔除低通滤波，此处将 alpha 映射为 AB 的测量修正系数。
    SetABFilterGain(alpha, ab_beta_, ab_rate_weight_);
}

void TofFusion_c::SetObstacleGroundVelocityFilterTau(float tau_s)
{
    obstacle_ground_vx_filter_tau_s_ = (tau_s > 0.0f) ? tau_s : 0.0f;
}

void TofFusion_c::Clear()
{
    memset(&data_, 0, sizeof(data_));
    data_.sensor_spacing_cm = sensor_spacing_cm_;
    filter_started_ = 0U;
    obstacle_ground_vx_filter_started_ = 0U;
    obstacle_ground_vx_filtered_mps_ = 0.0f;
    last_dt_s_ = TOF_FUSION_DEFAULT_DT_S;
    last_left_update_count_ = 0U;
    last_right_update_count_ = 0U;
    center_filter_.Init(ab_alpha_, ab_beta_, ab_rate_weight_);
    angle_filter_.Init(ab_alpha_, ab_beta_, ab_rate_weight_);
}

bool TofFusion_c::Update()
{
    return Update(body_vx_mps_, body_vy_mps_, body_wz_radps_, last_dt_s_);
}

bool TofFusion_c::Update(float body_vx_mps, float body_vy_mps, float body_wz_radps, float dt_s)
{
    return Update(body_vx_mps, body_vy_mps, body_wz_radps, body_pitch_rad_, dt_s);
}

bool TofFusion_c::Update(float body_vx_mps, float body_vy_mps, float body_wz_radps, float body_pitch_rad, float dt_s)
{
    SetBodyVelocity(body_vx_mps, body_vy_mps, body_wz_radps);
    SetBodyPitchRad(body_pitch_rad);
    last_dt_s_ = CheckDt(dt_s);

    bool left_online = false;
    bool right_online = false;
    bool left_valid = false;
    bool right_valid = false;
    UpdateSensorState(left_online, right_online, left_valid, right_valid);

    const uint32_t left_update_count = (left_tof_ != nullptr) ? left_tof_->data_.update_count : 0U;
    const uint32_t right_update_count = (right_tof_ != nullptr) ? right_tof_->data_.update_count : 0U;
    const bool left_new = left_online && (left_update_count != last_left_update_count_);
    const bool right_new = right_online && (right_update_count != last_right_update_count_);

    data_.sensor_spacing_cm = sensor_spacing_cm_;
    data_.corrected = 0U;
    data_.predicted = 0U;

    PredictByVelocity(last_dt_s_);

    const bool has_new_valid_measure = (left_new && left_valid) || (right_new && right_valid);
    if (has_new_valid_measure)
    {
        CorrectByTofFrame(left_new, right_new);
    }

    last_left_update_count_ = left_update_count;
    last_right_update_count_ = right_update_count;

    if (!left_valid && !right_valid)
    {
        data_.valid = 0U;
        data_.degraded = 0U;
        data_.raw_left_distance_cm = 0.0f;
        data_.raw_right_distance_cm = 0.0f;
        data_.raw_center_distance_cm = 0.0f;
        data_.pitch_corrected_center_distance_cm = 0.0f;
        data_.raw_distance_delta_cm = 0.0f;
        data_.raw_thickness_cm = 0.0f;
        data_.raw_obstacle_angle_rad = 0.0f;
        data_.raw_obstacle_angle_deg = 0.0f;
        data_.raw_obstacle_signed_angle_rad = 0.0f;
        data_.raw_obstacle_signed_angle_deg = 0.0f;
        return false;
    }

    if (filter_started_ != 0U)
    {
        data_.valid = 1U;
        return true;
    }

    data_.valid = 0U;
    return false;
}

void TofFusion_c::PredictByVelocity(float dt_s)
{
    if (filter_started_ == 0U)
    {
        return;
    }

    // 默认模型：TOF 朝机体 x 正方向测距。机体向前靠近障碍物时，中心距离减小。
    const float center_rate_cmps = -body_vx_mps_ * 100.0f;

    // 默认模型：机体正向自转会让障碍物相对角度反向变化。
    const float angle_rate_radps = -body_wz_radps_;

    center_filter_.Predict(center_rate_cmps, CheckDt(dt_s));
    angle_filter_.Predict(angle_rate_radps, CheckDt(dt_s));
    data_.predicted = 1U;
    UpdateOutputFromFilter();
}

bool TofFusion_c::CorrectByTofFrame(bool left_new, bool right_new)
{
    (void)left_new;
    (void)right_new;

    const bool left_valid = data_.left_valid != 0U;
    const bool right_valid = data_.right_valid != 0U;

    if (!left_valid && !right_valid)
    {
        return false;
    }

    float left_distance = left_valid ? static_cast<float>(left_tof_->GetRawDistanceCm()) : 0.0f;
    float right_distance = right_valid ? static_cast<float>(right_tof_->GetRawDistanceCm()) : 0.0f;
    bool angle_measure_valid = true;

    if (left_valid && !right_valid)
    {
        right_distance = left_distance;
        angle_measure_valid = false;
        data_.degraded = 1U;
    }
    else if (!left_valid && right_valid)
    {
        left_distance = right_distance;
        angle_measure_valid = false;
        data_.degraded = 1U;
    }
    else
    {
        data_.degraded = 0U;
    }

    data_.raw_left_distance_cm = left_distance;
    data_.raw_right_distance_cm = right_distance;
    data_.raw_center_distance_cm = (left_distance + right_distance) * 0.5f;
    data_.pitch_corrected_center_distance_cm = CorrectCenterDistanceByPitch(data_.raw_center_distance_cm);
    data_.raw_distance_delta_cm = right_distance - left_distance;
    data_.raw_thickness_cm = fabsf(data_.raw_distance_delta_cm);

    if (sensor_spacing_cm_ > 0.0f && angle_measure_valid)
    {
        data_.raw_obstacle_signed_angle_rad = atan2f(data_.raw_distance_delta_cm, sensor_spacing_cm_);
        data_.raw_obstacle_signed_angle_deg = data_.raw_obstacle_signed_angle_rad * TOF_FUSION_RAD_TO_DEG;
        data_.raw_obstacle_angle_rad = fabsf(data_.raw_obstacle_signed_angle_rad);
        data_.raw_obstacle_angle_deg = fabsf(data_.raw_obstacle_signed_angle_deg);
    }
    else
    {
        data_.raw_obstacle_angle_rad = 0.0f;
        data_.raw_obstacle_angle_deg = 0.0f;
        data_.raw_obstacle_signed_angle_rad = 0.0f;
        data_.raw_obstacle_signed_angle_deg = 0.0f;
    }

    if (filter_started_ == 0U)
    {
        const float center_rate_cmps = -body_vx_mps_ * 100.0f;
        const float angle_rate_radps = -body_wz_radps_;
        center_filter_.Reset(data_.pitch_corrected_center_distance_cm, center_rate_cmps);
        angle_filter_.Reset(data_.raw_obstacle_signed_angle_rad, angle_rate_radps);
        filter_started_ = 1U;
    }
    else
    {
        center_filter_.Correct(data_.pitch_corrected_center_distance_cm);
        if (angle_measure_valid)
        {
            angle_filter_.Correct(data_.raw_obstacle_signed_angle_rad);
        }
    }

    data_.corrected = 1U;
    data_.valid = 1U;
    UpdateOutputFromFilter();
    return true;
}

void TofFusion_c::UpdateOutputFromFilter()
{
    data_.center_distance_cm = center_filter_.GetValue();
    data_.center_distance_rate_cmps = center_filter_.GetRate();
    data_.obstacle_relative_body_vx_mps = data_.center_distance_rate_cmps * 0.01f;

    const float obstacle_ground_vx_raw_mps = body_vx_mps_ + data_.obstacle_relative_body_vx_mps;
    if (obstacle_ground_vx_filter_started_ == 0U || obstacle_ground_vx_filter_tau_s_ <= 0.0f)
    {
        obstacle_ground_vx_filtered_mps_ = obstacle_ground_vx_raw_mps;
        obstacle_ground_vx_filter_started_ = 1U;
    }
    else
    {
        const float dt_s = CheckDt(last_dt_s_);
        const float alpha = dt_s / (obstacle_ground_vx_filter_tau_s_ + dt_s);
        obstacle_ground_vx_filtered_mps_ += alpha * (obstacle_ground_vx_raw_mps - obstacle_ground_vx_filtered_mps_);
    }
    data_.obstacle_ground_vx_mps = obstacle_ground_vx_filtered_mps_;

    data_.obstacle_signed_angle_rad = angle_filter_.GetValue();
    data_.obstacle_signed_angle_rate_radps = angle_filter_.GetRate();
    data_.obstacle_signed_angle_deg = data_.obstacle_signed_angle_rad * TOF_FUSION_RAD_TO_DEG;
    data_.obstacle_angle_rad = fabsf(data_.obstacle_signed_angle_rad);
    data_.obstacle_angle_deg = fabsf(data_.obstacle_signed_angle_deg);

    if (sensor_spacing_cm_ > 0.0f)
    {
        data_.distance_delta_cm = tanf(data_.obstacle_signed_angle_rad) * sensor_spacing_cm_;
    }
    else
    {
        data_.distance_delta_cm = 0.0f;
    }

    data_.thickness_cm = fabsf(data_.distance_delta_cm);
    data_.left_distance_cm = data_.center_distance_cm - data_.distance_delta_cm * 0.5f;
    data_.right_distance_cm = data_.center_distance_cm + data_.distance_delta_cm * 0.5f;
}

void TofFusion_c::UpdateSensorState(bool &left_online, bool &right_online, bool &left_valid, bool &right_valid)
{
    left_online = (left_tof_ != nullptr) && left_tof_->IsOnline();
    right_online = (right_tof_ != nullptr) && right_tof_->IsOnline();
    left_valid = left_online && left_tof_->IsDistanceValid();
    right_valid = right_online && right_tof_->IsDistanceValid();

    data_.left_online = left_online ? 1U : 0U;
    data_.right_online = right_online ? 1U : 0U;
    data_.left_valid = left_valid ? 1U : 0U;
    data_.right_valid = right_valid ? 1U : 0U;
}

float TofFusion_c::CorrectCenterDistanceByPitch(float raw_center_distance_cm) const
{
    // 只修正测量距离：将 TOF 射线距离投影到机体 pitch 为 0 时的前向距离。
    float pitch_scale = cosf(body_pitch_rad_);
    if (pitch_scale < 0.0f)
    {
        pitch_scale = 0.0f;
    }
    return raw_center_distance_cm * pitch_scale;
}

float TofFusion_c::Clamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

float TofFusion_c::CheckDt(float dt_s)
{
    if (dt_s > 0.0f)
    {
        return dt_s;
    }
    return TOF_FUSION_DEFAULT_DT_S;
}

} // namespace Tof_n
