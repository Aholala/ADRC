#ifndef ADRC_ALL_HPP
#define ADRC_ALL_HPP

#include <limits>

namespace ADRC
{
    // 统一约定：SetParameters仅用于已初始化对象，保留动态状态和已配置限制；
    // 失败不改变对象。保留状态不等于控制命令连续，同一实例须串行访问。
    // 限制默认+inf（关闭），0表示限制为零；Init/Reset保留限制配置。
    // Update只要求dt有限且为正；参数与周期是否匹配，由调用方通过离线验证确定。

// ============================================================================
// 基础函数：Fal
// ============================================================================

    // 原分段幂函数，alpha>0、delta>0。失败保持value；无时间状态。
    bool Fal(float error, float alpha, float delta, float& value);

// ============================================================================
// 跟踪微分器：LTD2
// ============================================================================

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
         * @return 参数有效、增益可表示且初始速度未超限时返回true；失败保持对象。
         */
        bool Init(float r, float output = 0.0f, float rate = 0.0f);

        /** @brief 已初始化后改参，保留状态和限制；失败不改变对象。 */
        bool SetParameters(float r);
        /** @brief 对称速度/加速度上限，允许0和正无穷；当前速度超限则拒绝。 */
        bool SetLimits(float max_rate, float max_acceleration);
        /** @brief 计算已限制的内部加速度，不推进状态。 */
        bool Calc(float input, float& acceleration) const;

        /** @brief 仅重设状态，速度须在已配置上限内；失败保持对象。 */
        bool Reset(float output = 0.0f, float rate = 0.0f);

        /**
         * @brief 用旧状态计算加速度并限幅，积分后限制速度；位置使用旧速度推进。
         * @param dt 实际预测区间/s，须有限且为正。
         * @return 成功后getter为下一拍状态；未初始化、无效输入或溢出时保持状态。
         * @note 同一实例串行访问，每个区间推进一次；库不进行运行时稳定性判定。
         */
        bool Update(float input, float dt);

        float GetOutput() const { return x1_; }
        float GetRate() const { return x2_; }

    private:
        float r_ = 0.0f;
        float r_squared_ = 0.0f;
        float damping_ = 0.0f;
        float max_rate_ = std::numeric_limits<float>::infinity();
        float max_acceleration_ = std::numeric_limits<float>::infinity();
        float x1_ = 0.0f;
        float x2_ = 0.0f;
    };

// ============================================================================
// 跟踪微分器：TD2
// ============================================================================

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
         * @return 参数、初态、内部尺度有效且初始速度未超限时返回true。
         */
        bool Init(float r, float h0, float output = 0.0f, float rate = 0.0f);

        bool SetParameters(float r, float h0);
        /** @brief 与LTD2相同的速度/加速度限幅规则；当前速度超限则拒绝。 */
        bool SetLimits(float max_rate, float max_acceleration);

        /** @brief 仅重设状态，速度须在上限内；失败保持对象。 */
        bool Reset(float output = 0.0f, float rate = 0.0f);

        /**
         * @brief 用当前状态计算并限制本拍内部加速度，不推进状态。
         * @return 未初始化、输入无效或中间量溢出时返回false，输出引用保持不变。
         */
        bool Calc(float input, float& acceleration) const;

        /**
         * @brief 用旧状态算加速度并限幅，积分后限制速度；位置使用旧速度推进。
         * @param dt 实际区间长度/s，须有限且为正。
         * @return 无效输入、周期或数值溢出时返回false，两个状态保持不变。
         * @note 调用方选择与参数匹配的周期，库不进行运行时稳定性判定。
         */
        bool Update(float input, float dt);

        float GetOutput() const { return x1_; }
        float GetRate() const { return x2_; }

    private:
        float r_ = 0.0f;
        float h0_ = 0.0f;
        float max_rate_ = std::numeric_limits<float>::infinity();
        float max_acceleration_ = std::numeric_limits<float>::infinity();
        float x1_ = 0.0f;
        float x2_ = 0.0f;
    };

// ============================================================================
// 观测器：LESO2
// ============================================================================

    /** @brief 手动线性观测器系数；均为有限值，b0非零、beta为正。 */
    struct LESO2Config
    {
        float b0, beta1, beta2;
    };

    /**
     * @brief 一阶对象y_dot=b0*u+f的二阶线性ESO，前向欧拉预测。
     * @note 无硬件、动态内存和系统时钟依赖；每个实例独立保存状态。
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

        /** @brief 手动增益模式；增益与周期的稳定性由调用方离线验证。 */
        bool Init(const LESO2Config& config, float output = 0.0f,
                  float disturbance = 0.0f);
        bool SetParameters(float b0, float omega_o);
        bool SetParameters(const LESO2Config& config);
        /** @brief 估计总扰动的对称上限，允许0/+inf；状态在下次Update/Reset时限幅。 */
        bool SetDisturbanceLimit(float limit);

        /** @brief 只重置状态；非有限初值返回 false，原状态不变。 */
        bool Reset(float output = 0.0f, float disturbance = 0.0f);

        /**
         * @brief 根据 u[k]、y_meas[k]，由 z[k] 预测 z[k+1]。
         * @param input 对应 [t_k,t_k+dt) 区间的模型输入 u[k]。
         * @param measurement 区间开始时的测量 y_meas[k]。
         * @param dt 预测区间/s，须有限且为正。
         * @return 成功后 getter 是下一拍预测值。未初始化、无效参数、
         *         非正周期或数值溢出返回 false，状态保持不变。
         * @note 参数与周期是否匹配须离线验证，返回true不表示闭环稳定。
         */
        bool Update(float input, float measurement, float dt);

        float GetOutput() const { return z1_; }
        float GetDisturbance() const { return z2_; }

    private:
        float b0_ = 0.0f;
        float beta1_ = 0.0f;
        float beta2_ = 0.0f;
        float disturbance_limit_ = std::numeric_limits<float>::infinity();
        float z1_ = 0.0f;
        float z2_ = 0.0f;
    };

// ============================================================================
// 观测器：LESO3
// ============================================================================

    /** @brief 手动三阶线性观测器系数；库检查数值有效性，不判定增益稳定性。 */
    struct LESO3Config
    {
        float b0, beta1, beta2, beta3;
    };

    /**
     * @brief 二阶对象y_ddot=b0*u+f的三阶线性ESO，前向欧拉预测。
     * @note z1为输出、z2为变化率、z3为总扰动；无硬件和动态内存依赖。
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

        /** @brief 手动增益模式；增益与周期的稳定性由调用方离线验证。 */
        bool Init(const LESO3Config& config, float output = 0.0f,
                  float rate = 0.0f, float disturbance = 0.0f);
        bool SetParameters(float b0, float omega_o);
        bool SetParameters(const LESO3Config& config);
        bool SetDisturbanceLimit(float limit);

        /** @brief 仅重设三个状态；非有限初值返回false，原状态不变。 */
        bool Reset(float output = 0.0f, float rate = 0.0f, float disturbance = 0.0f);

        /**
         * @brief 使用u[k]、y_meas[k]，由z[k]预测z[k+1]。
         * @param input 对应本预测区间的实际模型输入，含调用方最终输出处理。
         * @param measurement 区间开始时的输出测量。
         * @param dt 预测区间/s，须有限且为正。
         * @return 成功后getter为下一拍预测；未初始化、无效输入或溢出时保持状态。
         * @note 每个区间推进一次，同一实例由调用方串行访问。
         *       返回true仅表示计算成功，不表示闭环稳定。
         */
        bool Update(float input, float measurement, float dt);

        float GetOutput() const { return z1_; }
        float GetRate() const { return z2_; }
        float GetDisturbance() const { return z3_; }

    private:
        float b0_ = 0.0f;
        float beta1_ = 0.0f;
        float beta2_ = 0.0f;
        float beta3_ = 0.0f;
        float disturbance_limit_ = std::numeric_limits<float>::infinity();
        float z1_ = 0.0f;
        float z2_ = 0.0f;
        float z3_ = 0.0f;
    };

// ============================================================================
// 观测器：RLESO1
// ============================================================================

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

        /** @brief 给出当前测量，保留该测量处的扰动估计并重算辅助状态q。 */
        bool SetParameters(float b0, float omega_o, float measurement);
        /** @brief 限制Calc返回的扰动及Update重构值，不直接限制辅助状态q。 */
        bool SetDisturbanceLimit(float limit);

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
         * @param dt 区间长度/s，须有限且为正；每区间只更新一次。
         * @return 无效输入、周期或数值溢出时返回false，q保持不变。
         * @note 更新后还需要下一拍测量才能计算下一拍完整扰动估计。
         *       参数与周期的稳定性由调用方离线验证。
         */
        bool Update(float input, float measurement, float dt);

        /** @brief 辅助状态q，供诊断；它本身不是扰动估计。 */
        float GetAuxiliaryState() const { return q_; }

    private:
        float b0_ = 0.0f;
        float omega_o_ = 0.0f;
        float q_ = 0.0f;
        float disturbance_limit_ = std::numeric_limits<float>::infinity();
    };

// ============================================================================
// 观测器：ESO2（面向一阶对象）
// ============================================================================

    /** @brief 非线性ESO2的显式配置；beta为公式系数，delta与输出误差同单位。 */
    struct ESO2Config
    {
        float b0, beta1, beta2, alpha, delta;
    };

    /**
     * @brief 一阶对象y_dot=b0*u+f的二状态非线性观测器。
     * @note z1估计输出，z2估计总扰动；首行线性纠偏，第二行使用Fal。
     *       来自一阶扩张模型的补充实现，不冒充ADRC1中空白ESO模板的原实现。
     */
    class ESO2
    {
    public:
        // b0有限非零，beta1/2>0，0<alpha<=1，delta>0；失败保持配置与状态。
        bool Init(const ESO2Config& config, float output=0.0f, float disturbance=0.0f);
        bool SetParameters(const ESO2Config& config);
        bool SetDisturbanceLimit(float limit);
        bool Reset(float output=0.0f, float disturbance=0.0f);
        // 两行均使用旧状态；dt有限为正，每区间更新一次。参数/周期由调用方验证。
        bool Update(float input, float measurement, float dt);
        float GetOutput() const { return z1_; }
        float GetDisturbance() const { return z2_; }
    private:
        ESO2Config config_ = {};
        float z1_=0.0f, z2_=0.0f;
        float disturbance_limit_ = std::numeric_limits<float>::infinity();
    };

// ============================================================================
// 观测器：ESO3
// ============================================================================

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
        bool SetParameters(const ESO3Config& config);
        bool SetDisturbanceLimit(float limit);
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
        float disturbance_limit_ = std::numeric_limits<float>::infinity();
    };

// ============================================================================
// 控制器：LADRC1
// ============================================================================

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

        bool Init(const LESO2Config& observer, float omega_c, float output = 0.0f,
                  float disturbance = 0.0f);
        /** @brief 同时改反馈与观测器参数，保留估计状态；不保证命令连续。 */
        bool SetParameters(float b0, float omega_c, float omega_o);
        bool SetParameters(const LESO2Config& observer, float omega_c);
        bool SetDisturbanceLimit(float limit);

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
         *       库不进行运行时稳定性判定，参数与周期须由调用方验证。
         */
        bool UpdateObserver(float applied_input, float measurement, float dt);

        float GetEstimatedOutput() const { return observer_.GetOutput(); }
        float GetEstimatedDisturbance() const { return observer_.GetDisturbance(); }

    private:
        float b0_ = 0.0f;
        float omega_c_ = 0.0f;
        LESO2 observer_;
    };

// ============================================================================
// 控制器：LADRC2
// ============================================================================

    /**
     * @brief 二阶对象的最小LADRC：位置与速度反馈、扰动补偿、三阶LESO。
     * @note Calc使用z[k]，UpdateObserver推进为z[k+1]；输入限幅由调用方处理。
     *       支持位置给定或位置与目标速度；本类不包含目标加速度前馈。
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

        bool Init(const LESO3Config& observer, float omega_c, float output = 0.0f,
                  float rate = 0.0f, float disturbance = 0.0f);
        bool SetParameters(float b0, float omega_c, float omega_o);
        bool SetParameters(const LESO3Config& observer, float omega_c);
        bool SetDisturbanceLimit(float limit);

        /** @brief 只复位三个估计状态；失败时保持状态，参数不变。 */
        bool Reset(float output = 0.0f, float rate = 0.0f, float disturbance = 0.0f);

        /**
         * @brief 用当前z[k]计算未限幅请求u[k]，不推进LESO。
         * @return 未初始化、目标非有限或计算溢出时返回false，不改变command。
         */
        bool Calc(float target, float& command) const;

        /**
         * @brief 用目标位置和目标速度计算控制命令，不推进LESO。
         * @param target 目标位置，与估计输出同单位。
         * @param target_rate 目标位置的变化率，单位为输出单位/s。
         * @note 速度误差为target_rate-z2；目标速度须来自有效轨迹或估计，
         *       不能将原始阶跃差分当作无限大的目标速度。失败保持command。
         */
        bool Calc(float target, float target_rate, float& command) const;

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

// ============================================================================
// 误差反馈：NLESF1（数字1表示一项误差）
// ============================================================================

    /** @brief 一阶对象的单项Fal误差反馈与显式扰动补偿，无动态状态。 */
    class NLESF1
    {
    public:
        // beta为期望输出变化率域系数；alpha允许大于1。失败保留已有参数。
        bool Init(float b0, float beta, float alpha, float delta);
        bool SetParameters(float b0, float beta, float alpha, float delta);
        // command=(beta*Fal(target-output)-disturbance)/b0，不推进ESO或TD。
        bool Calc(float target, float output, float disturbance, float& command) const;
    private:
        float b0_=0.0f, beta_=0.0f, alpha_=0.0f, delta_=0.0f;
    };

// ============================================================================
// 误差反馈：NLESF2
// ============================================================================

    // 原NLSEF_2fal两误差反馈，加显式(b0,z3)扰动补偿；不持有ESO/TD、不限幅。
    class NLESF2
    {
    public:
        // beta1/beta2是期望输出二阶导数域系数；允许alpha>1。
        bool Init(float b0,float beta1,float alpha1,float beta2,float alpha2,float delta);
        bool SetParameters(float b0,float beta1,float alpha1,float beta2,float alpha2,float delta);
        // e1=target-output，e2=target_rate-rate。静态计算不推进任何状态。
        // 失败保持command引用。初始target_rate不应误传原始阶跃的无穷导数。
        bool Calc(float target,float target_rate,float output,float rate,
                  float disturbance,float& command) const;
    private:
        float b0_=0.0f,beta1_=0.0f,beta2_=0.0f,alpha1_=0.0f,alpha2_=0.0f,delta_=0.0f;
    };

} // namespace ADRC

#endif
