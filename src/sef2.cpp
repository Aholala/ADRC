#include "adrc/sef2.hpp"
#include "adrc/fal.hpp"
#include <cmath>
namespace adrc
{
    bool SEF2::Init(float b0,float b1,float a1,float b2,float a2,float delta)
    {
        if (!std::isfinite(b0) || b0==0.0f || !std::isfinite(b1) || b1<=0.0f ||
            !std::isfinite(b2) || b2<=0.0f || !std::isfinite(a1) || a1<=0.0f ||
            !std::isfinite(a2) || a2<=0.0f || !std::isfinite(delta) || delta<=0.0f) return false;
        const float local1=b1*std::pow(delta,a1-1.0f),local2=b2*std::pow(delta,a2-1.0f);
        if (!std::isfinite(local1) || local1<=0.0f || !std::isfinite(local2) || local2<=0.0f) return false;
        b0_=b0; beta1_=b1; beta2_=b2; alpha1_=a1; alpha2_=a2; delta_=delta;
        return true;
    }
    bool SEF2::Calc(float target,float target_rate,float output,float rate,
                                   float disturbance,float& command) const
    {
        if (b0_==0.0f || !std::isfinite(target) || !std::isfinite(target_rate) ||
            !std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance)) return false;
        float f1=0.0f,f2=0.0f;
        if (!Fal(target-output,alpha1_,delta_,f1) || !Fal(target_rate-rate,alpha2_,delta_,f2)) return false;
        const float result=(beta1_*f1+beta2_*f2-disturbance)/b0_;
        if (!std::isfinite(result)) return false;
        command=result;
        return true;
    }
}
