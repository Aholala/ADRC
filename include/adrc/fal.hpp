#ifndef ADRC_FAL_HPP
#define ADRC_FAL_HPP
namespace adrc
{
    // 原分段幂函数，alpha>0、delta>0。失败保持value；无时间状态。
    bool Fal(float error, float alpha, float delta, float& value);
}
#endif
