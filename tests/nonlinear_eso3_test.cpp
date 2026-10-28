#include "adrc/nonlinear_eso3.hpp"
#include "adrc/linear_eso3.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void Check(bool c) { if(!c) {std::cerr<<"FAIL: nonlinear ESO\n";std::exit(1);} }
int main()
{
    adrc::NonlinearESO3 n;
    Check(!n.Update(0,0,0.001f));
    adrc::NonlinearESO3Config c={10,3,3,1,0.5f,0.25f,0.01f};
    Check(n.Init(c,0.01f,0,0) && n.Update(0,0,0.001f));
    Check(std::fabs(n.GetOutput()-0.00997f)<1e-8f &&
        std::fabs(n.GetRate()+0.0003f)<1e-8f &&
        std::fabs(n.GetDisturbance()+0.00031622777f)<1e-8f);
    const float z1=n.GetOutput(),z2=n.GetRate(),z3=n.GetDisturbance();
    const float nan=std::numeric_limits<float>::quiet_NaN();
    c.alpha1=0; Check(!n.Init(c)); c.alpha1=0.5f; c.beta2=-1; Check(!n.Init(c));
    c.beta2=3; c.b0=0; Check(!n.Init(c)); c.b0=10; c.alpha2=1.5f; Check(!n.Init(c));
    Check(!n.Reset(nan) && !n.Update(0,nan,0.001f) && !n.Update(0,0,0) &&
        !n.Update(std::numeric_limits<float>::max(),0,0.001f));
    Check(n.GetOutput()==z1 && n.GetRate()==z2 && n.GetDisturbance()==z3);
    // alpha=1时，与全阶LESO比较一整条自主轨迹（允许乘法结合次序的float舍入）。
    c={10,12,48,64,1,1,0.01f};
    adrc::LinearESO3 l;
    Check(n.Init(c) && l.Init(10,4));
    for(int k=0;k<3000;++k) {
        float y=static_cast<float>(std::sin(k*0.001));
        Check(n.Update(0.1f,y,0.001f) && l.Update(0.1f,y,0.001f));
        Check(std::fabs(n.GetOutput()-l.GetOutput())<2e-6f &&
            std::fabs(n.GetRate()-l.GetRate())<2e-5f &&
            std::fabs(n.GetDisturbance()-l.GetDisturbance())<2e-4f);
    }
    c={-10,12,48,64,0.5f,0.25f,0.01f};
    Check(n.Init(c,2,0,-2) && n.Update(-0.2f,2,0.001f));
    Check(n.GetOutput()==2 && n.GetRate()==0 && n.GetDisturbance()==-2);
    adrc::NonlinearESO3 other;
    Check(other.Init(c) && other.Reset(1,2,3) && n.GetOutput()==2);
    std::cout<<"PASS: NLESO hand step, old states, failure preservation, linear reduction, signed gain and instances\n";
}
