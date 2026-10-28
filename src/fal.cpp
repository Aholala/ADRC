#include "adrc/fal.hpp"
#include <cmath>
namespace adrc
{
    bool Fal(float error, float alpha, float delta, float& value)
    {
        if (!std::isfinite(error) || !std::isfinite(alpha) || alpha <= 0.0f ||
            !std::isfinite(delta) || delta <= 0.0f) return false;
        float result;
        if (std::fabs(error) < delta)
        {
            const float gain = std::pow(delta, alpha-1.0f);
            if (!std::isfinite(gain) || gain <= 0.0f) return false;
            result = error * gain;
        }
        else
            result = std::pow(std::fabs(error), alpha) * (error > 0.0f ? 1.0f : -1.0f);
        if (!std::isfinite(result)) return false;
        value = result;
        return true;
    }
}
