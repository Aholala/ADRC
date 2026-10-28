function command = adrc_control(target, z, b0, omega_c)
% 这是需要输入参数的内部函数；仿真请运行“第07讲_LADRC与LESO初始化/run_lesson.m”。
%ADRC_CONTROL 一阶 LADRC：线性状态反馈 + 总扰动补偿。
% 参考 ADRC1/MyLibrary/ADRC_1.m:38-40，明确使用 (u0-z2)/b0 的单位约定。
% 本函数不推进 ESO，不包含 TD、限幅或任务逻辑。
assert(isequal(size(z),[2 1]) && all(isfinite(z)) ...
    && all(isfinite([target b0 omega_c])) && b0 ~= 0 && omega_c > 0, ...
    'ADRC:InvalidControlParameter', '检查状态、目标与控制增益。');
u0 = omega_c*(target-z(1));
command = (u0-z(2))/b0;
assert(isfinite(command), 'ADRC:NonfiniteControl', '控制量得到非有限结果。');
end
