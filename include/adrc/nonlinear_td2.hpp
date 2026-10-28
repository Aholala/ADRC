#ifndef ADRC_NONLINEAR_TD2_HPP
#define ADRC_NONLINEAR_TD2_HPP

namespace adrc
{
    /**
     * @brief 对应ADRC1/TD2_NL.m与原分段fhan的非线性二阶TD。
     * @note 无HAL、RTOS、动态内存或时钟依赖；同一实例串行访问。
     */
    class NonlinearTD2
    {
    public:
        NonlinearTD2() = default;

        /**
         * @param r 正加速度尺度，单位为输入单位/s²；不是变化率上限。
         * @param h0 固定的fhan内部时间尺度/s，不是实际调用周期。
         * @return 参数、初态及内部尺度可表示时返回true；失败保留配置与状态。
         */
        bool Init(float r, float h0, float output = 0.0f, float rate = 0.0f);

        /** @brief 仅重设两个状态；无效初值返回false并保持原状态。 */
        bool Reset(float output = 0.0f, float rate = 0.0f);

        /**
         * @brief 用当前状态与input[k]计算本拍内部加速度，不推进状态。
         * @return 未初始化、输入无效或中间量溢出时返回false，输出引用保持不变。
         */
        bool Calc(float input, float& acceleration) const;

        /**
         * @brief 两条前向欧拉更新均使用旧状态，每区间推进一次。
         * @param dt 实际区间长度/s，要求0<dt/h0<2。
         * @return 无效输入、周期或数值溢出时返回false，两个状态保持不变。
         * @note 周期限制来自小误差分支的局部线性化，不保证全局非线性收敛或实机性能。
         */
        bool Update(float input, float dt);

        float GetOutput() const { return x1_; }
        float GetRate() const { return x2_; }

    private:
        float r_ = 0.0f;
        float h0_ = 0.0f;
        float x1_ = 0.0f;
        float x2_ = 0.0f;
    };
}

#endif
