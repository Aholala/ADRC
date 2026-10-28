// 第19讲：完整C++接入示例。CSV和对象模型仅为主机演示，核心调用顺序见循环中的编号。
#include "adrc/ladrc2.hpp"
#include "adrc/ltd2.hpp"

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
    if (argc != 8)
    {
        std::cerr << "Usage: control_example input.csv output.csv b0 wc wo td_r limit(0=off)\n";
        return EXIT_FAILURE;
    }
    float parameters[5];
    for (int i = 0; i < 5; ++i)
    {
        char* end = nullptr;
        parameters[i] = std::strtof(argv[i + 3], &end);
        if (end == argv[i + 3] || *end != '\0' || !std::isfinite(parameters[i]))
            return EXIT_FAILURE;
    }
    if (parameters[4] < 0.0f)
        return EXIT_FAILURE;

    // 本例对象从零角度、零速度、零扰动开始，初值与之匹配。
    adrc::LTD2 td;
    adrc::LADRC2 controller;
    if (!td.Init(parameters[3], 0.0f, 0.0f) ||
        !controller.Init(parameters[0], parameters[1], parameters[2], 0.0f, 0.0f, 0.0f))
        return EXIT_FAILURE;

    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    if (!input || !output)
        return EXIT_FAILURE;
    std::string line;
    if (!std::getline(input, line) ||
        (line != "time,raw_target,disturbance,dt" && line != "time,raw_target,disturbance,dt\r"))
        return EXIT_FAILURE;
    output << "time,raw_target,disturbance,dt,measurement,true_y,true_velocity,td_x1_before,td_x2_before,"
              "z1_before,z2_before,z3_before,requested_input,applied_input,td_x1_after,td_x2_after,"
              "z1_after,z2_after,z3_after\n";
    double y = 0.0, velocity = 0.0;
    int count = 0;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        double time;
        float raw_target, force, dt;
        char c1, c2, c3;
        if (!(row >> time >> c1 >> raw_target >> c2 >> force >> c3 >> dt) ||
            c1 != ',' || c2 != ',' || c3 != ',' || !std::isfinite(time) || !std::isfinite(force))
            return EXIT_FAILURE;
        row >> std::ws;
        if (!row.eof())
            return EXIT_FAILURE;

        // 1. 当前测量。硬件工程在这里读取与本拍对应的输出反馈。
        const float measurement = static_cast<float>(y);
        // 2. 读取本拍TD输出；v2只记录，不加入本例速度前馈。
        const float smooth_target = td.GetOutput();
        const float target_rate = td.GetRate();
        const float before1 = controller.GetEstimatedOutput();
        const float before2 = controller.GetEstimatedRate();
        const float before3 = controller.GetEstimatedDisturbance();
        // 3. 计算请求，不推进观测器。
        float requested = 0.0f;
        if (!controller.Calc(smooth_target, requested))
            return EXIT_FAILURE;
        // 4. 调用方处理最终输出；参数中的0仅在这个主机程序里表示关闭限幅。
        const float applied = parameters[4] > 0.0f
            ? std::max(-parameters[4], std::min(parameters[4], requested)) : requested;
        // 硬件工程在此提交最终输出；若还有功率限制等处理，应使用最终作用量。
        // 本主机对象假设精确执行applied，对象推进统一放在区间末尾。
        // 5. 用同一区间实际输入和当前测量，将观测器预测到下一拍。
        if (!controller.UpdateObserver(applied, measurement, dt))
            return EXIT_FAILURE;
        // 6. TD也只推进一次，新值供下一拍使用。
        if (!td.Update(raw_target, dt))
            return EXIT_FAILURE;

        output << std::setprecision(17) << time << ',' << std::setprecision(9)
               << raw_target << ',' << force << ',' << dt << ',' << measurement << ','
               << std::setprecision(17) << y << ',' << velocity << ',' << std::setprecision(9)
               << smooth_target << ',' << target_rate << ',' << before1 << ',' << before2 << ',' << before3 << ','
               << requested << ',' << applied << ',' << td.GetOutput() << ',' << td.GetRate() << ','
               << controller.GetEstimatedOutput() << ',' << controller.GetEstimatedRate() << ','
               << controller.GetEstimatedDisturbance() << '\n';

        // 7. 以下仅为离线教学对象；真实角速度和负载没有直接传给TD或控制器。
        const double period = dt;
        const double acceleration = static_cast<double>(parameters[0]) * applied + force;
        y += period * velocity + 0.5 * period * period * acceleration;
        velocity += period * acceleration;
        if (!std::isfinite(y) || !std::isfinite(velocity))
            return EXIT_FAILURE;
        ++count;
    }
    output.flush();
    if (input.bad() || !output || count == 0)
        return EXIT_FAILURE;
    std::cout << "PASS: complete TD + LADRC2 integration rows=" << count << '\n';
}
