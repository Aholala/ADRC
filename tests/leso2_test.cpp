#include "adrc/leso2.hpp"

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
    adrc::LESO2 observer;
    Check(!observer.Update(0.0f, 0.0f, 0.001f), "reject update before Init");
    Check(observer.Init(10.0f, 20.0f), "initialize observer");

    // 比较离散误差系统的解析解；控制输入变化不应被错误归入扰动。
    constexpr double dt = 0.001;
    constexpr double omega = 20.0;
    constexpr double force = -3.0;
    constexpr double pole = 1.0 - omega * dt;
    double output = 0.0;
    for (int k = 1; k <= 2000; ++k)
    {
        const float input = k < 600 ? 0.5f : (k < 1200 ? -0.25f : 0.125f);
        Check(observer.Update(input, static_cast<float>(output), static_cast<float>(dt)),
              "analytic response update");
        output += dt * (10.0 * input + force);
        const double power = std::pow(pole, k - 1);
        const double expected_error = -force * k * dt * power;
        const double expected_disturbance = force * (1.0 - std::pow(pole, k)
                                                    - k * omega * dt * power);
        Near(observer.GetOutput() - output, expected_error, 5e-5,
             "output error agrees with analytic discrete solution");
        Near(observer.GetDisturbance(), expected_disturbance, 8e-4,
             "disturbance agrees with analytic discrete solution");
    }
    Near(observer.GetDisturbance(), force, 8e-4, "constant disturbance converges");

    // 减小步长应使前向欧拉结果接近上一节的连续时间解析解。
    const double expected_continuous = force * (1.0 - (1.0 + omega * 0.1)
                                               * std::exp(-omega * 0.1));
    double previous_error = std::numeric_limits<double>::infinity();
    for (int samples = 20; samples <= 80; samples *= 2)
    {
        const double period = 0.1 / samples;
        adrc::LESO2 refinement;
        Check(refinement.Init(10.0f, static_cast<float>(omega)), "refinement Init");
        for (int k = 0; k < samples; ++k)
        {
            Check(refinement.Update(0.0f, static_cast<float>(force * k * period),
                                    static_cast<float>(period)), "refinement update");
        }
        const double error = std::fabs(refinement.GetDisturbance() - expected_continuous);
        Check(error < previous_error, "smaller period approaches continuous solution");
        previous_error = error;
    }

    // 与参考代码中的教学手算对应，且两个新状态使用同一拍旧状态。
    Check(observer.Init(10.0f, 10.0f, 0.8f, 0.0f), "initialize hand calculation");
    Check(observer.Update(0.5f, 1.0f, 0.001f), "hand calculation update");
    Near(observer.GetOutput(), 0.809, 2e-7, "hand calculation output");
    Near(observer.GetDisturbance(), 0.020, 2e-7, "hand calculation disturbance");

    Check(observer.Reset(3.0f, -2.0f), "reset output and disturbance");
    Check(observer.Update(0.2f, 3.0f, 0.001f), "balanced known disturbance after reset");
    Near(observer.GetOutput(), 3.0, 0.0, "known input balances initial disturbance");
    Near(observer.GetDisturbance(), -2.0, 0.0, "reset retains nonzero disturbance");
    Check(observer.Reset(3.0f, 0.0f), "reset output");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float maximum = std::numeric_limits<float>::max();
    Check(!observer.Init(0.0f, 10.0f) && !observer.Init(nan, 10.0f) &&
          !observer.Init(inf, 10.0f), "invalid input gain");
    Check(!observer.Init(10.0f, 0.0f) && !observer.Init(10.0f, -1.0f) &&
          !observer.Init(10.0f, inf) && !observer.Init(10.0f, nan) &&
          !observer.Init(10.0f, maximum), "invalid bandwidth");
    Check(!observer.Init(10.0f, 10.0f, nan) && !observer.Reset(inf) &&
          !observer.Reset(0.0f, nan), "invalid initial states");
    Check(!observer.Update(nan, 0.0f, 0.001f) &&
          !observer.Update(0.0f, inf, 0.001f), "invalid measurements or input");
    Check(!observer.Update(0.0f, 0.0f, 0.0f) &&
          !observer.Update(0.0f, 0.0f, -0.001f) &&
          !observer.Update(0.0f, 0.0f, nan) &&
          !observer.Update(0.0f, 0.0f, inf), "invalid sample period");
    Check(!observer.Update(0.0f, 0.0f, 0.2f) &&
          !observer.Update(0.0f, 0.0f, 0.25f), "Euler stability boundary");
    Check(!observer.Update(maximum, 0.0f, 0.001f), "overflow does not corrupt state");
    Near(observer.GetOutput(), 3.0, 0.0, "failure preserves output");
    Near(observer.GetDisturbance(), 0.0, 0.0, "failure preserves disturbance");
    Check(observer.Update(0.0f, 3.0f, 0.001f), "valid update after rejections");
    Near(observer.GetOutput(), 3.0, 0.0, "failure preserves parameters");

    adrc::LESO2 reverse;
    Check(reverse.Init(-10.0f, 20.0f, 2.0f), "negative gain is allowed");
    Check(reverse.Update(0.5f, 2.0f, 0.001f), "negative gain update");
    Near(reverse.GetOutput(), 1.995, 2e-7, "negative input gain direction");
    Near(reverse.GetDisturbance(), 0.0, 0.0, "known input is not disturbance");
    Near(observer.GetOutput(), 3.0, 0.0, "independent instances");

    std::cout << "PASS: analytic discrete response, step refinement, input changes, hand calculation, "
                 "reset, invalid inputs, negative gain, independent instances\n";
}
