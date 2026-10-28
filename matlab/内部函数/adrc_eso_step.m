function z_next = adrc_eso_step(z, input, measurement, b0, beta1, beta2, dt)
% 这是需要输入参数的内部函数；独立观测器实验请运行“第10讲_二阶LESO状态与时序/run_lesson.m”。
%ADRC_ESO_STEP 一阶对象的二阶全阶线性 ESO，一步前向欧拉。
% 参考 ADRC1/MyLibrary/ADRC_1.m:34-36：x(3)->z1，x(4)->z2。
% 保留 e=估计-测量；将原例单位输入增益扩展为显式 b0。
% z 为 k 拍估计，返回 k+1 拍预测。参数精度由调用方决定。
assert(isequal(size(z),[2 1]) && all(isfinite(z)), 'ADRC:InvalidState', ...
    'z 必须是两个有限状态构成的列向量。');
assert(all(isfinite([input measurement b0 beta1 beta2 dt])) ...
    && b0 ~= 0 && beta1 > 0 && beta2 > 0 && dt > 0, ...
    'ADRC:InvalidParameter', '检查输入、增益与采样周期。');
e = z(1)-measurement;
z_next = [z(1)+dt*(z(2)+b0*input-beta1*e); ...
          z(2)+dt*(-beta2*e)];
assert(all(isfinite(z_next)), 'ADRC:NonfiniteState', 'ESO 得到非有限结果。');
end
