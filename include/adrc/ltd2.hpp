#ifndef ADRC_LTD2_HPP
#define ADRC_LTD2_HPP

namespace adrc
{
    /**
     * @brief 线性二阶跟踪微分器，对应ADRC1的TD2_L前向欧拉更新。
     * @note x1是平滑输出，x2是其变化率状态；无HAL、RTOS或动态内存依赖。
     */
    class LTD2
    {
    public:
        LTD2() = default;

        /**
         * @param r 有限正频率参数，单位1/s；不是变化率上限。
         * @param output 初始平滑输出，与输入同单位。
         * @param rate 初始变化率，单位为输入单位/s。
         * @return 参数有效且增益可表示时返回true；失败保持原配置与状态。
         */
        bool Init(float r, float output = 0.0f, float rate = 0.0f);

        /** @brief 仅重设两个状态；失败时保持原状态，参数不变。 */
        bool Reset(float output = 0.0f, float rate = 0.0f);

        /**
         * @brief 根据input[k]，将x[k]推进为x[k+1]；两个右侧都使用旧状态。
         * @param dt 实际预测区间/s，要求0<r*dt<2。
         * @return 成功后getter为下一拍状态；未初始化、无效输入或溢出时保持状态。
         * @note 同一实例串行访问，每个区间推进一次；固定周期稳定范围不代表任意抖动保证。
         */
        bool Update(float input, float dt);

        float GetOutput() const { return x1_; }
        float GetRate() const { return x2_; }

    private:
        float r_ = 0.0f;
        float r_squared_ = 0.0f;
        float damping_ = 0.0f;
        float x1_ = 0.0f;
        float x2_ = 0.0f;
    };
}

#endif
