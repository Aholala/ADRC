#ifndef ADRC_ALL_HPP
#define ADRC_ALL_HPP

// ADRC算法库公共接口；各类保持独立，按基础函数、TD、ESO和控制器排列。

// ============================================================================
// 基础函数：Fal
// ============================================================================

namespace adrc
{
    // 原分段幂函数，alpha>0、delta>0。失败保持value；无时间状态。
    bool Fal(float error, float alpha, float delta, float& value);
}

// ============================================================================
// 跟踪微分器：LTD2
// ============================================================================

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

// ============================================================================
// 跟踪微分器：TD2
// ============================================================================

namespace adrc
{
    /**
     * @brief 对应ADRC1/TD2_NL.m与原分段fhan的非线性二阶TD。
     * @note 无HAL、RTOS、动态内存或时钟依赖；同一实例串行访问。
     */
    class TD2
    {
    public:
        TD2() = default;

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

// ============================================================================
// 观测器：LESO2
// ============================================================================

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

// ============================================================================
// 观测器：LESO3
// ============================================================================

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

// ============================================================================
// 观测器：RLESO1
// ============================================================================

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

// ============================================================================
// 观测器：ESO3
// ============================================================================

namespace adrc
{
    // beta是原公式系数，不是未换算的LESO带宽参数。所有字段由调用方提供。
    struct ESO3Config
    {
        float b0, beta1, beta2, beta3, alpha1, alpha2, delta;
    };
    class ESO3
    {
    public:
        // 有效域：b0非零，beta>0，0<alpha<=1，delta>0；失败保留配置与状态。
        bool Init(const ESO3Config& config, float output=0.0f,
                  float rate=0.0f, float disturbance=0.0f);
        bool Reset(float output=0.0f, float rate=0.0f, float disturbance=0.0f);
        // 原ESO2_NL连续导数的前向欧拉。三个右侧都使用旧状态。
        // dt须有限为正；本类不声称存在通用非线性稳定步长，调用方须验证参数/周期。
        // 失败保持状态。每区间仅更新一次，输入为最终实际作用量。
        bool Update(float input, float measurement, float dt);
        float GetOutput() const { return z1_; }
        float GetRate() const { return z2_; }
        float GetDisturbance() const { return z3_; }
    private:
        ESO3Config config_ = {};
        float z1_=0.0f, z2_=0.0f, z3_=0.0f;
    };
}

// ============================================================================
// 控制器：LADRC1
// ============================================================================

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

// ============================================================================
// 控制器：LADRC2
// ============================================================================

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

// ============================================================================
// 误差反馈：SEF2
// ============================================================================

namespace adrc
{
    // 原NLSEF_2fal两误差反馈，加显式(b0,z3)扰动补偿；不持有ESO/TD、不限幅。
    class SEF2
    {
    public:
        // beta1/beta2是期望输出二阶导数域系数；允许alpha>1。
        bool Init(float b0,float beta1,float alpha1,float beta2,float alpha2,float delta);
        // e1=target-output，e2=target_rate-rate。静态计算不推进任何状态。
        // 失败保持command引用。初始target_rate不应误传原始阶跃的无穷导数。
        bool Calc(float target,float target_rate,float output,float rate,
                  float disturbance,float& command) const;
    private:
        float b0_=0.0f,beta1_=0.0f,beta2_=0.0f,alpha1_=0.0f,alpha2_=0.0f,delta_=0.0f;
    };
}

#endif // ADRC_ALL_HPP
