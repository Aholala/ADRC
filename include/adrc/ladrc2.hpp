#ifndef ADRC_LADRC2_HPP
#define ADRC_LADRC2_HPP

#include "adrc/leso3.hpp"

namespace adrc
{
    /**
     * @brief 二阶对象的最小LADRC：位置与速度反馈、扰动补偿、三阶LESO。
     * @note Calc使用z[k]，UpdateObserver推进为z[k+1]；输入限幅由调用方处理。
     *       位置给定与零速度基准反馈，不含目标速度/加速度前馈。
     */
    class LADRC2
    {
    public:
        LADRC2() = default;

        /**
         * @param b0 有限非零的模型输入增益，允许为负。
         * @param omega_c 有限正控制带宽参数，内部计算kp=wc²、kd=2wc。
         * @param omega_o 有限正观测器带宽参数，rad/s。
         * @param output 初始输出估计z1。
         * @param rate 初始输出变化率估计z2。
         * @param disturbance 初始总扰动估计z3。
         * @return 成功返回true；失败保留原参数和观测状态。
         */
        bool Init(float b0, float omega_c, float omega_o, float output = 0.0f,
                  float rate = 0.0f, float disturbance = 0.0f);

        /** @brief 只复位三个估计状态；失败时保持状态，参数不变。 */
        bool Reset(float output = 0.0f, float rate = 0.0f, float disturbance = 0.0f);

        /**
         * @brief 用当前z[k]计算未限幅请求u[k]，不推进LESO。
         * @return 未初始化、目标非有限或计算溢出时返回false，不改变command。
         */
        bool Calc(float target, float& command) const;

        /**
         * @brief 用本区间的实际输入与当前测量，预测下一拍状态。
         * @note 每区间调用一次，沿用LESO3的输入、周期和失败保持约定。
         *       对象与闭环的稳定性仍需针对实际参数和调用时序验证。
         */
        bool UpdateObserver(float applied_input, float measurement, float dt);

        float GetEstimatedOutput() const { return observer_.GetOutput(); }
        float GetEstimatedRate() const { return observer_.GetRate(); }
        float GetEstimatedDisturbance() const { return observer_.GetDisturbance(); }

    private:
        float b0_ = 0.0f;
        float kp_ = 0.0f;
        float kd_ = 0.0f;
        LESO3 observer_;
    };
}

#endif
