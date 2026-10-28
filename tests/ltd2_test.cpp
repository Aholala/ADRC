#include "adrc/adrc.hpp"

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
    adrc::LTD2 td;
    Check(!td.Update(1.0f, 0.01f), "reject Update before Init");
    Check(td.Init(4.0f), "analytic Init");
    constexpr double amplitude = 10.0;
    constexpr double frequency = 4.0;
    constexpr double h = 1.0 / 256.0;
    constexpr double q = frequency * h;
    constexpr double pole = 1.0 - q;
    double peak_rate = 0.0;
    for (int n = 1; n <= 2048; ++n)
    {
        Check(td.Update(static_cast<float>(amplitude), static_cast<float>(h)), "analytic update");
        const double factor = std::pow(pole, n - 1);
        const double expected_output = amplitude * (1.0 - std::pow(pole, n) - n * q * factor);
        const double expected_rate = amplitude * n * frequency * q * factor;
        Near(td.GetOutput(), expected_output, 6e-5, "analytic discrete step output");
        Near(td.GetRate(), expected_rate, 2e-4, "analytic discrete step rate");
        if (td.GetRate() > peak_rate)
            peak_rate = td.GetRate();
    }
    Check(peak_rate > frequency, "frequency parameter is not a rate limit");

    Check(td.Init(5.0f, 0.2f, -0.1f), "nonzero hand calculation Init");
    Check(td.Update(1.0f, 0.01f), "hand calculation update");
    Near(td.GetOutput(), 0.199, 1e-7, "hand calculation uses old rate");
    Near(td.GetRate(), 0.11, 1e-7, "hand calculation rate");
    Check(td.Reset(2.0f), "reset output and zero rate");
    Check(td.Update(2.0f, 0.01f), "constant equilibrium update");
    Near(td.GetOutput(), 2.0, 0.0, "equilibrium output");
    Near(td.GetRate(), 0.0, 0.0, "equilibrium rate");

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float maximum = std::numeric_limits<float>::max();
    Check(!td.Init(0.0f) && !td.Init(-1.0f) && !td.Init(nan) && !td.Init(inf) &&
          !td.Init(maximum) && !td.Init(std::numeric_limits<float>::min()),
          "reject invalid frequency and unrepresentable gains");
    Check(!td.Init(5.0f, nan) && !td.Init(5.0f, 0.0f, inf) &&
          !td.Reset(nan) && !td.Reset(0.0f, inf), "reject invalid initial states");
    Check(!td.Update(nan, 0.01f) && !td.Update(inf, 0.01f) &&
          !td.Update(2.0f, 0.0f) && !td.Update(2.0f, -0.01f) &&
          !td.Update(2.0f, nan) && !td.Update(2.0f, inf) &&
          !td.Update(2.0f, 0.4f) && !td.Update(2.0f, 0.5f),
          "reject invalid input, period and Euler boundary");
    Check(!td.Update(maximum, 0.01f), "reject update overflow");
    Near(td.GetOutput(), 2.0, 0.0, "failure preserves output");
    Near(td.GetRate(), 0.0, 0.0, "failure preserves rate");
    Check(td.Update(1.0f, 0.01f), "valid update after failures");
    Near(td.GetOutput(), 2.0, 0.0, "preserved old rate");
    Near(td.GetRate(), -0.25, 1e-7, "preserved frequency squared");
    Check(td.Update(2.0f, 0.01f), "exercise preserved damping");
    Near(td.GetOutput(), 1.9975, 2e-7, "preserved state progression");
    Near(td.GetRate(), -0.225, 1e-7, "preserved damping");

    Check(td.Reset(-1.0f), "negative signal reset");
    Check(td.Update(-2.0f, 0.01f), "negative signal update");
    Near(td.GetOutput(), -1.0, 0.0, "negative signal old rate");
    Near(td.GetRate(), -0.25, 1e-7, "negative signal direction");
    adrc::LTD2 other;
    Check(other.Init(8.0f, 5.0f) && other.Update(5.0f, 0.01f), "second instance equilibrium");
    Near(td.GetOutput(), -1.0, 0.0, "independent instances");
    std::cout << "PASS: TD2 analytic discrete response, hand calculation, reset, failure preservation, "
                 "period bounds, negative signals, independent instances, no implicit rate clamp\n";
}
