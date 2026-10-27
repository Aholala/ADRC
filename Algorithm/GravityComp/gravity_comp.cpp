/******************************** ********************************
 * @file gravity_comp.cpp
 * @author L_Zero
 * @date 2026-03-28
 * @brief 三维重补代码实现
 *
 * @note 平常，云台的重力会产生pitch扭矩
 *       在重心有y轴偏移、云台有roll偏时，云台的重力会产生yaw扭矩
 ******************************** ********************************
 * @attention 采用西北天坐标系，注意pitch抬头为负
 ******************************** ********************************/

#include "gravity_comp.hpp"

namespace Alg_n
{
    // mg  云台重力
    // cgx 云台重心相对pitch转轴中心x轴偏移
    // cgy 云台重心相对pitch转轴中心y轴偏移
    // cgz 云台重心相对pitch转轴中心z轴偏移
    // d   云台pitch转轴直线到yaw转轴直线距离
    //     即 以pitch为零点 x轴负方向为正
    GravityComp_c::GravityComp_c(float mg, float cgx, float cgy, float cgz, float d)
            : t_pitch_(0.0f),
              t_yaw_(0.0f),
              mg_(mg),
              x_(cgx),
              y_(cgy),
              z_(cgz),
              d_(d)
    {}

    // motor_pitch  电机pitch角度、以垂直yaw转轴直线为0
    // pitch 云台系相对世界系pitch
    // roll  云台系相对世界系roll
    void GravityComp_c::Calc(float motor_pitch, float pitch, float roll)
    {
        // 预处理
        user_maths_c math;
        float sp = math.sin_calculate(pitch);
        float cp = math.cos_calculate(pitch);
        float sr = math.sin_calculate(roll);
        float cr = math.cos_calculate(roll);
        float y_sp = math.sin_calculate(motor_pitch);
        float y_cp = math.cos_calculate(motor_pitch);

        // 重力在云台系表示
        float Fx = -mg_ * sp;
        float Fy =  mg_ * cp * sr;
        float Fz =  mg_ * cp * cr;

        // pitch力矩
        t_pitch_ = -(z_ * mg_ * sp + x_ * mg_ * cp * cr);

        // 重心在yaw轴系表示
        float Px = y_cp * x_ + y_sp * z_ + d_;
        float Py = y_;

        // 重力在yaw轴系表示
        float Fx_y = y_cp * Fx + y_sp * Fz;

        // yaw力矩
        t_yaw_ = Px * Fy - Py * Fx_y;
    }
}
