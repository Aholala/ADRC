#include "adrc/adrc.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace
{
    float Parse(const char* value)
    {
        char* end = nullptr;
        const float result = std::strtof(value, &end);
        if (end == value || *end != '\0' || !std::isfinite(result))
        {
            std::cerr << "Invalid numeric argument\n";
            std::exit(EXIT_FAILURE);
        }
        return result;
    }
}

int main(int argc, char** argv)
{
    if (argc != 7)
    {
        std::cerr << "Usage: reduced_leso_runner input.csv output.csv b0 omega_o measurement0 disturbance0\n";
        return EXIT_FAILURE;
    }
    adrc::RLESO1 eso;
    if (!eso.Init(Parse(argv[3]), Parse(argv[4]), Parse(argv[5]), Parse(argv[6])))
        return EXIT_FAILURE;
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    if (!input || !output)
        return EXIT_FAILURE;
    std::string line;
    if (!std::getline(input, line))
        return EXIT_FAILURE;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "time,input,measurement,dt")
        return EXIT_FAILURE;
    output << "time,input,measurement,dt,q_before,f_hat,q_after\n";
    unsigned int rows = 0;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        double time = 0.0;
        float applied = 0.0f, measurement = 0.0f, dt = 0.0f;
        char c1 = 0, c2 = 0, c3 = 0;
        if (!(row >> time >> c1 >> applied >> c2 >> measurement >> c3 >> dt) ||
            c1 != ',' || c2 != ',' || c3 != ',' || !std::isfinite(time))
            return EXIT_FAILURE;
        row >> std::ws;
        if (!row.eof()) return EXIT_FAILURE;
        const float before = eso.GetAuxiliaryState();
        float estimate = 0.0f;
        // 先读本拍估计；Update只推进一次，之后等待下一拍新测量。
        if (!eso.Calc(measurement, estimate) || !eso.Update(applied, measurement, dt))
            return EXIT_FAILURE;
        output << std::setprecision(std::numeric_limits<double>::max_digits10) << time << ','
               << std::setprecision(std::numeric_limits<float>::max_digits10)
               << applied << ',' << measurement << ',' << dt << ','
               << before << ',' << estimate << ',' << eso.GetAuxiliaryState() << '\n';
        ++rows;
    }
    output.flush();
    if (input.bad() || !output || rows == 0)
        return EXIT_FAILURE;
    std::cout << "PASS: reduced LESO runner rows=" << rows << '\n';
}
