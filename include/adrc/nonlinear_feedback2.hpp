#ifndef ADRC_NONLINEAR_FEEDBACK2_HPP
#define ADRC_NONLINEAR_FEEDBACK2_HPP
namespace adrc
{
    // 原NLSEF_2fal两误差反馈，加显式(b0,z3)扰动补偿；不持有ESO/TD、不限幅。
    class NonlinearFeedback2
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
#endif
