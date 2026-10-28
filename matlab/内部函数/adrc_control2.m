function command = adrc_control2(target, z, b0, omega_c)
%ADRC_CONTROL2 第13讲：二阶LADRC的线性反馈与总扰动补偿。
% 内部公式函数；仿真请运行“第13讲_二阶LADRC闭环/run_lesson.m”。
% 对照原Motion_acc03的kp、kd、Sum3和1/b0接线；这里kd按正阻尼增益书写。
% 位置给定与零速度基准反馈；不含目标速度/加速度前馈。
% 可输入过渡目标，但会有动态跟踪误差；本函数不推进LESO，不含TD或限幅。
assert(isequal(size(z),[3 1]) && all(isfinite(z)) ...
    && all(isfinite([target b0 omega_c])) && b0 ~= 0 && omega_c > 0, ...
    'ADRC:InvalidControlParameter','检查目标、三个估计状态、b0和控制带宽参数。');
kp = omega_c^2;
kd = 2*omega_c;
u0 = kp*(target-z(1))-kd*z(2);
command = (u0-z(3))/b0;
assert(isfinite(command),'ADRC:NonfiniteControl','控制量得到非有限结果。');
end
