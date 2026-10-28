#include "adrc/linear_adrc.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ladrc_sim output.csv\n";
        return 1;
    }
    std::ofstream csv(argv[1]);
    if (!csv)
        return 1;
    csv << "case,compensation,time,target,measurement,true_output,input,load,"
           "true_disturbance,estimated_output,estimated_disturbance\n" << std::setprecision(10);

    // 教学参数，不是机器人默认配置。控制量在本例不做限幅。
    constexpr float b0 = 10.0f;
    constexpr float omega_c = 8.0f;
    constexpr float omega_o = 40.0f;
    constexpr double dt = 0.001;
    const char* cases[] = {"matched_plant", "model_error", "measurement_noise"};
    for (int scenario = 0; scenario < 3; ++scenario)
    {
        adrc::LinearADRC1 controllers[2];
        double outputs[2] = {0.0, 0.0};
        for (auto& controller : controllers)
        {
            if (!controller.Init(b0, omega_c, omega_o))
                return 1;
        }
        const double actual_gain = scenario == 1 ? 8.0 : b0;
        const double friction = scenario == 1 ? 0.6 : 0.0;
        std::mt19937 generator(20261028);
        std::uniform_real_distribution<double> noise(-0.03, 0.03);
        for (int k = 0; k < 5000; ++k)
        {
            const double time = k * dt;
            const float target = time < 0.2 ? 0.0f : (time < 3.0 ? 1.0f : 0.5f);
            const double load = time >= 1.5 && time < 4.0 ? -3.0 : 0.0;
            // 两条闭环使用相同的测量噪声样本，各自拥有独立对象和 ESO。
            const double sample_noise = scenario == 2 ? noise(generator) : 0.0;
            for (int index = 0; index < 2; ++index)
            {
                const bool compensate = index == 1;
                auto& controller = controllers[index];
                const double output = outputs[index];
                const double measurement = output + sample_noise;
                float command = 0.0f;
                if (compensate)
                {
                    if (!controller.Calc(target, command))
                        return 1;
                }
                else
                {
                    // 仅用于实验的对照：保留同一状态反馈与 ESO，去掉 -z2 项。
                    command = omega_c * (target - controller.GetEstimatedOutput()) / b0;
                }
                const double disturbance = -friction * output + load
                                           + (actual_gain - b0) * command;
                // 比较 k 拍的真实状态、估计和当前控制，再推进到 k+1。
                csv << cases[scenario] << ',' << (compensate ? "on" : "off") << ','
                    << time << ',' << target << ',' << measurement << ',' << output << ','
                    << command << ',' << load << ',' << disturbance << ','
                    << controller.GetEstimatedOutput() << ','
                    << controller.GetEstimatedDisturbance() << '\n';
                if (!controller.UpdateObserver(command, static_cast<float>(measurement),
                                               static_cast<float>(dt)))
                    return 1;

                const double drive = actual_gain * command + load;
                if (friction > 0.0)
                {
                    const double equilibrium = drive / friction;
                    outputs[index] = equilibrium + (output - equilibrium)
                                     * std::exp(-friction * dt);
                }
                else
                {
                    outputs[index] = output + dt * drive;
                }
            }
        }
    }
    csv.close();
    if (!csv)
        return 1;
    std::cout << "Closed-loop CSV: " << argv[1] << '\n';
}
