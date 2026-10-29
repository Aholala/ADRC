#include "ALG_ADRC.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void Check(bool value) {if(!value){std::cerr<<"FAIL: NLESF1\n";std::exit(1);}}
static void Near(float actual,float expected,float tolerance=1e-6f) {Check(std::fabs(actual-expected)<=tolerance);}
int main()
{
    ADRC::NLESF1 f;float u=123;
    Check(!f.Calc(0,0,0,u)); Near(u,123,0);
    Check(f.Init(2,3,0.5f,0.1f));
    const ADRC::NLESF1& view=f;
    Check(view.Calc(4,0,1,u)); Near(u,2.5f);
    Check(view.Calc(-4,0,1,u)); Near(u,-3.5f);
    Check(f.Init(-2,3,1.5f,0.1f));
    Check(f.Calc(4,0,1,u)); Near(u,-11.5f);
    // With alpha=1 the static law must reproduce LADRC1 at the same estimates.
    ADRC::LADRC1 l; Check(l.Init(10,4,8,0.3f,-0.5f)); Check(f.Init(10,4,1,0.01f));
    for(int k=-20;k<=20;++k) {
        float expected=0; const float r=0.1f*k;
        Check(l.Calc(r,expected) && f.Calc(r,l.GetEstimatedOutput(),l.GetEstimatedDisturbance(),u));
        Near(u,expected,0);
    }
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    Check(!f.Init(0,4,1,0.1f) && !f.Init(10,0,1,0.1f) && !f.Init(10,4,0,0.1f) &&
          !f.Init(10,4,1,0) && !f.Init(nan,4,1,0.1f));
    u=123;Check(!f.Calc(nan,0,0,u) && !f.Calc(0,inf,0,u) && !f.Calc(0,0,inf,u));Near(u,123,0);
    Check(f.Calc(1,0.3f,-0.5f,u)); Near(u,0.33f);
    Check(f.Init(1,1,2,0.1f));u=123;
    Check(!f.Calc(std::numeric_limits<float>::max(),0,0,u));Near(u,123,0);
    std::cout<<"PASS: NLESF1 nonlinear hand values, alpha>1, signed gain, LADRC1 reduction and failure preservation\n";
}
