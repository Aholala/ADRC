#include "adrc/adrc.hpp"

#include <algorithm>
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
    adrc::LADRC2 controller;
    float command = 123.0f;
    Check(!controller.Calc(1.0f, command), "reject uninitialized Calc");
    Check(!controller.UpdateObserver(0.0f, 0.0f, 0.001f), "reject uninitialized observer");
    Near(command, 123.0, 0.0, "failed Calc preserves output argument");
    Check(controller.Init(40.0f, 8.0f, 40.0f, 0.8f, 0.3f, -2.0f), "hand calculation Init");
    const adrc::LADRC2& view = controller;
    Check(view.Calc(1.0f, command), "const Calc");
    Near(command, 0.25, 2e-7, "position, rate and disturbance have correct signs");
    const float previous_command = command;
    Check(controller.Calc(1.0f, command) && command == previous_command, "repeatable static Calc");
    Near(controller.GetEstimatedOutput(), 0.8, 1e-7, "Calc preserves z1");
    Near(controller.GetEstimatedRate(), 0.3, 1e-7, "Calc preserves z2");
    Near(controller.GetEstimatedDisturbance(), -2.0, 0.0, "Calc preserves z3");
    Check(controller.Reset(1.0f, 0.0f, -2.0f), "reset equilibrium");
    Check(controller.Calc(1.0f, command), "equilibrium Calc");
    Near(command, 0.05, 1e-8, "zero error still balances load");
    Check(controller.UpdateObserver(command, 1.0f, 0.001f), "equilibrium observer");
    Near(controller.GetEstimatedOutput(), 1.0, 0.0, "equilibrium output");
    Near(controller.GetEstimatedRate(), 0.0, 0.0, "equilibrium rate");

    // 匹配欧拉对象、零扰动：独立的二重极点阶跃解析解。
    Check(controller.Init(40.0f, 8.0f, 32.0f), "analytic Init");
    constexpr double h = 1.0 / 1024.0;
    constexpr double wc = 8.0;
    constexpr double q = wc * h;
    constexpr double pole = 1.0 - q;
    double y = 0.0, velocity = 0.0;
    for (int n = 1; n <= 2048; ++n)
    {
        Check(controller.Calc(1.0f, command), "analytic Calc");
        Check(controller.UpdateObserver(command, static_cast<float>(y), static_cast<float>(h)),
              "analytic observer update");
        y += h * velocity;
        velocity += h * 40.0 * command;
        const double factor = std::pow(pole, n - 1);
        Near(y, 1.0 - std::pow(pole, n) - n * q * factor, 4e-5, "analytic position response");
        Near(velocity, n * wc * q * factor, 3e-4, "analytic velocity response");
    }

    // 外部限幅后反馈实际输入，闭环仍能维持本例可达的恒定负载。
    Check(controller.Init(40.0f, 8.0f, 40.0f), "limited loop Init");
    y = 0.0; velocity = 0.0;
    for (int k = 0; k < 6000; ++k)
    {
        const float target = k >= 500 ? 1.0f : 0.0f;
        const double force = k >= 2500 ? -2.0 : 0.0;
        Check(controller.Calc(target, command), "limited loop Calc");
        const float applied = std::max(-0.2f, std::min(0.2f, command));
        Check(controller.UpdateObserver(applied, static_cast<float>(y), 0.001f), "actual input feedback");
        const double acceleration = 40.0 * applied + force;
        y += 0.001 * velocity + 0.5 * 0.001 * 0.001 * acceleration;
        velocity += 0.001 * acceleration;
    }
    Near(y, 1.0, 4e-5, "limited loop rejects reachable load");
    Check(controller.Calc(1.0f, command), "steady load Calc");
    Near(command, 0.05, 4e-5, "steady holding input");

    Check(controller.Reset(), "reset before input override");
    Check(controller.Calc(1.0f, command) && command > 0.2f, "Calc is unlimited");
    Check(controller.UpdateObserver(0.0f, 0.0f, 0.001f), "external zero override");
    Near(controller.GetEstimatedOutput(), 0.0, 0.0, "actual zero input z1");
    Near(controller.GetEstimatedRate(), 0.0, 0.0, "actual zero input z2");
    Near(controller.GetEstimatedDisturbance(), 0.0, 0.0, "actual zero input z3");

    Check(controller.Init(40.0f, 8.0f, 40.0f, 0.8f, 0.3f, -2.0f), "failure baseline Init");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float maximum = std::numeric_limits<float>::max();
    Check(!controller.Init(40.0f, 0.0f, 40.0f) && !controller.Init(40.0f, -1.0f, 40.0f) &&
          !controller.Init(40.0f, nan, 40.0f) && !controller.Init(40.0f, inf, 40.0f) &&
          !controller.Init(40.0f, maximum, 40.0f) &&
          !controller.Init(40.0f, std::numeric_limits<float>::min(), 40.0f), "invalid control gains");
    Check(!controller.Init(0.0f, 8.0f, 40.0f) && !controller.Init(40.0f, 8.0f, 0.0f) &&
          !controller.Init(40.0f, 8.0f, 40.0f, nan) &&
          !controller.Init(40.0f, 8.0f, 40.0f, 0.0f, inf) &&
          !controller.Init(40.0f, 8.0f, 40.0f, 0.0f, 0.0f, nan), "invalid observer Init");
    Check(controller.Calc(1.0f, command), "Calc after failed Init");
    Near(command, 0.25, 2e-7, "failed Init preserves configuration and states");
    command = 123.0f;
    Check(!controller.Calc(nan, command) && !controller.Calc(inf, command) &&
          !controller.Calc(maximum, command), "invalid Calc and overflow");
    Near(command, 123.0, 0.0, "Calc failure preserves caller output");
    Check(!controller.Reset(nan) && !controller.Reset(0.0f, inf) &&
          !controller.Reset(0.0f, 0.0f, nan), "invalid Reset");
    Check(!controller.UpdateObserver(inf, 0.8f, 0.001f) &&
          !controller.UpdateObserver(0.0f, nan, 0.001f) &&
          !controller.UpdateObserver(0.0f, 0.8f, 0.0f) &&
          !controller.UpdateObserver(0.0f, 0.8f, 0.05f) &&
          !controller.UpdateObserver(maximum, 0.8f, 0.001f), "observer failure preservation");
    Check(controller.Calc(1.0f, command), "valid Calc after failures");
    Near(command, 0.25, 2e-7, "failed operations preserve state");

    adrc::LADRC2 reverse;
    Check(reverse.Init(-40.0f, 8.0f, 40.0f), "negative gain Init");
    Check(reverse.Calc(1.0f, command), "negative gain Calc");
    Near(command, -1.6, 2e-7, "negative gain control direction");
    Near(controller.GetEstimatedOutput(), 0.8, 1e-7, "independent instances");
    std::cout << "PASS: LADRC2 signs, const/static Calc, equilibrium, analytic closed loop, "
                 "external limits and actual input, failure preservation, negative gain\n";
}
