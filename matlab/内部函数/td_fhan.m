function acceleration = td_fhan(error, rate, r, h0)
%TD_FHAN 第9讲：非线性TD的加速、制动与小范围连续修正。
% 这是需要输入参数的内部函数；仿真请运行“第09讲_非线性TD/run_lesson.m”。
% 逐式对应ADRC1/MyLibrary/TD2_NL.m:41-56；内部h对应外部h0。
% error=x1-输入，rate=x2；输出是TD内部变化率的变化率，不是电机控制命令。
assert(all(isfinite([error rate r h0])) && r > 0 && h0 > 0, ...
    'ADRC:InvalidFhan','fhan要求有限输入、r>0且h0>0。');
d = r*h0;
d0 = h0*d;
y = error+h0*rate;
a0 = sqrt(d^2+8*r*abs(y));
assert(isfinite(d) && d > 0 && isfinite(a0), ...
    'ADRC:FhanScale','fhan中间量溢出或时间尺度过小。');
if abs(y) > d0
    a = rate+(a0-d)*sign(y)/2;
else
    a = rate+y/h0;
end
if abs(a) > d
    acceleration = -r*sign(a);
else
    acceleration = -r*a/d;
end
assert(isfinite(acceleration),'ADRC:FhanNonfinite','fhan得到非有限输出。');
end
