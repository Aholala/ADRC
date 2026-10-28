#include "adrc/nonlinear_td2.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition)
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
    adrc::NonlinearTD2 td;
    float acceleration = 123.0f;
    Check(!td.Calc(0.0f, acceleration) && acceleration == 123.0f && !td.Update(0.0f, 0.01f),
          "uninitialized operations rejected, output preserved");
    Check(td.Init(2.0f, 0.5f), "branch probe Init");
    const adrc::NonlinearTD2& view = td;
    Check(view.Calc(0.0f, acceleration), "zero equilibrium Calc");
    Near(acceleration, 0.0, 0.0, "sign zero and equilibrium");
    Check(td.Reset(1.0f) && td.Calc(0.0f, acceleration), "positive far-error branch");
    Near(acceleration, -2.0, 0.0, "positive far-error acceleration");
    Check(td.Reset(-1.0f) && td.Calc(0.0f, acceleration), "negative far-error branch");
    Near(acceleration, 2.0, 0.0, "negative far-error acceleration");
    Check(td.Reset(0.125f) && td.Calc(0.0f, acceleration), "inner continuous branch");
    Near(acceleration, -0.5, 0.0, "small-error hand result");
    Check(td.Reset(0.5f) && td.Calc(0.0f, acceleration), "both equality boundaries");
    Near(acceleration, -2.0, 0.0, "equality boundary result");
    Check(td.Reset(1.0f, -1.0f) && td.Calc(0.0f, acceleration), "y boundary and cancelling rate");
    Near(acceleration, 0.0, 0.0, "braking balance at boundary");
    Check(td.Reset(1.5f, -1.0f) && td.Calc(0.0f, acceleration), "outer y with inner a branch");
    Near(acceleration, 3.0-std::sqrt(17.0), 1e-6, "independent radical hand result");
    Check(td.Reset(0.125f, 0.25f) && td.Calc(0.0f, acceleration), "old-state hand step");
    Near(acceleration, -1.5, 0.0, "hand acceleration");
    Check(view.Calc(0.0f, acceleration), "const repeated Calc");
    Near(td.GetOutput(), 0.125, 0.0, "Calc preserves output");
    Near(td.GetRate(), 0.25, 0.0, "Calc preserves rate");
    Check(td.Update(0.0f, 0.125f), "hand Update");
    Near(td.GetOutput(), 0.15625, 0.0, "output uses old rate");
    Near(td.GetRate(), 0.0625, 0.0, "rate uses old output");

    Check(td.Init(20.0f, 0.01f, 0.95f, 2.0f) && td.Calc(1.0f, acceleration), "early braking Init");
    Near(acceleration, -20.0, 0.0, "braking before reaching target");
    Check(td.Update(1.0f, 0.01f), "early braking Update");
    Near(td.GetOutput(), 0.97, 1e-7, "braking output");
    Near(td.GetRate(), 1.8, 1e-7, "braking rate");

    Check(td.Init(2.0f, 0.125f), "saturated analytic phase Init");
    const float dt = 1.0f / 128.0f;
    bool braking_seen = false;
    double peak_rate = 0.0;
    for (int n = 0; n < 2048; ++n)
    {
        if (n <= 128)
        {
            Near(td.GetOutput(), dt*dt*n*(n-1), 0.0, "saturated acceleration analytic output");
            Near(td.GetRate(), 2.0*dt*n, 0.0, "saturated acceleration analytic rate");
        }
        Check(td.Calc(10.0f, acceleration), "long step Calc");
        Check(std::fabs(acceleration) <= 2.0f+1e-6f, "acceleration bound");
        if (td.GetOutput() < 10.0f && td.GetRate() > 0.0f && acceleration < 0.0f)
            braking_seen = true;
        if (td.GetRate() > peak_rate) peak_rate = td.GetRate();
        const float previous_rate = td.GetRate();
        Check(td.Update(10.0f, dt), "long step Update");
        Check(std::fabs(td.GetRate()-previous_rate) <= dt*2.0f+1e-6f, "rate increment bound");
    }
    Near(td.GetOutput(), 10.0, 1e-4, "constant tracking equilibrium output");
    Near(td.GetRate(), 0.0, 1e-4, "constant tracking equilibrium rate");
    Check(peak_rate > 2.0 && braking_seen, "r is not rate clamp, early braking occurs");

    Check(td.Init(2.0f, 0.5f, 0.125f, 0.25f), "failure-preservation baseline");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float maximum = std::numeric_limits<float>::max();
    Check(!td.Init(0.0f, 0.5f) && !td.Init(-1.0f, 0.5f) && !td.Init(nan, 0.5f) &&
          !td.Init(inf, 0.5f) && !td.Init(maximum, 0.5f) &&
          !td.Init(std::numeric_limits<float>::min(), 0.5f) &&
          !td.Init(2.0f, 0.0f) && !td.Init(2.0f, -0.5f) && !td.Init(2.0f, nan) &&
          !td.Init(2.0f, inf) && !td.Init(2.0f, maximum) &&
          !td.Init(2.0f, 0.5f, nan) && !td.Init(2.0f, 0.5f, 0.0f, inf),
          "invalid configuration and unrepresentable scales rejected");
    Check(!td.Reset(nan) && !td.Reset(0.0f, inf), "invalid Reset rejected");
    acceleration = 456.0f;
    Check(!td.Calc(nan, acceleration) && !td.Calc(inf, acceleration) &&
          !td.Calc(maximum, acceleration) && acceleration == 456.0f,
          "Calc failure preserves caller output");
    Check(!td.Update(nan, 0.01f) && !td.Update(inf, 0.01f) && !td.Update(maximum, 0.01f) &&
          !td.Update(1.0f, 0.0f) && !td.Update(1.0f, -0.01f) && !td.Update(1.0f, nan) &&
          !td.Update(1.0f, inf) && !td.Update(1.0f, 1.0f) && !td.Update(1.0f, 1.1f),
          "invalid inputs and local Euler boundary rejected");
    Near(td.GetOutput(), 0.125, 0.0, "all failures preserve output");
    Near(td.GetRate(), 0.25, 0.0, "all failures preserve rate");
    Check(td.Update(0.0f, 0.125f), "valid update after failures");
    Near(td.GetOutput(), 0.15625, 0.0, "preserved state progression");
    Near(td.GetRate(), 0.0625, 0.0, "failed Init preserves r and h0");

    adrc::NonlinearTD2 overflow;
    Check(overflow.Init(1e-5f, 1.0f, maximum, maximum/2.0f) &&
          overflow.Calc(maximum, acceleration), "finite fhan before state overflow");
    Check(!overflow.Update(maximum, 1.5f), "state overflow rejected");
    Near(overflow.GetOutput(), maximum, 0.0, "overflow preserves output");
    Near(overflow.GetRate(), maximum/2.0f, 0.0, "overflow preserves rate");
    adrc::NonlinearTD2 tiny_step;
    Check(tiny_step.Init(1e-5f, 1e5f) &&
          !tiny_step.Update(0.0f, std::numeric_limits<float>::denorm_min()), "ratio underflow rejected");
    adrc::NonlinearTD2 other;
    Check(other.Init(20.0f, 0.01f, -2.0f) && other.Update(-2.0f, 0.01f), "independent equilibrium");
    Near(td.GetOutput(), 0.15625, 0.0, "independent instances");
    std::cout << "PASS: nonlinear TD original branches, hand steps, saturated analytic phase, early braking, "
                 "constant tracking, const Calc, Reset, failure preservation and numerical bounds\n";
}
