#include "td.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: td_sim output.csv\n";
        return 1;
    }
    std::ofstream output(argv[1]);
    if (!output)
    {
        std::cerr << "Cannot open output CSV\n";
        return 1;
    }
    output << "case,time,input,value,rate,reference_rate\n" << std::setprecision(9);

    // 仅为无量纲信号仿真参数，不是云台或底盘默认参数。
    constexpr float dt = 0.001f;
    constexpr float bandwidth = 20.0f;
    constexpr float pi = 3.14159265358979323846f;
    const char* cases[] = {"step", "ramp", "sine", "noisy_sine"};
    for (int scenario = 0; scenario < 4; ++scenario)
    {
        alg_n::LinearTD_c td;
        if (!td.Init(bandwidth))
            return 1;
        std::mt19937 generator(20261028);
        std::uniform_real_distribution<float> noise(-0.05f, 0.05f);
        for (int k = 0; k < 4000; ++k)
        {
            const float time = k * dt;
            float input = 0.0f;
            float reference_rate = 0.0f;
            if (scenario == 0)
                input = time >= 2.0f ? -0.5f : (time >= 0.5f ? 1.0f : 0.0f);
            else if (scenario == 1)
            {
                input = time >= 0.5f ? 0.5f * (time - 0.5f) : 0.0f;
                reference_rate = time >= 0.5f ? 0.5f : 0.0f;
            }
            else
            {
                input = std::sin(2.0f * pi * time);
                reference_rate = 2.0f * pi * std::cos(2.0f * pi * time);
                if (scenario == 3)
                    input += noise(generator);
            }

            // 记录更新前的状态 x[k]，对应时刻 t[k]；随后推进到 x[k+1]。
            output << cases[scenario] << ',' << time << ',' << input << ','
                   << td.GetValue() << ',' << td.GetRate() << ',';
            // 阶跃时刻不存在普通意义的一阶导数，不画成全零的“真实导数”。
            if (scenario != 0)
                output << reference_rate;
            output << '\n';
            if (!td.Calc(input, dt))
            {
                std::cerr << "TD update failed: " << cases[scenario] << " sample " << k << '\n';
                return 1;
            }
        }
    }
    output.close();
    if (!output)
        return 1;
    std::cout << "Simulation CSV: " << argv[1] << '\n';
}
