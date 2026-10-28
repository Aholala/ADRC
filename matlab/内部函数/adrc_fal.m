function value = adrc_fal(error, alpha, delta)
%ADRC_FAL 第25讲：原ESO2_NL/ESO_2rd中的分段幂函数，支持实浮点数组。
% 静态映射，不保存状态，不是独立仿真入口；请运行“第25讲_fal与非线性ESO入门”。
% 原阈值保留abs(error)<delta，等号使用外侧幂函数。
% 第28讲起共享函数支持alpha>0（包括反馈中的1.5）；delta与error使用相同尺度。
assert(isfloat(error) && isreal(error) && all(isfinite(error),'all'), ...
    'ADRC:InvalidFalError','误差必须是有限实浮点数组。');
assert(isnumeric(alpha) && isreal(alpha) && isscalar(alpha) && isfinite(alpha) ...
    && alpha > 0 && isnumeric(delta) && isreal(delta) ...
    && isscalar(delta) && isfinite(delta) && delta > 0, ...
    'ADRC:InvalidFalParameter','要求alpha>0、delta>0，参数为有限实标量。');
inner_gain = delta^(alpha-1);
assert(isfinite(inner_gain) && inner_gain > 0,'ADRC:FalScale','小误差区斜率不可表示。');
value = zeros(size(error),'like',error);
inside = abs(error) < delta;
value(inside) = error(inside).*inner_gain;
value(~inside) = abs(error(~inside)).^alpha.*sign(error(~inside));
assert(all(isfinite(value),'all'),'ADRC:NonfiniteFal','fal结果存在非有限数值。');
end
