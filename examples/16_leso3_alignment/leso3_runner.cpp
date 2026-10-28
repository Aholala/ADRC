#include "adrc/leso3.hpp"

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
        std::cerr << "Usage: leso3_runner input.csv output.csv b0 omega_o z1 z2 z3\n";
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
    adrc::LESO3 observer;
    if (!observer.Init(parameters[0], parameters[1], parameters[2], parameters[3], parameters[4]))
        return EXIT_FAILURE;
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    if (!input || !output)
        return EXIT_FAILURE;
    std::string line;
    if (!std::getline(input, line) || (line != "time,input,measurement,dt" && line != "time,input,measurement,dt\r"))
        return EXIT_FAILURE;
    output << "time,input,measurement,dt,z1_before,z2_before,z3_before,z1_after,z2_after,z3_after\n";
    int count = 0;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        double time;
        float applied, measurement, dt;
        char c1, c2, c3;
        if (!(row >> time >> c1 >> applied >> c2 >> measurement >> c3 >> dt) ||
            c1 != ',' || c2 != ',' || c3 != ',' || !std::isfinite(time))
            return EXIT_FAILURE;
        row >> std::ws;
        if (!row.eof())
            return EXIT_FAILURE;
        const float before1 = observer.GetOutput();
        const float before2 = observer.GetRate();
        const float before3 = observer.GetDisturbance();
        if (!observer.Update(applied, measurement, dt))
        {
            std::cerr << "Update rejected at data row " << count + 1 << '\n';
            return EXIT_FAILURE;
        }
        output << std::setprecision(17) << time << ',' << std::setprecision(9)
               << applied << ',' << measurement << ',' << dt << ','
               << before1 << ',' << before2 << ',' << before3 << ','
               << observer.GetOutput() << ',' << observer.GetRate() << ',' << observer.GetDisturbance() << '\n';
        ++count;
    }
    output.flush();
    if (input.bad() || !output || count == 0)
        return EXIT_FAILURE;
    std::cout << "PASS: LESO3 C++ rows=" << count << '\n';
}
