#ifndef ADRC_RLESO1_HPP
#define ADRC_RLESO1_HPP

namespace adrc
{
    /**
     * @brief 一阶对象 y_dot=b0*u+f 的一阶降阶线性ESO。
     * @note 只积分q；当前扰动估计=q+omega_o*当前测量。
     *       无HAL、RTOS、动态内存或时钟依赖；实例应串行访问。
     */
    class RLESO1
    {
    public:
        RLESO1() = default;

        /**
         * @param b0 有限非零输入增益，允许负值。
         * @param omega_o 固定正带宽参数，单位1/s。
         * @param measurement 初始测量；据此设置q=disturbance-omega_o*measurement。
         * @param disturbance 期望初始扰动估计，单位为测量单位/s。
         * @return 参数与初态可表示时成功，失败保持原配置与状态。
         */
        bool Init(float b0, float omega_o, float measurement = 0.0f,
                  float disturbance = 0.0f);

        /** @brief 已初始化后重设q，保留配置；失败保持状态。 */
        bool Reset(float measurement = 0.0f, float disturbance = 0.0f);

        /**
         * @brief 使用当前测量重构本拍扰动估计，不推进q。
         * @return 未初始化、输入无效或溢出时返回false，并保持输出引用不变。
         * @note 有限精度下重构含大数相消，不保证与初始化期望值逐位相等。
         */
        bool Calc(float measurement, float& disturbance) const;

        /**
         * @brief 用区间实际输入u[k]与区间开始测量y[k]预测q[k+1]。
         * @param dt 区间长度/s，要求0<omega_o*dt<2；每区间只更新一次。
         * @return 无效输入、周期或数值溢出时返回false，q保持不变。
         * @note 更新后还需要下一拍测量才能计算下一拍完整扰动估计。
         *       理想离散极点范围不代表任意闭环工况或周期抖动保证。
         */
        bool Update(float input, float measurement, float dt);

        /** @brief 辅助状态q，供诊断；它本身不是扰动估计。 */
        float GetAuxiliaryState() const { return q_; }

    private:
        float b0_ = 0.0f;
        float omega_o_ = 0.0f;
        float q_ = 0.0f;
    };
}

#endif
