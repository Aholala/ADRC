function [q_next, f_hat] = leso_reduced_step(q, input, measurement, b0, omega_o, dt)
%LESO_REDUCED_STEP 第20讲：一阶对象的降阶线性ESO，前向欧拉一步。
% 对象：y_dot = b0*u + f；只积分辅助状态q。
% f_hat是使用q[k]与y_meas[k]重构的本拍估计，q_next是下一拍内部状态。
% 下一拍扰动估计还需要下一拍测量，不能把q_next+omega_o*y_meas[k]标为f_hat[k+1]。
% 初值：q[0]=期望初始扰动估计-omega_o*初始测量。
% 这是公式函数，请运行“第20讲_一阶对象的降阶LESO/run_lesson.m”。
values = {q,input,measurement,b0,omega_o,dt};
assert(all(cellfun(@(v) isnumeric(v) && isreal(v) && isscalar(v) && isfinite(v),values)), ...
    'ADRC:InvalidParameter','状态、输入、测量与参数必须是有限实数标量。');
assert(b0 ~= 0 && omega_o > 0 && dt > 0 && omega_o*dt < 2, ...
    'ADRC:InvalidStep','要求b0非零、带宽和周期为正，且0<omega_o*dt<2。');
f_hat = q + omega_o*measurement;
q_next = q - dt*omega_o*(f_hat+b0*input);
assert(isfinite(f_hat) && isfinite(q_next), ...
    'ADRC:NonfiniteState','降阶LESO得到非有限结果。');
end
