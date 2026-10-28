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

    double RunConstantLoad(bool compensate)
    {
        adrc::LADRC1 controller;
        Check(controller.Init(10.0f, 8.0f, 40.0f), "load test Init");
        double output = 0.0;
        for (int k = 0; k < 4000; ++k)
        {
            float command = 0.0f;
            if (compensate)
                Check(controller.Calc(1.0f, command), "load test Calc");
            else
                command = 8.0f * (1.0f - controller.GetEstimatedOutput()) / 10.0f;
            Check(controller.UpdateObserver(command, static_cast<float>(output), 0.001f),
                  "load test observer");
            output += 0.001 * (10.0 * command - 3.0);
        }
        return output;
    }
}

int main()
{
    adrc::LADRC1 controller;
    float command = 123.0f;
    Check(!controller.Calc(1.0f, command), "reject Calc before Init");
    Near(command, 123.0, 0.0, "failed Calc preserves caller output");
    Check(controller.Init(10.0f, 8.0f, 40.0f, 0.8f, -3.0f), "initialize controller");
    Check(controller.Calc(1.0f, command), "hand calculation");
    Near(command, 0.46, 1e-7, "feedback and compensation have consistent units");
    // Calc 是静态反馈，不能重复推进 ESO。
    Check(controller.Calc(1.0f, command), "repeated Calc");
    Near(controller.GetEstimatedOutput(), 0.8, 1e-7, "Calc leaves ESO state unchanged");
    Near(controller.GetEstimatedDisturbance(), -3.0, 0.0, "Calc preserves disturbance");

    Check(controller.Reset(1.0f, -3.0f), "reset balanced state");
    Check(controller.Calc(1.0f, command), "zero tracking error with load");
    Near(command, 0.3, 1e-7, "compensation remains when tracking error is zero");
    Check(controller.UpdateObserver(command, 1.0f, 0.001f), "balanced state prediction");
    Near(controller.GetEstimatedOutput(), 1.0, 1e-7, "balanced speed is constant");

    // 匹配对象、零扰动时，对照一阶闭环的离散解析阶跃响应。
    Check(controller.Reset(), "reset before analytic response");
    double output = 0.0;
    for (int k = 1; k <= 1500; ++k)
    {
        Check(controller.Calc(1.0f, command), "analytic Calc");
        Check(controller.UpdateObserver(command, static_cast<float>(output), 0.001f),
              "analytic observer");
        output += 0.001 * 10.0 * command;
        Near(output, 1.0 - std::pow(1.0 - 8.0 * 0.001, k), 3e-5,
             "closed loop agrees with discrete analytic response");
    }
    Near(RunConstantLoad(true), 1.0, 3e-5, "LADRC rejects constant load");
    Near(RunConstantLoad(false), 1.0 - 3.0 / 8.0, 3e-5,
         "feedback alone has predicted steady error");

    // 外部把控制量改为零时，ESO 应接收零，而非 Calc 的原始输出。
    Check(controller.Reset(), "reset before actuator input check");
    Check(controller.Calc(1.0f, command), "compute command before external override");
    Check(command > 0.0f, "unlimited command is nonzero");
    Check(controller.UpdateObserver(0.0f, 0.0f, 0.001f), "feed actually applied zero input");
    Near(controller.GetEstimatedOutput(), 0.0, 0.0, "observer models applied input");
    Near(controller.GetEstimatedDisturbance(), 0.0, 0.0, "no fictitious disturbance");

    Check(controller.Init(-10.0f, 8.0f, 40.0f), "negative input gain Init");
    Check(controller.Calc(1.0f, command), "negative input gain Calc");
    Near(command, -0.8, 1e-7, "negative gain command direction");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    Check(!controller.Init(10.0f, 0.0f, 40.0f) &&
          !controller.Init(10.0f, -1.0f, 40.0f) &&
          !controller.Init(10.0f, nan, 40.0f) &&
          !controller.Init(10.0f, inf, 40.0f), "invalid controller bandwidth");
    Check(!controller.Init(0.0f, 8.0f, 40.0f) &&
          !controller.Init(10.0f, 8.0f, 0.0f) &&
          !controller.Init(10.0f, 8.0f, 40.0f, nan), "invalid ESO Init");
    Check(controller.Calc(1.0f, command), "valid Calc after failed Init");
    Near(command, -0.8, 1e-7, "failed Init preserves existing parameters");
    command = 123.0f;
    Check(!controller.Calc(nan, command) && !controller.Calc(inf, command) &&
          !controller.Calc(std::numeric_limits<float>::max(), command), "invalid control calculation");
    Near(command, 123.0, 0.0, "failed calculations preserve caller output");
    Check(!controller.Reset(inf) && !controller.UpdateObserver(0.0f, nan, 0.001f),
          "failed state operations");
    Near(controller.GetEstimatedOutput(), 0.0, 0.0, "failed operations preserve state");

    std::cout << "PASS: control units, equilibrium, static Calc, analytic closed loop, "
                 "load rejection comparison, applied input, negative gain, invalid parameters\n";
}
