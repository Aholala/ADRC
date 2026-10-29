#include "ALG_ADRC.hpp"

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
        std::cerr << "Usage: td_runner input.csv output.csv r h0 output0 rate0\n";
        return EXIT_FAILURE;
    }
    float parameters[4];
    for (int i = 0; i < 4; ++i)
    {
        char* end = nullptr;
        parameters[i] = std::strtof(argv[i+3], &end);
        if (end == argv[i+3] || *end != '\0' || !std::isfinite(parameters[i])) return EXIT_FAILURE;
    }
    ADRC::TD2 td;
    if (!td.Init(parameters[0],parameters[1],parameters[2],parameters[3])) return EXIT_FAILURE;
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    std::string line;
    if (!input || !output || !std::getline(input,line)) return EXIT_FAILURE;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "time,input,dt") return EXIT_FAILURE;
    output << "time,input,dt,x1_before,x2_before,acceleration,x1_after,x2_after\n";
    unsigned int count = 0;
    while (std::getline(input,line))
    {
        std::istringstream row(line);
        double time = 0.0;
        float target = 0.0f, dt = 0.0f;
        char c1 = 0, c2 = 0;
        if (!(row >> time >> c1 >> target >> c2 >> dt) || c1 != ',' || c2 != ',' || !std::isfinite(time))
            return EXIT_FAILURE;
        row >> std::ws;
        if (!row.eof()) return EXIT_FAILURE;
        const float x1 = td.GetOutput(), x2 = td.GetRate();
        float acceleration = 0.0f;
        if (!td.Calc(target,acceleration) || !td.Update(target,dt)) return EXIT_FAILURE;
        output << std::setprecision(17) << time << ',' << std::setprecision(9)
               << target << ',' << dt << ',' << x1 << ',' << x2 << ',' << acceleration << ','
               << td.GetOutput() << ',' << td.GetRate() << '\n';
        ++count;
    }
    output.flush();
    if (input.bad() || !output || count == 0) return EXIT_FAILURE;
    std::cout << "PASS: TD runner rows=" << count << '\n';
}
