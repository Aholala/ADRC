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
    adrc::LESO3 observer;
    Check(!observer.Update(0.0f, 0.0f, 0.001f), "reject update before Init");
    Check(observer.Init(8.0f, 16.0f), "analytic Init");

    // 与观测器匹配的欧拉对象：三重极点的离散误差解析解，不另写一份ESO。
    constexpr double h = 1.0 / 1024.0;
    constexpr double omega = 16.0;
    constexpr double force = -2.0;
    constexpr double q = omega * h;
    constexpr double pole = 1.0 - q;
    double y = 0.0;
    double velocity = 0.0;
    for (int n = 1; n <= 2048; ++n)
    {
        const float input = n <= 512 ? 0.25f : (n <= 1024 ? -0.125f : 0.0f);
        Check(observer.Update(input, static_cast<float>(y), static_cast<float>(h)), "analytic update");
        y += h * velocity;
        velocity += h * (8.0 * input + force);
        const double first = n * std::pow(pole, n - 1);
        const double second = 0.5 * n * (n - 1) * std::pow(pole, n - 2);
        const double eta1 = -force * h * h * second;
        const double eta2 = -force * h * (first + 2.0 * q * second);
        const double z3 = force * (1.0 - std::pow(pole, n) - q * first - q * q * second);
        Near(observer.GetOutput() - y, eta1, 2e-5, "analytic output error");
        Near(observer.GetRate() - velocity, eta2, 2e-4, "analytic rate error");
        Near(observer.GetDisturbance(), z3, 1e-3, "analytic disturbance response");
    }

    Check(observer.Init(40.0f, 10.0f, 0.8f, 0.3f, -2.0f), "hand calculation Init");
    Check(observer.Update(0.1f, 1.0f, 0.001f), "hand calculation update");
    Near(observer.GetOutput(), 0.8063, 2e-7, "hand calculation output");
    Near(observer.GetRate(), 0.362, 2e-7, "hand calculation rate");
    Near(observer.GetDisturbance(), -1.8, 3e-7, "hand calculation disturbance");

    Check(observer.Init(40.0f, 16.0f), "equilibrium Init");
    Check(observer.Reset(2.0f, 0.0f, -4.0f), "reset all states");
    Check(observer.Update(0.1f, 2.0f, 0.001f), "nonzero equilibrium update");
    Near(observer.GetOutput(), 2.0, 0.0, "equilibrium output");
    Near(observer.GetRate(), 0.0, 0.0, "equilibrium rate");
    Near(observer.GetDisturbance(), -4.0, 0.0, "equilibrium disturbance");

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float maximum = std::numeric_limits<float>::max();
    Check(!observer.Init(0.0f, 16.0f) && !observer.Init(nan, 16.0f) &&
          !observer.Init(inf, 16.0f), "reject invalid gain");
    Check(!observer.Init(40.0f, 0.0f) && !observer.Init(40.0f, -1.0f) &&
          !observer.Init(40.0f, nan) && !observer.Init(40.0f, inf) &&
          !observer.Init(40.0f, 1e15f) &&
          !observer.Init(40.0f, std::numeric_limits<float>::min()),
          "reject bandwidth, cubic overflow and underflow");
    Check(!observer.Init(40.0f, 16.0f, nan) && !observer.Init(40.0f, 16.0f, 0.0f, inf) &&
          !observer.Init(40.0f, 16.0f, 0.0f, 0.0f, nan), "reject initial states");
    Check(!observer.Reset(nan) && !observer.Reset(0.0f, inf) && !observer.Reset(0.0f, 0.0f, nan),
          "reject reset states");
    Check(!observer.Update(nan, 2.0f, 0.001f) && !observer.Update(0.1f, inf, 0.001f),
          "reject invalid input or measurement");
    Check(!observer.Update(0.1f, 2.0f, 0.0f) && !observer.Update(0.1f, 2.0f, -0.001f) &&
          !observer.Update(0.1f, 2.0f, nan) && !observer.Update(0.1f, 2.0f, inf) &&
          !observer.Update(0.1f, 2.0f, 0.125f) && !observer.Update(0.1f, 2.0f, 0.25f),
          "reject invalid period and Euler boundary");
    Check(!observer.Update(maximum, 2.0f, 0.001f), "reject update overflow");
    Near(observer.GetOutput(), 2.0, 0.0, "failure preserves output");
    Near(observer.GetRate(), 0.0, 0.0, "failure preserves rate");
    Near(observer.GetDisturbance(), -4.0, 0.0, "failure preserves disturbance");
    Check(observer.Update(0.1f, 2.0f, 0.001f), "valid update after rejected Init");
    Near(observer.GetRate(), 0.0, 0.0, "failure preserves gain");
    Check(observer.Update(0.1f, 1.5f, 0.001f), "exercise preserved correction gains");
    Near(observer.GetOutput(), 1.976, 3e-7, "failure preserves beta1");
    Near(observer.GetRate(), -0.384, 3e-7, "failure preserves beta2");
    Near(observer.GetDisturbance(), -6.048, 1e-6, "failure preserves beta3");
    Check(observer.Reset(2.0f, 0.0f, -4.0f), "restore independent instance baseline");

    adrc::LESO3 reverse;
    Check(reverse.Init(-40.0f, 16.0f, 1.0f), "negative b0 Init");
    Check(reverse.Update(0.1f, 1.0f, 0.001f), "negative b0 update");
    Near(reverse.GetOutput(), 1.0, 0.0, "known acceleration does not directly change position");
    Near(reverse.GetRate(), -0.004, 1e-8, "negative b0 direction");
    Near(reverse.GetDisturbance(), 0.0, 0.0, "known input is not unknown disturbance");
    Near(observer.GetOutput(), 2.0, 0.0, "independent instances");
    std::cout << "PASS: LESO3 analytic response, known-input changes, hand calculation, reset, "
                 "failure preservation, invalid parameters, negative gain, independent instances\n";
}
