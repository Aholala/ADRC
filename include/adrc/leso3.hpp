#ifndef ADRC_LESO3_HPP
#define ADRC_LESO3_HPP

namespace adrc
{
    /**
     * @brief 二阶对象 y_ddot=b0*u+f 对应的三阶全阶线性ESO。
     * @note 前向欧拉预测，无HAL、RTOS、动态内存和系统时钟依赖。
     *       z1估计输出，z2估计输出变化率，z3估计总扰动。
     */
    class LESO3
    {
    public:
        LESO3() = default;

        /**
         * @param b0 有限且非零的输入增益，允许负增益。
         * @param omega_o 有限正带宽参数，rad/s；内部计算3w、3w²、w³。
         * @param output 初始输出估计，与输出同单位。
         * @param rate 初始输出变化率估计，单位为输出单位/s。
         * @param disturbance 初始总扰动估计，单位为输出单位/s²。
         * @return 成功返回true；参数无效或增益无法表示时保留原配置和状态。
         */
        bool Init(float b0, float omega_o, float output = 0.0f,
                  float rate = 0.0f, float disturbance = 0.0f);

        /** @brief 仅重设三个状态；非有限初值返回false，原状态不变。 */
        bool Reset(float output = 0.0f, float rate = 0.0f, float disturbance = 0.0f);

        /**
         * @brief 使用u[k]、y_meas[k]，由z[k]预测z[k+1]。
         * @param input 对应本预测区间的实际模型输入，含调用方最终输出处理。
         * @param measurement 区间开始时的输出测量。
         * @param dt 预测区间长度/s，要求0<omega_o*dt<2。
         * @return 成功后getter为下一拍预测；未初始化、无效输入或溢出时保持状态。
         * @note 每个区间推进一次，同一实例由调用方串行访问。
         *       固定步长理想误差系统稳定范围不等于任意工况或闭环稳定保证。
         */
        bool Update(float input, float measurement, float dt);

        float GetOutput() const { return z1_; }
        float GetRate() const { return z2_; }
        float GetDisturbance() const { return z3_; }

    private:
        float b0_ = 0.0f;
        float omega_o_ = 0.0f;
        float beta1_ = 0.0f;
        float beta2_ = 0.0f;
        float beta3_ = 0.0f;
        float z1_ = 0.0f;
        float z2_ = 0.0f;
        float z3_ = 0.0f;
    };
}

#endif
