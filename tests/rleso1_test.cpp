#include "ALG_ADRC.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
    void Check(bool value, const char* message)
    {
        if (!value)
        {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    void Near(double actual, double expected, double tolerance, const char* message)
    {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance)
        {
            std::cerr << "actual=" << actual << ", expected=" << expected << '\n';
            Check(false, message);
        }
    }
}

int main()
{
    ADRC::RLESO1 eso;
    float estimate = 123.0f;
    Check(!eso.Calc(3.0f, estimate) && estimate == 123.0f,
          "uninitialized Calc preserves caller output");
    Check(!eso.Update(0.0f, 3.0f, 0.001f) && !eso.Reset(3.0f),
          "uninitialized Update and Reset rejected");
    Check(eso.Init(10.0f, 20.0f, 3.0f), "nonzero measurement Init");
    Near(eso.GetAuxiliaryState(), -60.0, 0.0, "measurement aligned auxiliary state");
    const ADRC::RLESO1& view = eso;
    Check(view.Calc(3.0f, estimate), "const current reconstruction");
    Near(estimate, 0.0, 0.0, "initial disturbance is zero");
    Check(view.Calc(3.01f, estimate), "new measurement reconstruction");
    Near(estimate, 0.2, 4e-6, "measurement direct feedthrough");
    Near(eso.GetAuxiliaryState(), -60.0, 0.0, "Calc does not advance q");
    Check(eso.Update(0.3f, 3.01f, 0.001f), "hand calculation Update");
    Near(eso.GetAuxiliaryState(), -60.064, 4e-6, "hand calculation next q");

    // 独立离散阶跃解析解；二进制精确的周期、输入与对象增量避免夹带测量量化。
    Check(eso.Init(2.0f, 4.0f, 3.0f), "analytic Init");
    float y = 3.0f;
    const float dt = 1.0f / 256.0f;
    const double pole = 1.0 - 4.0 / 256.0;
    for (int n = 0; n <= 1024; ++n)
    {
        Check(eso.Calc(y, estimate), "analytic Calc");
        Near(estimate, -2.0 * (1.0 - std::pow(pole, n)), 1e-4,
             "analytic discrete disturbance response");
        Check(eso.Update(0.25f, y, dt), "analytic Update");
        y += dt * (-1.5f); // b0*u+f = 2*0.25-2。
    }

    // 实际作用量与负载平衡时，两种输入增益符号都应保持平衡。
    Check(eso.Init(-10.0f, 20.0f, 3.0f, -2.0f), "negative input gain Init");
    Check(eso.Update(-0.2f, 3.0f, 0.001f) && eso.Calc(3.0f, estimate),
          "negative gain equilibrium Update");
    Near(estimate, -2.0, 0.0, "negative gain equilibrium");
    Check(eso.Reset(2.0f, -2.0f), "measurement aligned Reset");
    Near(eso.GetAuxiliaryState(), -42.0, 0.0, "Reset preserves bandwidth");

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float maximum = std::numeric_limits<float>::max();
    Check(!eso.Init(0.0f, 20.0f) && !eso.Init(nan, 20.0f) && !eso.Init(inf, 20.0f) &&
          !eso.Init(10.0f, 0.0f) && !eso.Init(10.0f, -1.0f) &&
          !eso.Init(10.0f, nan) && !eso.Init(10.0f, inf) &&
          !eso.Init(10.0f, maximum, 2.0f) && !eso.Init(10.0f, 20.0f, nan) &&
          !eso.Init(10.0f, 20.0f, 0.0f, inf), "invalid Init and initial overflow rejected");
    Check(!eso.Reset(nan) && !eso.Reset(0.0f, inf) && !eso.Reset(maximum),
          "invalid Reset preserves state");
    estimate = 456.0f;
    Check(!eso.Calc(nan, estimate) && !eso.Calc(inf, estimate) &&
          !eso.Calc(maximum, estimate) && estimate == 456.0f,
          "invalid reconstruction preserves output reference");
    Check(!eso.Update(nan, 2.0f, 0.001f) && !eso.Update(inf, 2.0f, 0.001f) &&
          !eso.Update(0.0f, nan, 0.001f) && !eso.Update(0.0f, inf, 0.001f) &&
          !eso.Update(0.0f, 2.0f, 0.0f) && !eso.Update(0.0f, 2.0f, -0.001f) &&
          !eso.Update(0.0f, 2.0f, nan) && !eso.Update(0.0f, 2.0f, inf) &&
          !eso.Update(maximum, 2.0f, 0.001f) && !eso.Update(0.0f, maximum, 0.001f),
          "invalid update and numerical overflow rejected");
    Near(eso.GetAuxiliaryState(), -42.0, 0.0, "all failures preserve q");
    Check(eso.Update(-0.2f, 2.0f, 0.001f) && eso.Calc(2.0f, estimate),
          "valid Update after failures");
    Near(estimate, -2.0, 0.0, "failed Init preserves input gain and bandwidth");

    ADRC::RLESO1 tiny;
    Check(tiny.Init(1.0f, std::numeric_limits<float>::min()), "small representable bandwidth");
    Check(tiny.Update(0.0f, 0.0f, std::numeric_limits<float>::min()),
          "accept positive finite period even when the increment rounds to zero");
    ADRC::RLESO1 large;
    Check(large.Init(1.0f, maximum), "no unnecessary bandwidth squared constraint");
    Check(large.Update(0.0f, 0.0f, 1.0f / maximum), "representable large bandwidth step");
    ADRC::RLESO1 extreme;
    const float half_ulp_probe = std::ldexp(3.0f, 103);
    Check(extreme.Init(1.0f, 1.0f, 2.0f), "extreme reconstruction baseline");
    Check(!extreme.Init(1.0f, 1.0f, half_ulp_probe, maximum) &&
          !extreme.Reset(half_ulp_probe, maximum),
          "reject finite q whose initial reconstructed estimate overflows");
    Near(extreme.GetAuxiliaryState(), -2.0, 0.0, "initial reconstruction failure preserves q");
    ADRC::RLESO1 other;
    Check(other.Init(10.0f, 5.0f, -1.0f, -2.0f) && other.Update(0.2f, -1.0f, 0.01f),
          "second instance equilibrium");
    Near(eso.GetAuxiliaryState(), -42.0, 0.0, "independent instances");
    std::cout << "PASS: reduced LESO analytic response, hand step, const reconstruction, Reset, "
                 "signed input gain, failure preservation, numerical bounds and independent instances\n";
}
