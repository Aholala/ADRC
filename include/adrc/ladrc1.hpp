#ifndef ADRC_LADRC1_HPP
#define ADRC_LADRC1_HPP

#include "adrc/leso2.hpp"

namespace adrc
{
    /**
     * @brief 一阶对象的最小 LADRC：线性反馈 + 扰动补偿 + 二阶全阶 ESO。
     * @note 沿用前向欧拉预测式 ESO，Calc 使用 z[k]，UpdateObserver 推进到 z[k+1]。
     *       输入限幅与执行器处理由调用方负责，观测器接收处理后的实际模型输入。
     */
    class LADRC1
    {
    public:
        LADRC1() = default;

        /**
         * @param b0 有限非零的对象输入增益估计，允许为负。
         * @param omega_c 有限正数，反馈增益/理想闭环频率参数，单位 rad/s。
         * @param omega_o 有限正数，ESO 频率参数，单位 rad/s。
         * @param output 初始输出估计 z1。
         * @param disturbance 初始总扰动估计 z2。
         * @return 成功返回 true，无效参数不改变原配置和状态。
         */
        bool Init(float b0, float omega_c, float omega_o, float output = 0.0f,
                  float disturbance = 0.0f);

        /** @brief 重设 ESO 状态，保留参数。成功返回 true。 */
        bool Reset(float output = 0.0f, float disturbance = 0.0f);

        /**
         * @brief 按 z[k] 计算未限幅的 u[k]，不推进观测器。
         * @return 未初始化、目标非有限或数值溢出时返回 false，不改变 command。
         */
        bool Calc(float target, float& command) const;

        /**
         * @brief 用实际的 u[k] 和 y_meas[k]，由 z[k] 预测 z[k+1]。
         * @note 每区间推进一次。输入、时间标签、dt 的约束与 LESO2 相同。
         *       ESO 的步长检查不是整个闭环在任意对象上的稳定性保证。
         */
        bool UpdateObserver(float applied_input, float measurement, float dt);

        float GetEstimatedOutput() const { return observer_.GetOutput(); }
        float GetEstimatedDisturbance() const { return observer_.GetDisturbance(); }

    private:
        float b0_ = 0.0f;
        float omega_c_ = 0.0f;
        LESO2 observer_;
    };
}

#endif
