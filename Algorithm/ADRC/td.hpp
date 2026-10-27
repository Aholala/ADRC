#ifndef ECF_ADRC_TD_HPP
#define ECF_ADRC_TD_HPP

namespace alg_n
{
    /**
     * @brief 线性二阶跟踪微分器，输出平滑值和它的一阶变化率。
     * @note 对应 ADRC1/MyLibrary/TD2_L.m 的前向欧拉更新。
     *       不依赖 HAL、RTOS，不分配内存；每个对象独立保存状态。
     */
    class LinearTD_c
    {
    public:
        LinearTD_c() = default;

        /**
         * @param bandwidth 频率参数 omega，单位 rad/s，必须为有限正数。
         * @param value 初始平滑值，与输入同单位。
         * @param rate 初始变化率，单位为输入单位/s。
         * @return 配置成功；无效参数不改变已有配置或状态。
         */
        bool Init(float bandwidth, float value = 0.0f, float rate = 0.0f);

        /** @brief 重置状态，保留频率参数；非有限初值返回 false，状态不变。 */
        bool Reset(float value = 0.0f, float rate = 0.0f);

        /**
         * @brief 更新一个采样周期，成功后通过 GetValue/GetRate 读取新状态。
         * @param input 当前输入，必须为有限数。
         * @param dt 实际采样周期，单位 s；必须满足 0 < bandwidth * dt < 2。
         * @return 成功返回 true。未初始化、参数无效、离散不稳定或数值
         *         溢出时返回 false，保留上一状态，由调用方决定如何处理。
         */
        bool Calc(float input, float dt);

        float GetValue() const { return value_; }
        float GetRate() const { return rate_; }

    private:
        float bandwidth_ = 0.0f;
        float value_ = 0.0f;
        float rate_ = 0.0f;
    };
}

#endif
