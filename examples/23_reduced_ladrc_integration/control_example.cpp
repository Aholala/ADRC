// 第23讲：独立降阶LESO接入一阶LADRC。编号1～5为控制流程，编号6仅为主机对象。
#include "ALG_ADRC.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 9)
    {
        std::cerr << "Usage: control_example input.csv output.csv b0 wc wo limit(0=off) initial_y initial_f_hat\n";
        return EXIT_FAILURE;
    }
    float parameters[6];
    for (int i = 0; i < 6; ++i)
    {
        char* end = nullptr;
        parameters[i] = std::strtof(argv[i + 3], &end);
        if (end == argv[i + 3] || *end != '\0' || !std::isfinite(parameters[i]))
            return EXIT_FAILURE;
    }
    const float b0 = parameters[0], wc = parameters[1], wo = parameters[2], limit = parameters[3];
    if (b0 == 0.0f || wc <= 0.0f || wo <= 0.0f || limit < 0.0f)
        return EXIT_FAILURE;
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    if (!input || !output)
        return EXIT_FAILURE;
    std::string line;
    if (!std::getline(input, line))
        return EXIT_FAILURE;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "time,target,disturbance,noise,dt")
        return EXIT_FAILURE;
    output << "time,target,disturbance,noise,dt,measurement,true_y,q_before,f_hat,"
              "requested_input,applied_input,observer_input,q_after,y_next\n";

    ADRC::RLESO1 observer;
    double y = parameters[4];
    unsigned int count = 0;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        double time = 0.0, noise = 0.0;
        float target = 0.0f, force = 0.0f, dt = 0.0f;
        char c1 = 0, c2 = 0, c3 = 0, c4 = 0;
        if (!(row >> time >> c1 >> target >> c2 >> force >> c3 >> noise >> c4 >> dt) ||
            c1 != ',' || c2 != ',' || c3 != ',' || c4 != ',' ||
            !std::isfinite(time) || !std::isfinite(target) || !std::isfinite(force) ||
            !std::isfinite(noise) || !std::isfinite(dt) || dt <= 0.0f)
            return EXIT_FAILURE;
        row >> std::ws;
        if (!row.eof()) return EXIT_FAILURE;
        const float control_step = wc * dt;
        if (!std::isfinite(control_step) || control_step <= 0.0f || control_step >= 2.0f)
            return EXIT_FAILURE;

        // 1. 从自己的对象采集当前测量。硬件工程在这里取得实际速度反馈。
        const float measurement = static_cast<float>(y + noise);
        if (count == 0 && !observer.Init(b0, wo, measurement, parameters[5]))
            return EXIT_FAILURE;
        const float before = observer.GetAuxiliaryState();
        // 2. 只重构当前扰动估计，不推进状态。
        float estimate = 0.0f;
        if (!observer.Calc(measurement, estimate))
            return EXIT_FAILURE;
        // 3. 调用方计算一阶反馈与补偿；未使用对象的真实扰动或真实变化率。
        const float requested = (wc * (target - measurement) - estimate) / b0;
        if (!std::isfinite(requested)) return EXIT_FAILURE;
        // 4. 调用方完成最终输出处理并提交。0仅在本主机工具中表示关闭限幅。
        const float applied = limit > 0.0f ? std::max(-limit, std::min(limit, requested)) : requested;
        const float observer_input = applied;
        // 硬件工程在这里提交最终作用量。若还有后级限制，应继续核对最终输入。
        // 5. 每区间仅更新一次q，下一拍结合新的测量再Calc。
        if (!observer.Update(observer_input, measurement, dt))
            return EXIT_FAILURE;

        // 6. 以下为主机教学对象：区间内u、f恒定时，一阶对象的精确更新。
        const double next_y = y + static_cast<double>(dt) * (static_cast<double>(b0) * applied + force);
        if (!std::isfinite(next_y)) return EXIT_FAILURE;
        output << std::setprecision(17) << time << ',' << std::setprecision(9)
               << target << ',' << force << ',' << std::setprecision(17) << noise << ','
               << std::setprecision(9) << dt << ',' << measurement << ',' << std::setprecision(17) << y << ','
               << std::setprecision(9) << before << ',' << estimate << ',' << requested << ',' << applied << ','
               << observer_input << ',' << observer.GetAuxiliaryState() << ',' << std::setprecision(17) << next_y << '\n';
        y = next_y;
        ++count;
    }
    output.flush();
    if (input.bad() || !output || count == 0)
        return EXIT_FAILURE;
    std::cout << "PASS: autonomous reduced LESO + first-order LADRC rows=" << count << '\n';
}
