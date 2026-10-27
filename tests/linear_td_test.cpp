#include "td.hpp"

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

    void Near(float actual, float expected, float tolerance, const char* message)
    {
        Check(std::isfinite(actual) && std::fabs(actual - expected) <= tolerance, message);
    }
}

int main()
{
    alg_n::LinearTD_c td;
    Check(!td.Calc(1.0f, 0.001f), "uninitialized controller must reject updates");
    Check(td.Init(20.0f), "initialize");

    // 与离散系统的解析阶跃响应比较，而不是重新执行一遍实现公式。
    const double dt = 0.001;
    const double step = 20.0 * dt;
    const double pole = 1.0 - step;
    float previous = 0.0f;
    for (int k = 1; k <= 2000; ++k)
    {
        Check(td.Calc(1.0f, static_cast<float>(dt)), "step update");
        const double expected_value = 1.0 - std::pow(pole, k)
                                      - k * step * std::pow(pole, k - 1);
        const double expected_rate = k * 20.0 * step * std::pow(pole, k - 1);
        Near(td.GetValue(), static_cast<float>(expected_value), 5e-6f, "analytic step value");
        Near(td.GetRate(), static_cast<float>(expected_rate), 1e-4f, "analytic step rate");
        Check(td.GetValue() >= previous && td.GetValue() <= 1.000001f,
              "step must be monotonic for this sample period");
        previous = td.GetValue();
    }
    Near(td.GetValue(), 1.0f, 5e-6f, "constant input converges");
    Near(td.GetRate(), 0.0f, 1e-4f, "constant input derivative converges to zero");

    // 匀速输入稳态：变化率应趋于斜率，平滑值有约 2*v/omega 的滞后误差。
    Check(td.Reset(), "reset before ramp");
    for (int k = 0; k < 4000; ++k)
    {
        Check(td.Calc(0.5f * k * 0.001f, 0.001f), "ramp update");
    }
    Near(td.GetRate(), 0.5f, 2e-4f, "ramp derivative");
    Near(2.0f - td.GetValue(), 0.05f, 1e-4f, "ramp tracking lag");

    Check(td.Reset(3.0f, 0.0f), "reset to current signal");
    Check(td.Calc(3.0f, 0.001f), "constant after reset");
    Near(td.GetValue(), 3.0f, 0.0f, "reset prevents initial transition");
    Near(td.GetRate(), 0.0f, 0.0f, "reset prevents derivative kick");

    // 所有拒绝路径都应保留状态及已有参数。
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    Check(!td.Init(0.0f) && !td.Init(-1.0f) && !td.Init(nan) && !td.Init(inf),
          "invalid bandwidth");
    Check(!td.Init(20.0f, nan) && !td.Reset(inf), "invalid initial states");
    Check(!td.Calc(nan, 0.001f) && !td.Calc(inf, 0.001f), "nonfinite input");
    Check(!td.Calc(0.0f, 0.0f) && !td.Calc(0.0f, -0.001f) &&
          !td.Calc(0.0f, nan) && !td.Calc(0.0f, inf), "invalid period");
    Check(!td.Calc(0.0f, 0.1f) && !td.Calc(0.0f, 0.11f), "unstable period");
    Check(!td.Calc(std::numeric_limits<float>::max(), 0.001f), "overflow rejection");
    Near(td.GetValue(), 3.0f, 0.0f, "failed update preserves value");
    Near(td.GetRate(), 0.0f, 0.0f, "failed update preserves rate");
    Check(td.Calc(3.0f, 0.001f), "valid update after rejected operations");

    alg_n::LinearTD_c other;
    Check(other.Init(10.0f, -2.0f), "second instance");
    Check(other.Calc(-2.0f, 0.001f), "second instance update");
    Near(other.GetValue(), -2.0f, 0.0f, "second instance state");
    Near(td.GetValue(), 3.0f, 0.0f, "instances are independent");

    std::cout << "PASS: analytic step, ramp, reset, invalid inputs, independent instances\n";
}
