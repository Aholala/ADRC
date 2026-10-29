#include "ALG_ADRC.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: eso_sim output.csv\n";
        return 1;
    }
    std::ofstream csv(argv[1]);
    if (!csv)
    {
        std::cerr << "Cannot open output CSV\n";
        return 1;
    }
    csv << "case,time,input,measurement,true_output,true_disturbance,omega_o,"
           "estimated_output,estimated_disturbance\n" << std::setprecision(10);

    // 教学参数；对象采用独立的解析 ZOH 更新，ESO 则调用实际的 float C++ 核心。
    constexpr double dt = 0.001;
    constexpr float b0 = 10.0f;
    const float bandwidths[] = {10.0f, 20.0f, 40.0f};
    const char* cases[] = {"load_steps", "model_error", "measurement_noise"};
    for (int scenario = 0; scenario < 3; ++scenario)
    {
        ADRC::LESO2 observers[3];
        for (int index = 0; index < 3; ++index)
        {
            if (!observers[index].Init(b0, bandwidths[index]))
                return 1;
        }
        const double actual_gain = scenario == 1 ? 8.0 : b0;
        const double friction = scenario == 1 ? 0.6 : 0.0;
        double output = 0.0;
        std::mt19937 generator(20261028);
        std::uniform_real_distribution<double> noise(-0.03, 0.03);
        for (int k = 0; k < 4000; ++k)
        {
            const double time = k * dt;
            const float input = time < 1.5 ? 0.5f : 0.2f;
            const double load = time < 0.5 ? 0.0 : (time < 2.5 ? -3.0 : 1.0);
            const double total_disturbance = -friction * output + load
                                            + (actual_gain - b0) * input;
            const double measurement = output + (scenario == 2 ? noise(generator) : 0.0);
            for (int index = 0; index < 3; ++index)
            {
                ADRC::LESO2& observer = observers[index];
                // 比较同一时刻的 y[k]、f[k]、z[k]，再预测下一拍。
                csv << cases[scenario] << ',' << time << ',' << input << ','
                    << measurement << ',' << output << ',' << total_disturbance << ','
                    << bandwidths[index] << ',' << observer.GetOutput() << ','
                    << observer.GetDisturbance() << '\n';
                if (!observer.Update(input, static_cast<float>(measurement),
                                     static_cast<float>(dt)))
                {
                    std::cerr << "Observer update failed at sample " << k << '\n';
                    return 1;
                }
            }

            // 对象 dy/dt=-a*y+b*u+d；输入和负载在每个采样区间内保持不变。
            const double drive = actual_gain * input + load;
            if (friction > 0.0)
            {
                const double equilibrium = drive / friction;
                output = equilibrium + (output - equilibrium) * std::exp(-friction * dt);
            }
            else
            {
                output += dt * drive;
            }
        }
    }
    csv.close();
    if (!csv)
        return 1;
    std::cout << "Simulation CSV: " << argv[1] << '\n';
}
