#include "adrc/fal.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void Check(bool c) { if(!c) { std::cerr<<"FAIL: fal\n"; std::exit(1); } }
int main()
{
    float f=123.0f;
    Check(adrc::Fal(0.005f,0.5f,0.01f,f) && std::fabs(f-0.05f)<1e-7f);
    Check(adrc::Fal(-4.0f,0.5f,0.01f,f) && f==-2.0f);
    Check(adrc::Fal(0.01f,0.5f,0.01f,f) && std::fabs(f-0.1f)<1e-7f);
    Check(adrc::Fal(4.0f,1.5f,0.01f,f) && f==8.0f);
    for(int i=-100;i<=100;++i) {
        const float e=i*0.03125f; float neg=0;
        Check(adrc::Fal(e,1.0f,0.1f,f) && f==e);
        Check(adrc::Fal(e,0.25f,0.01f,f) && adrc::Fal(-e,0.25f,0.01f,neg) && f==-neg);
    }
    f=123;
    const float nan=std::numeric_limits<float>::quiet_NaN();
    Check(!adrc::Fal(nan,0.5f,0.01f,f) && !adrc::Fal(0,0,0.01f,f) &&
        !adrc::Fal(0,0.5f,0,f) && !adrc::Fal(1,0.5f,nan,f) &&
        !adrc::Fal(std::numeric_limits<float>::max(),2,0.1f,f) && f==123);
    std::cout<<"PASS: fal hand values, alpha>1, oddness, linear identity, boundaries and failure output\n";
}
