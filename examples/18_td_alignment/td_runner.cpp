#include "adrc/ltd2.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 6)
    {
        std::cerr << "Usage: td_runner input.csv output.csv r initial_output initial_rate\n";
        return EXIT_FAILURE;
    }
    float parameters[3];
    for (int i = 0; i < 3; ++i)
    {
        char* end = nullptr;
        parameters[i] = std::strtof(argv[i + 3], &end);
        if (end == argv[i + 3] || *end != '\0' || !std::isfinite(parameters[i]))
            return EXIT_FAILURE;
    }
    adrc::LTD2 td;
    if (!td.Init(parameters[0], parameters[1], parameters[2]))
        return EXIT_FAILURE;
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    if (!input || !output)
        return EXIT_FAILURE;
    std::string line;
    if (!std::getline(input, line) || (line != "time,input,dt" && line != "time,input,dt\r"))
        return EXIT_FAILURE;
    output << "time,input,dt,x1_before,x2_before,x1_after,x2_after\n";
    int count = 0;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        double time;
        float signal, dt;
        char c1, c2;
        if (!(row >> time >> c1 >> signal >> c2 >> dt) || c1 != ',' || c2 != ',' || !std::isfinite(time))
            return EXIT_FAILURE;
        row >> std::ws;
        if (!row.eof())
            return EXIT_FAILURE;
        const float before1 = td.GetOutput();
        const float before2 = td.GetRate();
        if (!td.Update(signal, dt))
            return EXIT_FAILURE;
        output << std::setprecision(17) << time << ',' << std::setprecision(9)
               << signal << ',' << dt << ',' << before1 << ',' << before2 << ','
               << td.GetOutput() << ',' << td.GetRate() << '\n';
        ++count;
    }
    output.flush();
    if (input.bad() || !output || count == 0)
        return EXIT_FAILURE;
    std::cout << "PASS: TD2 C++ rows=" << count << '\n';
}
