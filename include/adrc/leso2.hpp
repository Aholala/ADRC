#ifndef ADRC_LESO2_HPP
#define ADRC_LESO2_HPP

namespace adrc
{
    /**
     * @brief 一阶对象 y_dot=b0*u+f 对应的二阶全阶线性 ESO。
     * @note 使用前向欧拉，与 ADRC1/ADRC_1.m 的 ESO 更新结构对应。
     *       无硬件依赖、动态内存和系统时钟读取。每个实例独立保存状态。
     */
    class LESO2
    {
    public:
        LESO2() = default;

        /**
         * @param b0 输入增益估计，必须有限且非零，允许负增益。
         * @param omega_o 观测器带宽参数，rad/s，必须有限且为正。
         * @param output 初始输出估计 z1，与测量同单位。
         * @param disturbance 初始总扰动估计 z2，单位为输出单位/s。
         * @return 成功返回 true；无效参数保留已有配置和状态。
         */
        bool Init(float b0, float omega_o, float output = 0.0f,
                  float disturbance = 0.0f);

        /** @brief 只重置状态；非有限初值返回 false，原状态不变。 */
        bool Reset(float output = 0.0f, float disturbance = 0.0f);

        /**
         * @brief 根据 u[k]、y_meas[k]，由 z[k] 预测 z[k+1]。
         * @param input 对应 [t_k,t_k+dt) 区间的模型输入 u[k]。
         * @param measurement 区间开始时的测量 y_meas[k]。
         * @param dt 预测区间长度，s；本方法要求 0 < omega_o*dt < 2。
         * @return 成功后 getter 是下一拍预测值。未初始化、无效参数、
         *         稳定范围外的步长或数值溢出返回 false，状态保持不变。
         * @note 固定周期下的稳定范围不代表任意工况的性能或闭环稳定保证。
         */
        bool Update(float input, float measurement, float dt);

        float GetOutput() const { return z1_; }
        float GetDisturbance() const { return z2_; }

    private:
        float b0_ = 0.0f;
        float omega_o_ = 0.0f;
        float beta1_ = 0.0f;
        float beta2_ = 0.0f;
        float z1_ = 0.0f;
        float z2_ = 0.0f;
    };
}

#endif
