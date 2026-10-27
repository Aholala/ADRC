#ifndef TOF_FUSION_HPP
#define TOF_FUSION_HPP

#include "ab_filter.hpp"
#include "tfmini_plus.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef __cplusplus
}

namespace Tof_n
{

typedef struct
{
    float sensor_spacing_cm;                  // 左右 TOF 安装间距，单位 cm

    float raw_left_distance_cm;               // 左 TOF 原始距离，单位 cm
    float raw_right_distance_cm;              // 右 TOF 原始距离，单位 cm
    float raw_center_distance_cm;             // 原始中心距离，单位 cm
    float pitch_corrected_center_distance_cm; // 经过机体 pitch 投影修正后的中心测量距离，单位 cm
    float raw_distance_delta_cm;              // 原始左右距离差，right - left，单位 cm
    float raw_thickness_cm;                   // 原始距离差绝对值，单位 cm
    float raw_obstacle_angle_rad;             // 原始障碍物夹角绝对值，单位 rad
    float raw_obstacle_angle_deg;             // 原始障碍物夹角绝对值，单位 deg
    float raw_obstacle_signed_angle_rad;      // 原始障碍物有符号夹角，单位 rad
    float raw_obstacle_signed_angle_deg;      // 原始障碍物有符号夹角，单位 deg

    float left_distance_cm;                   // 由中心距离与角度反算的左侧距离估计，单位 cm
    float right_distance_cm;                  // 由中心距离与角度反算的右侧距离估计，单位 cm
    float center_distance_cm;                 // AB 滤波后的中心距离，单位 cm
    float center_distance_rate_cmps;          // AB 估计的中心距离变化率，单位 cm/s
    float obstacle_relative_body_vx_mps;      // TOF 估计的障碍物相对机体 x 方向速度，单位 m/s，x 正方向为 TOF 朝向
    float obstacle_ground_vx_mps;             // 障碍物相对大地 x 方向速度，单位 m/s，x 正方向为 TOF 朝向

    float distance_delta_cm;                  // AB 滤波后的左右距离差，单位 cm
    float thickness_cm;                       // AB 滤波后的距离差绝对值，单位 cm

    float obstacle_angle_rad;                 // AB 滤波后的障碍物夹角绝对值，单位 rad
    float obstacle_angle_deg;                 // AB 滤波后的障碍物夹角绝对值，单位 deg
    float obstacle_signed_angle_rad;          // AB 滤波后的障碍物有符号夹角，单位 rad
    float obstacle_signed_angle_deg;          // AB 滤波后的障碍物有符号夹角，单位 deg
    float obstacle_signed_angle_rate_radps;   // AB 估计的有符号夹角变化率，单位 rad/s

    uint8_t valid;                            // 融合输出是否有效
    uint8_t degraded;                         // 是否处于单 TOF 降级状态
    uint8_t corrected;                        // 本次 Update 是否使用新 TOF 测量完成修正
    uint8_t predicted;                        // 本次 Update 是否执行了 AB 预测
    uint8_t left_online;                      // 左 TOF 是否在线
    uint8_t right_online;                     // 右 TOF 是否在线
    uint8_t left_valid;                       // 左 TOF 当前距离是否有效
    uint8_t right_valid;                      // 右 TOF 当前距离是否有效
} TofFusionData_t;

class TofFusion_c
{
public:
    TofFusionData_t data_ = {0};

    TofFusion_c() = default;
    TofFusion_c(TFminiPlus_c *left_tof, TFminiPlus_c *right_tof, float sensor_spacing_cm)
    {
        Init(left_tof, right_tof, sensor_spacing_cm);
    }
    TofFusion_c(UART_HandleTypeDef *left_handle,
                UART_HandleTypeDef *right_handle,
                float sensor_spacing_cm,
                const char *left_name = "TOF_LEFT",
                const char *right_name = "TOF_RIGHT")
    {
        Init(left_handle, right_handle, sensor_spacing_cm, left_name, right_name);
    }

    void Init(TFminiPlus_c *left_tof, TFminiPlus_c *right_tof, float sensor_spacing_cm);
    void Init(UART_HandleTypeDef *left_handle,
              UART_HandleTypeDef *right_handle,
              float sensor_spacing_cm,
              const char *left_name = "TOF_LEFT",
              const char *right_name = "TOF_RIGHT");
    void SetSensors(TFminiPlus_c *left_tof, TFminiPlus_c *right_tof);
    void SetSensorSpacing(float sensor_spacing_cm);

    /**
     * @brief 设置机体速度，供高频 AB 预测使用
     * @param body_vx_mps 机体 x 方向线速度，单位 m/s，默认约定 x 正方向为 TOF 朝向
     * @param body_vy_mps 机体 y 方向线速度，单位 m/s，当前默认模型仅保存，预留给安装几何扩展
     * @param body_wz_radps 机体 z 轴角速度，单位 rad/s
     */
    void SetBodyVelocity(float body_vx_mps, float body_vy_mps, float body_wz_radps);

    /**
     * @brief 设置机体 pitch 角，用于修正 TOF 中心测量距离
     * @param body_pitch_rad 机体 pitch 角，单位 rad；只修正距离测量，不修正速度预测
     */
    void SetBodyPitchRad(float body_pitch_rad);

    /**
     * @brief 设置中心距离与角度 AB 滤波参数
     * @param alpha 测量修正系数，范围 0~1，越大越相信 TOF 测量
     * @param beta 变化率修正系数，范围 0~1，越大变化率跟随越快
     * @param rate_weight 外部速度融合权重，范围 0~1，越大越相信外部速度
     */
    void SetABFilterGain(float alpha, float beta, float rate_weight);

    /**
     * @brief 兼容旧低通接口，内部映射为 AB 滤波 alpha 参数
     * @param alpha 测量修正系数，范围 0~1
     */
    void SetDistanceLowPassAlpha(float alpha);

    /**
     * @brief 设置障碍物对地速度一阶低通时间常数
     * @param tau_s 时间常数，单位 s；小于等于 0 时关闭低通
     */
    void SetObstacleGroundVelocityFilterTau(float tau_s);

    /**
     * @brief 高频更新接口：先使用外部速度预测，有新 TOF 帧时再修正
     * @param body_vx_mps 机体 x 方向线速度，单位 m/s
     * @param body_vy_mps 机体 y 方向线速度，单位 m/s
     * @param body_wz_radps 机体 z 轴角速度，单位 rad/s
     * @param dt_s 高频调用周期，单位 s
     * @return 当前融合输出是否有效
     */
    bool Update(float body_vx_mps, float body_vy_mps, float body_wz_radps, float dt_s);

    /**
     * @brief 高频更新接口：带机体 pitch 距离修正，速度预测不做 pitch 修正
     * @param body_vx_mps 机体 x 方向线速度，单位 m/s
     * @param body_vy_mps 机体 y 方向线速度，单位 m/s
     * @param body_wz_radps 机体 z 轴角速度，单位 rad/s
     * @param body_pitch_rad 机体 pitch 角，单位 rad，用于修正中心距离测量值
     * @param dt_s 高频调用周期，单位 s
     * @return 当前融合输出是否有效
     */
    bool Update(float body_vx_mps, float body_vy_mps, float body_wz_radps, float body_pitch_rad, float dt_s);

    /**
     * @brief 兼容旧更新接口，使用最近一次 SetBodyVelocity 保存的速度执行预测
     * @return 当前融合输出是否有效
     */
    bool Update();

    /**
     * @brief 清空融合数据与 AB 滤波状态
     */
    void Clear();

    bool IsValid() const { return data_.valid != 0U; }
    bool IsDegraded() const { return data_.degraded != 0U; }

    float GetLeftDistanceCm() const { return data_.left_distance_cm; }
    float GetRightDistanceCm() const { return data_.right_distance_cm; }
    float GetCenterDistanceCm() const { return data_.center_distance_cm; }
    float GetCenterDistanceRateCmps() const { return data_.center_distance_rate_cmps; }
    float GetObstacleRelativeBodyVxMps() const { return data_.obstacle_relative_body_vx_mps; }
    float GetObstacleGroundVxMps() const { return data_.obstacle_ground_vx_mps; }
    float GetRawLeftDistanceCm() const { return data_.raw_left_distance_cm; }
    float GetRawRightDistanceCm() const { return data_.raw_right_distance_cm; }
    float GetRawCenterDistanceCm() const { return data_.raw_center_distance_cm; }
    float GetPitchCorrectedCenterDistanceCm() const { return data_.pitch_corrected_center_distance_cm; }
    float GetDistanceDeltaCm() const { return data_.distance_delta_cm; }
    float GetThicknessCm() const { return data_.thickness_cm; }
    float GetRawDistanceDeltaCm() const { return data_.raw_distance_delta_cm; }
    float GetRawThicknessCm() const { return data_.raw_thickness_cm; }
    float GetObstacleAngleRad() const { return data_.obstacle_angle_rad; }
    float GetObstacleAngleDeg() const { return data_.obstacle_angle_deg; }
    float GetObstacleSignedAngleRad() const { return data_.obstacle_signed_angle_rad; }
    float GetObstacleSignedAngleDeg() const { return data_.obstacle_signed_angle_deg; }
    float GetObstacleSignedAngleRateRadps() const { return data_.obstacle_signed_angle_rate_radps; }
    float GetRawObstacleAngleRad() const { return data_.raw_obstacle_angle_rad; }
    float GetRawObstacleAngleDeg() const { return data_.raw_obstacle_angle_deg; }
    float GetRawObstacleSignedAngleRad() const { return data_.raw_obstacle_signed_angle_rad; }
    float GetRawObstacleSignedAngleDeg() const { return data_.raw_obstacle_signed_angle_deg; }

    TFminiPlus_c *GetLeftTof() { return left_tof_; }
    TFminiPlus_c *GetRightTof() { return right_tof_; }
    const TFminiPlus_c *GetLeftTof() const { return left_tof_; }
    const TFminiPlus_c *GetRightTof() const { return right_tof_; }

private:
    TFminiPlus_c left_tof_owned_;                  // 内部持有的左 TOF 对象
    TFminiPlus_c right_tof_owned_;                 // 内部持有的右 TOF 对象
    TFminiPlus_c *left_tof_ = nullptr;             // 左 TOF 对象指针
    TFminiPlus_c *right_tof_ = nullptr;            // 右 TOF 对象指针
    float sensor_spacing_cm_ = 0.0f;               // 左右 TOF 安装间距，单位 cm

    alg_n::ABRateFusionFilter_c center_filter_;    // 中心距离 AB 滤波器
    alg_n::ABRateFusionFilter_c angle_filter_;     // 有符号角度 AB 滤波器
    float ab_alpha_ = 0.55f;                       // AB 滤波 alpha 参数
    float ab_beta_ = 0.12f;                        // AB 滤波 beta 参数
    float ab_rate_weight_ = 0.75f;                 // 外部速度融合权重
    uint8_t filter_started_ = 0U;                  // AB 滤波器是否已有初值
    uint8_t obstacle_ground_vx_filter_started_ = 0U; // 障碍物对地速度低通是否已有初值
    float obstacle_ground_vx_filtered_mps_ = 0.0f; // 低通后的障碍物对地 x 方向速度，单位 m/s
    float obstacle_ground_vx_filter_tau_s_ = 0.08f; // 障碍物对地速度一阶低通时间常数，单位 s

    float body_vx_mps_ = 0.0f;                     // 机体 x 方向线速度，单位 m/s
    float body_vy_mps_ = 0.0f;                     // 机体 y 方向线速度，单位 m/s
    float body_wz_radps_ = 0.0f;                   // 机体 z 轴角速度，单位 rad/s
    float body_pitch_rad_ = 0.0f;                  // 机体 pitch 角，单位 rad，仅用于距离测量修正
    float last_dt_s_ = 0.001f;                     // 上次预测周期，单位 s

    uint32_t last_left_update_count_ = 0U;         // 上次已处理的左 TOF 帧计数
    uint32_t last_right_update_count_ = 0U;        // 上次已处理的右 TOF 帧计数

    void PredictByVelocity(float dt_s);
    bool CorrectByTofFrame(bool left_new, bool right_new);
    void UpdateOutputFromFilter();
    void UpdateSensorState(bool &left_online, bool &right_online, bool &left_valid, bool &right_valid);
    float CorrectCenterDistanceByPitch(float raw_center_distance_cm) const;
    static float Clamp01(float value);
    static float CheckDt(float dt_s);
};

} // namespace Tof_n

#endif

#endif // TOF_FUSION_HPP
