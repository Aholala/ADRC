#include "adrc/nonlinear_eso3.hpp"
#include "adrc/fal.hpp"
#include <cmath>
namespace adrc
{
    bool NonlinearESO3::Init(const NonlinearESO3Config& c,float output,float rate,float disturbance)
    {
        if (!std::isfinite(c.b0) || c.b0==0.0f || !std::isfinite(c.beta1) || c.beta1<=0.0f ||
            !std::isfinite(c.beta2) || c.beta2<=0.0f || !std::isfinite(c.beta3) || c.beta3<=0.0f ||
            !std::isfinite(c.alpha1) || c.alpha1<=0.0f || c.alpha1>1.0f ||
            !std::isfinite(c.alpha2) || c.alpha2<=0.0f || c.alpha2>1.0f ||
            !std::isfinite(c.delta) || c.delta<=0.0f ||
            !std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance)) return false;
        const float local2=c.beta2*std::pow(c.delta,c.alpha1-1.0f);
        const float local3=c.beta3*std::pow(c.delta,c.alpha2-1.0f);
        if (!std::isfinite(local2) || local2<=0.0f || !std::isfinite(local3) || local3<=0.0f) return false;
        config_=c; z1_=output; z2_=rate; z3_=disturbance;
        return true;
    }
    bool NonlinearESO3::Reset(float output,float rate,float disturbance)
    {
        if (!std::isfinite(output) || !std::isfinite(rate) || !std::isfinite(disturbance)) return false;
        z1_=output; z2_=rate; z3_=disturbance;
        return true;
    }
    bool NonlinearESO3::Update(float input,float measurement,float dt)
    {
        if (config_.beta1<=0.0f || !std::isfinite(input) || !std::isfinite(measurement) ||
            !std::isfinite(dt) || dt<=0.0f) return false;
        const float e=z1_-measurement;
        float f1=0.0f,f2=0.0f;
        if (!Fal(e,config_.alpha1,config_.delta,f1) || !Fal(e,config_.alpha2,config_.delta,f2)) return false;
        const float n1=z1_+dt*(z2_-config_.beta1*e);
        const float n2=z2_+dt*(z3_-config_.beta2*f1+config_.b0*input);
        const float n3=z3_+dt*(-config_.beta3*f2);
        if (!std::isfinite(n1) || !std::isfinite(n2) || !std::isfinite(n3)) return false;
        z1_=n1; z2_=n2; z3_=n3;
        return true;
    }
}
