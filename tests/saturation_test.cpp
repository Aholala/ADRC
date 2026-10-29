#include "ALG_ADRC.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

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

    struct Result
    {
        double output = 0.0;
        float disturbance = 0.0f;
        float last_request = 0.0f;
        double peak_output = 0.0;
        double peak_estimation_error = 0.0;
    };

    Result Run(bool feed_applied, bool friction)
    {
        ADRC::LADRC1 controller;
        Check(controller.Init(10.0f, 8.0f, 40.0f), "saturation Init");
        constexpr float limit = 0.2f;
        constexpr double dt = 0.001;
        const float target = friction ? 3.0f : 1.0f;
        const int samples = friction ? 8000 : 2000;
        Result result;
        for (int k = 0; k < samples; ++k)
        {
            float requested = 0.0f;
            Check(controller.Calc(target, requested), "saturation Calc");
            const float applied = std::max(-limit, std::min(limit, requested));
            Check(std::fabs(applied) <= limit, "actuator respects its amplitude limit");
            const double physical_disturbance = friction ? -result.output : 0.0;
            result.peak_estimation_error = std::max(result.peak_estimation_error,
                std::fabs(controller.GetEstimatedDisturbance() - physical_disturbance));
            Check(controller.UpdateObserver(feed_applied ? applied : requested,
                                            static_cast<float>(result.output),
                                            static_cast<float>(dt)), "saturation observer");
            if (friction)
                result.output = 10.0 * applied + (result.output - 10.0 * applied) * std::exp(-dt);
            else
                result.output += dt * 10.0 * applied;
            Check(std::isfinite(result.output), "finite plant state");
            result.peak_output = std::max(result.peak_output, result.output);
            result.disturbance = controller.GetEstimatedDisturbance();
            result.last_request = requested;
        }
        return result;
    }
}

int main()
{
    const Result correct = Run(true, false);
    const Result incorrect = Run(false, false);
    Check(std::fabs(correct.output - 1.0) < 1e-4, "attainable target converges with applied feedback");
    Check(correct.peak_output <= 1.0001, "matched disturbance-free example has no overshoot");
    Check(correct.peak_estimation_error < 0.002, "known clipping is not absorbed as physical disturbance");
    Check(incorrect.peak_estimation_error > 1.0, "requested-input mismatch corrupts physical disturbance estimate");
    Check(incorrect.peak_output > 1.05, "mismatched observer input causes example overshoot");

    // dy/dt=-y+10*u，|u|<=0.2 时的最高正向稳态速度为 2，无法维持目标 3。
    const Result limited = Run(true, true);
    const Result wound_up = Run(false, true);
    Check(limited.peak_output < 2.001, "actuator authority is not exceeded");
    Check(std::fabs(limited.output - 2.0) < 0.001, "unreachable target is limited by plant equilibrium");
    Check(std::fabs(limited.disturbance + limited.output) < 0.002,
          "physical friction disturbance remains observable under saturation");
    Check(std::fabs(limited.last_request - 1.0f) < 0.002f,
          "correct feedback gives bounded request in this example");
    Check(wound_up.last_request > 10.0f && wound_up.disturbance < -100.0f,
          "wrong feedback accumulates large internal request in this example");

    std::cout << "PASS: applied bounds, attainable target, physical disturbance, "
                 "unreachable equilibrium, requested-input counterexample\n";
}
