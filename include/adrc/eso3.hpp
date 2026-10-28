#ifndef ADRC_ESO3_HPP
#define ADRC_ESO3_HPP
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
#endif
