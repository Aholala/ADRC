%% 第10讲：一阶对象的二阶LESO——状态含义与采样时序
% 教学归属：LESO专题；本讲只观察观测器，不接LADRC控制律或TD。
% 直接点击“运行”（F5），运行整个文件，无需填写函数参数。
% 对应讲义：docs/10_LESO状态与时序.md。
repo = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file = fullfile(reference_dir,'ADRC_1.m');
assert(isfile(reference_file),'ADRC:MissingReference','请检查ADRC1的ADRC_1.m参考文件。');
source_snapshot = fileread(reference_file);
output_dir = fullfile(repo,'build','lesson10');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 教学参数：沿用已有一阶对象的单位，不复制原完整模型的配置
p = struct('b0',10,'omega_o',20,'h',0.001,'end_time',3, ...
    'input_time',0.5,'input_value',0.3,'load_time',1.5,'load_value',-2);
p.beta1 = 2*p.omega_o;
p.beta2 = p.omega_o^2;
assert(p.omega_o*p.h > 0 && p.omega_o*p.h < 2, ...
    'ADRC:InvalidStep','当前前向欧拉配置超出理想误差系统稳定范围。');
time = (0:round(p.end_time/p.h))'*p.h;
input_sample = round(p.input_time/p.h);
load_sample = round(p.load_time/p.h);
teaching = struct('lesson','第10讲','algorithm','一阶对象的二阶全阶LESO', ...
    'topic','z1、z2的含义，以及k拍到k+1拍的预测时序', ...
    'reference_code',reference_file,'document','docs/10_LESO状态与时序.md', ...
    'parameter_source','本讲教学假设，非ADRC1完整模型或实际机器人参数');
fprintf('\n【第10讲：一阶对象的二阶LESO——状态含义与采样时序】\n');
fprintf('对象：速度变化率=b0*u+f；z1估计速度，z2估计总扰动。\n');
fprintf('教学参数：b0=%g，带宽参数=%g rad/s，实际步长=%g秒。\n',p.b0,p.omega_o,p.h);
fprintf('已知输入在%g秒变为%g A；未知总扰动在%g秒变为%g rad/s²。\n', ...
    p.input_time,p.input_value,p.load_time,p.load_value);

%% 临时调用原ADRC_1的flag=2，只比较其中第3、4个LESO状态
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(reference_dir,'-begin');
assert(strcmpi(which('ADRC_1'),reference_file),'ADRC:WrongReference', ...
    '当前ADRC_1不是指定参考文件。');
y = 0;
z = [0;0];
reference_x = zeros(4,1);
records = zeros(numel(time),11);
reference_max_error = 0;
for k = 1:numel(time)
    applied_input = p.input_value*double(k-1 >= input_sample);
    true_f = p.load_value*double(k-1 >= load_sample);
    measurement = y; % 本讲无噪声，隔离状态含义与时序。
    true_ydot = p.b0*applied_input+true_f;
    model_ydot = p.b0*applied_input+z(2);

    % 用k拍测量和区间输入，从z[k]预测到z[k+1]。
    z_next = adrc_eso_step(z,applied_input,measurement,p.b0,p.beta1,p.beta2,p.h);
    % 原u(3)接收模型作用量，本讲传b0*u；TD保持零状态，不使用控制输出分支。
    reference_next = ADRC_1(time(k),reference_x,[0;measurement;p.b0*applied_input], ...
        2,1,p.h,p.beta1,p.beta2,1);
    reference_next = reference_next(:);
    reference_max_error = max(reference_max_error,max(abs(z_next-reference_next(3:4))));

    % 当前真值和当前估计放在同一行；下一拍预测单独命名。
    records(k,:) = [time(k),applied_input,y,measurement,true_f,true_ydot, ...
        z(1),z(2),model_ydot,z_next(1),z_next(2)];
    if k < numel(time)
        y = y+p.h*true_ydot; % 区间内u、f恒定，因此是对象的精确更新。
        z = z_next;
        reference_x = reference_next;
    end
end
assert(strcmp(source_snapshot,fileread(reference_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');
clear restore_path;
T = array2table(records,'VariableNames',{'time','applied_input','true_y', ...
    'measurement','true_f','true_ydot','z1','z2','model_ydot','z1_next','z2_next'});

%% 独立核对：对象解析解、离散误差解析解、突变因果性与时间标签
true_y_analytic = p.b0*p.input_value*max(time-p.input_time,0) ...
    + p.load_value*max(time-p.load_time,0);
plant_max_error = max(abs(T.true_y-true_y_analytic));
assert(plant_max_error < 1e-10,'ADRC:PlantMismatch','对象与分段解析解不一致。');
after_load = load_sample+1:numel(time);
n = (0:numel(after_load)-1)';
pole = 1-p.omega_o*p.h;
expected_eta1 = -p.load_value*n*p.h.*pole.^(n-1);
expected_z2 = p.load_value*(1-pole.^n-n*p.omega_o*p.h.*pole.^(n-1));
analytic_max_error = max([abs(T.z1(after_load)-T.true_y(after_load)-expected_eta1); ...
    abs(T.z2(after_load)-expected_z2)]);
assert(analytic_max_error < 1e-10,'ADRC:AnalyticMismatch','LESO与离散误差解析解不一致。');
assert(reference_max_error < 1e-10,'ADRC:ReferenceMismatch','LESO与原ADRC_1更新不一致。');
assert(max(abs(T.z1_next(1:end-1)-T.z1(2:end))) == 0 ...
    && max(abs(T.z2_next(1:end-1)-T.z2(2:end))) == 0, ...
    'ADRC:TimeLabelMismatch','下一拍预测没有对应到下一行状态。');
assert(max(abs(T.z2(1:load_sample+2))) < 1e-10, ...
    'ADRC:Causality','当前理想实验不能在输出显现前知道未知扰动。');
assert(T.z2(load_sample+3) < 0 && abs(T.z2(end)-p.load_value) < 1e-9, ...
    'ADRC:DisturbanceTracking','扰动显现后的纠偏方向或末态异常。');
assert(all(isfinite(records),'all'),'ADRC:NonfiniteResult','实验结果存在非有限数值。');

%% 输出：扰动前后一拍也单独保存，便于手算与检查时间标签
timing_rows = load_sample+(0:3);
timing = T(timing_rows,{'time','true_y','true_f','z1','z2','z1_next','z2_next'});
timing_cn = table(timing.time,timing.true_y,timing.true_f,timing.z1,timing.z2, ...
    'VariableNames',{'时间_s','真实速度_rad_s','真实总扰动_rad_s2','当前速度估计_z1','当前扰动估计_z2'});
disp(timing_cn);
writetable(T,fullfile(output_dir,'leso_sim.csv'));
writetable(timing,fullfile(output_dir,'leso_timing.csv'));
save(fullfile(output_dir,'leso_result.mat'),'T','p','teaching','timing', ...
    'reference_max_error','analytic_max_error','plant_max_error');
fprintf('验证通过：原函数最大差值%.3g，离散误差解析解最大差值%.3g。\n', ...
    reference_max_error,analytic_max_error);
fprintf('末态：z2=%.9g，真实总扰动=%g；速度变化率估计=%.9g。\n', ...
    T.z2(end),T.true_f(end),T.model_ydot(end));

%% 中文教学图：看清状态、总扰动、完整变化率和纠偏发生的先后
fig = figure('Name','第10讲：二阶LESO的状态含义与采样时序', ...
    'Color','w','Position',[70 40 1220 800]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout);
plot(ax1,time,T.true_y,'k-',time,T.z1,'--','LineWidth',1.4);
title(ax1,'z1估计对象速度'); ylabel(ax1,'角速度（rad/s）');
legend(ax1,'真实速度 y','速度估计 z1','Location','northwest');
ax2 = nexttile(layout);
plot(ax2,time,T.true_f,'k--',time,T.z2,'LineWidth',1.4);
title(ax2,'z2估计总扰动'); ylabel(ax2,'总扰动（rad/s²）');
legend(ax2,'真实总扰动 f','总扰动估计 z2','Location','southwest');
ax3 = nexttile(layout);
plot(ax3,time,T.true_ydot,'k--',time,T.model_ydot,'LineWidth',1.4);
title(ax3,'模型变化率估计：z2+b0*u'); ylabel(ax3,'角加速度（rad/s²）');
legend(ax3,'真实速度变化率','模型变化率估计','Location','southwest');
ax4 = nexttile(layout);
plot(ax4,time,T.z1-T.true_y,'LineWidth',1.4);
xlim(ax4,[p.load_time-0.02,p.load_time+0.3]);
xline(ax4,p.load_time,'k:','HandleVisibility','off');
title(ax4,'扰动先改变对象，输出偏差随后驱动纠偏'); ylabel(ax4,'速度估计误差 z1-y（rad/s）');
for ax = [ax1 ax2 ax3 ax4]
    grid(ax,'on'); xlabel(ax,'时间（秒）');
end
title(layout,'第10讲｜一阶对象的二阶LESO：当前状态与下一拍预测（无噪声开环实验）');
adrc_export_figure(fig,output_dir,"leso_response");
