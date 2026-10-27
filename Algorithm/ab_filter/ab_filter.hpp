#ifndef __AB_FILTER_HPP
#define __AB_FILTER_HPP

namespace alg_n
{
    /**
     * @brief 标准 AB 滤波器
     *
     * @note 适合对“位置/角度/高度/电流”等带一阶变化趋势的标量进行平滑。
     *       Predict 和 Correct 分离，便于在不同传感器频率下手动安排预测与修正。
     */
    class ABFilter_c
    {
    public:
        ABFilter_c(float alpha = 0.6f, float beta = 0.2f);

        /**
         * @brief 初始化滤波器参数与初始状态
         * @param alpha 位置修正系数，建议范围 0~1，越大越相信测量值
         * @param beta  速度修正系数，建议范围 0~1，越大变化率跟随越快
         * @param value 初始滤波值
         * @param rate  初始变化率
         */
        void Init(float alpha, float beta, float value = 0.0f, float rate = 0.0f);

        /**
         * @brief 修改 AB 增益，不清空当前滤波状态
         * @param alpha 位置修正系数
         * @param beta  速度修正系数
         */
        void SetGain(float alpha, float beta);

        /**
         * @brief 重置滤波状态，不改变 AB 增益
         * @param value 重置后的滤波值
         * @param rate  重置后的变化率
         */
        void Reset(float value = 0.0f, float rate = 0.0f);

        /**
         * @brief 预测步骤，仅使用上一状态和变化率外推
         * @param dt 控制周期，单位由调用者保持一致，必须大于 0
         * @return 预测后的滤波值
         */
        float Predict(float dt);

        /**
         * @brief 修正步骤，使用新的测量值修正预测状态
         * @param measure 当前测量值
         * @return 修正后的滤波值
         */
        float Correct(float measure);

        float GetValue() const;
        float GetRate() const;
        float GetResidual() const;

    private:
        static float Clamp01(float value);
        static float CheckDt(float dt);

        float alpha_;
        float beta_;
        float value_;
        float rate_;
        float residual_;
        float last_dt_;
    };

    /**
     * @brief 融合外部变化率的 AB 滤波器
     *
     * @note 外部变化率只在 Predict 阶段参与预测，Correct 阶段仍只接收测量值。
     *       例如角度测量来自编码器，变化率来自陀螺仪，则 Predict(gyro_rate, dt)。
     */
    class ABRateFusionFilter_c
    {
    public:
        ABRateFusionFilter_c(float alpha = 0.6f, float beta = 0.2f, float rate_weight = 0.8f);

        /**
         * @brief 初始化滤波器参数与初始状态
         * @param alpha       位置修正系数，建议范围 0~1
         * @param beta        速度修正系数，建议范围 0~1
         * @param rate_weight 外部变化率融合权重，0 表示只用内部估计，1 表示只用外部变化率预测
         * @param value       初始滤波值
         * @param rate        初始变化率
         */
        void Init(float alpha, float beta, float rate_weight, float value = 0.0f, float rate = 0.0f);

        /**
         * @brief 修改滤波与融合参数，不清空当前状态
         * @param alpha       位置修正系数
         * @param beta        速度修正系数
         * @param rate_weight 外部变化率融合权重
         */
        void SetGain(float alpha, float beta, float rate_weight);

        /**
         * @brief 重置滤波状态，不改变当前参数
         * @param value 重置后的滤波值
         * @param rate  重置后的变化率
         */
        void Reset(float value = 0.0f, float rate = 0.0f);

        /**
         * @brief 预测步骤，融合外部变化率后外推状态
         * @param input_rate 外部输入变化率
         * @param dt         控制周期，单位由调用者保持一致，必须大于 0
         * @return 预测后的滤波值
         */
        float Predict(float input_rate, float dt);

        /**
         * @brief 修正步骤，使用新的测量值修正预测状态
         * @param measure 当前测量值
         * @return 修正后的滤波值
         */
        float Correct(float measure);

        float GetValue() const;
        float GetRate() const;
        float GetResidual() const;

    private:
        static float Clamp01(float value);
        static float CheckDt(float dt);

        float alpha_;
        float beta_;
        float rate_weight_;
        float value_;
        float rate_;
        float residual_;
        float last_dt_;
    };
}

#endif //! __AB_FILTER_HPP
