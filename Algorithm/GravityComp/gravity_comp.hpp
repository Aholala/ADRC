#ifndef __GRAVITY_COMP_HPP
#define __GRAVITY_COMP_HPP

#include "user_maths.hpp"

namespace Alg_n
{
    class GravityComp_c
    {
        float t_pitch_;
        float t_yaw_;

    public:
        GravityComp_c(float mg, float cgx, float cgy, float cgz, float d);
        void Calc(float motor_pitch, float pitch, float roll);

    private:
        float mg_;
        float x_, y_, z_;
        float d_;
    };
}

#endif //! __GRAVITY_COMP_HPP
