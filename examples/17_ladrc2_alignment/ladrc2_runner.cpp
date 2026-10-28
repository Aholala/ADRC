#include "adrc/linear_adrc2.hpp"

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
    if (argc != 7)
    {
        std::cerr << "Usage: ladrc2_runner input.csv output.csv b0 wc wo limit(0=off)\n";
        return EXIT_FAILURE;
    }
    float parameters[4];
    for (int i = 0; i < 4; ++i)
    {
        char* end = nullptr;
        parameters[i] = std::strtof(argv[i + 3], &end);
        if (end == argv[i + 3] || *end != '\0' || !std::isfinite(parameters[i]))
            return EXIT_FAILURE;
    }
    if (parameters[3] < 0.0f)
        return EXIT_FAILURE;
    adrc::LinearADRC2 controller;
    if (!controller.Init(parameters[0], parameters[1], parameters[2]))
        return EXIT_FAILURE;
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    if (!input || !output)
        return EXIT_FAILURE;
    std::string line;
    if (!std::getline(input, line) || (line != "time,target,disturbance,dt" && line != "time,target,disturbance,dt\r"))
        return EXIT_FAILURE;
    output << "time,target,disturbance,dt,measurement,true_y,true_velocity,z1_before,z2_before,z3_before,"
              "requested_input,applied_input,z1_after,z2_after,z3_after\n";
    double y = 0.0, velocity = 0.0;
    int count = 0;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        double time;
        float target, force, dt;
        char c1, c2, c3;
        if (!(row >> time >> c1 >> target >> c2 >> force >> c3 >> dt) ||
            c1 != ',' || c2 != ',' || c3 != ',' || !std::isfinite(time) ||
            !std::isfinite(target) || !std::isfinite(force))
            return EXIT_FAILURE;
        row >> std::ws;
        if (!row.eof())
            return EXIT_FAILURE;
        const float measurement = static_cast<float>(y);
        const float before1 = controller.GetEstimatedOutput();
        const float before2 = controller.GetEstimatedRate();
        const float before3 = controller.GetEstimatedDisturbance();
        float requested = 0.0f;
        if (!controller.Calc(target, requested))
            return EXIT_FAILURE;
        const float applied = parameters[3] > 0.0f
            ? std::max(-parameters[3], std::min(parameters[3], requested)) : requested;
        if (!controller.UpdateObserver(applied, measurement, dt))
            return EXIT_FAILURE;
        output << std::setprecision(17) << time << ',' << std::setprecision(9)
               << target << ',' << force << ',' << dt << ',' << measurement << ','
               << std::setprecision(17) << y << ',' << velocity << ',' << std::setprecision(9)
               << before1 << ',' << before2 << ',' << before3 << ',' << requested << ',' << applied << ','
               << controller.GetEstimatedOutput() << ',' << controller.GetEstimatedRate() << ','
               << controller.GetEstimatedDisturbance() << '\n';
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
    std::cout << "PASS: LADRC2 autonomous closed-loop rows=" << count << '\n';
}
