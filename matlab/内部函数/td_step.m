function [x_next, acceleration] = td_step(x, input, r, h, h0)
%TD_STEP 第9讲：非线性二阶跟踪微分器（TD）的一拍更新。
% 这是需要输入参数的内部函数；仿真请运行“第09讲_非线性TD/run_lesson.m”。
% 对应ADRC1/MyLibrary/TD2_NL.m:33-35。h是实际步长，h0只用于fhan。
% x(1)为平滑值，x(2)为变化率；两条更新都使用同一拍旧状态。
assert(isequal(size(x),[2 1]) && all(isfinite(x)) ...
    && all(isfinite([input r h h0])) && r > 0 && h > 0 && h0 > 0, ...
    'ADRC:InvalidTD','检查状态、输入以及r、h、h0的正值条件。');
acceleration = td_fhan(x(1)-input,x(2),r,h0);
x_next = [x(1)+h*x(2); x(2)+h*acceleration];
assert(all(isfinite(x_next)),'ADRC:NonfiniteTD','非线性TD得到非有限状态。');
end
