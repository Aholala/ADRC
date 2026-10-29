#include "ALG_ADRC.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void Check(bool value) { if(!value) {std::cerr<<"FAIL: ESO2\n";std::exit(1);} }
static void Near(float actual,float expected,float tolerance=1e-6f) {Check(std::fabs(actual-expected)<=tolerance);}
int main()
{
    ADRC::ESO2 n;
    Check(!n.Update(0,0,0.01f));
    ADRC::ESO2Config c={10,3,4,0.5f,0.25f};
    Check(n.Init(c,1,2));
    Check(n.Update(0.2f,0,0.1f));
    Near(n.GetOutput(),1.1f); Near(n.GetDisturbance(),1.6f);
    Check(n.Reset(0.125f,0));
    Check(n.Update(0,0,0.1f));
    Near(n.GetOutput(),0.0875f); Near(n.GetDisturbance(),-0.1f);
    // Alpha=1 must agree with the independently tested linear observer.
    c={-10,6,9,1,0.01f}; Check(n.Init(c,0.2f,-0.4f));
    ADRC::LESO2 l; Check(l.Init(-10,3,0.2f,-0.4f));
    for(int k=0;k<500;++k) {
        const float u=0.2f*std::sin(0.1f*k),y=0.3f*std::cos(0.2f*k);
        Check(n.Update(u,y,0.001f) && l.Update(u,y,0.001f));
        Near(n.GetOutput(),l.GetOutput(),2e-6f);
        Near(n.GetDisturbance(),l.GetDisturbance(),2e-6f);
    }
    const float old_y=n.GetOutput(),old_f=n.GetDisturbance();
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    const float maximum=std::numeric_limits<float>::max();
    for(int field=0;field<5;++field) {
        ADRC::ESO2Config bad=c;
        switch(field) {case 0:bad.b0=0;break;case 1:bad.beta1=0;break;case 2:bad.beta2=-1;break;case 3:bad.alpha=1.1f;break;default:bad.delta=0;}
        Check(!n.Init(bad)); Near(n.GetOutput(),old_y,0); Near(n.GetDisturbance(),old_f,0);
    }
    c={10,3,maximum,0.25f,std::numeric_limits<float>::min()}; Check(!n.Init(c));
    Check(!n.Reset(nan,0) && !n.Reset(0,inf));
    Check(!n.Update(nan,0,0.001f) && !n.Update(0,inf,0.001f) &&
          !n.Update(0,0,0) && !n.Update(0,0,-1) && !n.Update(0,0,nan) &&
          !n.Update(maximum,0,0.001f));
    Near(n.GetOutput(),old_y,0); Near(n.GetDisturbance(),old_f,0);
    ADRC::ESO2 other; Check(other.Init({1,2,1,1,0.1f}));
    Near(other.GetOutput(),0,0);
    std::cout<<"PASS: ESO2 hand values, old states, linear reduction, signed gain and transactional failures\n";
}
