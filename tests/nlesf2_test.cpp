#include "ALG_ADRC.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void Check(bool c) {if(!c){std::cerr<<"FAIL: nonlinear feedback\n";std::exit(1);}}
int main()
{
    ADRC::NLESF2 f; float u=123;
    Check(!f.Calc(0,0,0,0,0,u) && u==123);
    Check(f.Init(10,2,0.5f,3,1.5f,0.01f));
    const ADRC::NLESF2& view=f;
    Check(view.Calc(4,1,0,0,2,u) && std::fabs(u-0.5f)<1e-7f);
    Check(view.Calc(0,0,0,0,-2,u) && std::fabs(u-0.2f)<1e-7f);
    const float nan=std::numeric_limits<float>::quiet_NaN();
    Check(!f.Init(0,2,0.5f,3,1.5f,0.01f) && !f.Init(10,2,0,3,1.5f,0.01f));
    u=456;
    Check(!f.Calc(nan,0,0,0,0,u) &&
        !f.Calc(0,std::numeric_limits<float>::max(),0,0,0,u) && u==456);
    Check(f.Calc(4,1,0,0,2,u) && std::fabs(u-0.5f)<1e-7f);
    Check(f.Init(-10,64,1,16,1,0.01f) && f.Calc(1,0,0.5f,0.25f,-2,u));
    Check(std::fabs(u+3.0f)<1e-7f);
    std::cout<<"PASS: SEF hand values, compensation sign, alpha>1, const Calc, failure preservation and linear identity\n";
}
