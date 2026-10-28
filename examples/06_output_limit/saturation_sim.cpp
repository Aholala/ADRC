#include "adrc/adrc.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: saturation_sim output.csv\n";
        return 1;
    }
    std::ofstream csv(argv[1]);
    if (!csv)
        return 1;
    csv << "case,feedback,time,target,true_output,requested,applied,observer_input,"
           "true_disturbance,estimated_output,estimated_disturbance\n" << std::setprecision(10);

    // 限幅属于本实验的执行器模型，不是算法核心中的机器人默认值。
    constexpr float limit = 0.2f;
    constexpr double dt = 0.001;
    const char* cases[] = {"transient_saturation", "unreachable_then_release"};
    for (int scenario = 0; scenario < 2; ++scenario)
    {
        adrc::LADRC1 controllers[2];
        double outputs[2] = {0.0, 0.0};
        for (auto& controller : controllers)
        {
            if (!controller.Init(10.0f, 8.0f, 40.0f))
                return 1;
        }
        const double friction = scenario == 1 ? 1.0 : 0.0;
        const int samples = scenario == 1 ? 8000 : 4000;
        for (int k = 0; k < samples; ++k)
        {
            const double time = k * dt;
            const float target = scenario == 0
                ? (time < 0.2 ? 0.0f : (time < 2.0 ? 1.0f : 0.25f))
                : (time < 0.2 ? 0.0f : (time < 3.0 ? 3.0f : 0.5f));
            for (int index = 0; index < 2; ++index)
            {
                const bool feed_applied = index == 0;
                auto& controller = controllers[index];
                const double output = outputs[index];
                float requested = 0.0f;
                if (!controller.Calc(target, requested))
                    return 1;
                const float applied = std::max(-limit, std::min(limit, requested));
                const float observer_input = feed_applied ? applied : requested;
                const double physical_disturbance = -friction * output;
                csv << cases[scenario] << ',' << (feed_applied ? "applied" : "requested")
                    << ',' << time << ',' << target << ',' << output << ',' << requested
                    << ',' << applied << ',' << observer_input << ',' << physical_disturbance
                    << ',' << controller.GetEstimatedOutput() << ','
                    << controller.GetEstimatedDisturbance() << '\n';
                if (!controller.UpdateObserver(observer_input, static_cast<float>(output),
                                               static_cast<float>(dt)))
                    return 1;

                const double drive = 10.0 * applied;
                if (friction > 0.0)
                    outputs[index] = drive / friction + (output - drive / friction)
                                     * std::exp(-friction * dt);
                else
                    outputs[index] = output + dt * drive;
            }
        }
    }
    csv.close();
    if (!csv)
        return 1;
    std::cout << "Saturation CSV: " << argv[1] << '\n';
}
