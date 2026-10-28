function x_next = ltd_step(x, input, r, h)
% 这是需要输入参数的内部函数；仿真请运行“第08讲_线性TD/run_lesson.m”。
%LTD_STEP 线性二阶跟踪微分器（TD）的一拍状态更新。
% 对应ADRC1/MyLibrary/TD2_L.m第33～35行，两个新状态都使用旧状态。
% x(1)：平滑信号；x(2)：平滑信号的一阶变化率。
assert(isequal(size(x),[2 1]) && all(isfinite(x)) ...
    && all(isfinite([input r h])) && r > 0 && h > 0 && r*h < 2, ...
    'ADRC:InvalidLTD','检查状态、输入以及线性TD参数，要求0<r*h<2。');
x_next = [x(1)+h*x(2); ...
          x(2)+h*(-r^2*(x(1)-input)-2*r*x(2))];
assert(all(isfinite(x_next)),'ADRC:NonfiniteTD','线性TD得到非有限结果。');
end
