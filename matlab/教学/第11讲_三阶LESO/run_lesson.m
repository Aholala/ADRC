%% 第11讲：二阶对象的三阶LESO——角度、角速度与总扰动
% 教学归属：LESO专题；本讲不接TD和LADRC控制律。
% 直接点击“运行”（F5），运行整个文件，无需填写函数参数。
% 对应讲义：docs/11_三阶LESO.md。
repo = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1','LADRC实现参考', ...
    'MATLABDemo-LADRC 实现(高志强老师)','LADRC Demo');
model_file = fullfile(reference_dir,'Motion_acc03.mdl');
tuning_file = fullfile(reference_dir,'tuning.m');
assert(isfile(model_file) && isfile(tuning_file),'ADRC:MissingReference', ...
    '请检查ADRC1原模型和tuning.m的位置。');
model_snapshot = fileread(model_file);
tuning_snapshot = fileread(tuning_file);
output_dir = fullfile(repo,'build','lesson11');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 读取原观测器矩阵与h、b0、wo；不执行原tuning中的sim调用
parameter_names = {'h','b0','wo'};
original = struct;
for index = 1:numel(parameter_names)
    name = parameter_names{index};
    token = regexp(tuning_snapshot,['(?m)^\s*' name '\s*=\s*([^;]+);'],'tokens','once');
    assert(~isempty(token),'ADRC:MissingParameter','原参数未找到。');
    original.(name) = str2double(strtrim(token{1}));
end
h = original.h; b0 = original.b0; wo = original.wo;
assert(all(isfinite([h b0 wo])) && h > 0 && b0 ~= 0 && wo > 0 && wo*h < 2, ...
    'ADRC:InvalidReference','检查原参数和当前离散方式的稳定范围。');
blocks = regexp(model_snapshot, ...
    'BlockType\s+StateSpace\s+Name\s+"LESO[^"]*"(.*?)\n\s*}', 'tokens');
assert(numel(blocks)==1,'ADRC:ReferenceChanged','需要重新核对原LESO块。');
token_a = regexp(blocks{1}{1},'(?m)^\s*A\s+"([^"]+)"','tokens','once');
token_b = regexp(blocks{1}{1},'(?m)^\s*B\s+"([^"]+)"','tokens','once');
assert(~isempty(token_a) && ~isempty(token_b),'ADRC:MissingMatrix','原矩阵未找到。');
A_reference = eval(token_a{1}); % 仅求值用户指定原模型的矩阵表达式。
B_reference = eval(token_b{1});
assert(isequal(size(A_reference),[3 3]) && isequal(size(B_reference),[3 2]), ...
    'ADRC:MatrixShape','原LESO矩阵维度变化。');

% h、b0、wo来自原tuning；下面的对象、输入、扰动、时长是新增教学场景。
p = struct('h',h,'b0',b0,'omega_o',wo,'end_time',3, ...
    'input_time',0.5,'input_value',0.075,'load_time',1.5,'load_value',-2);
p.beta1 = 3*wo; p.beta2 = 3*wo^2; p.beta3 = wo^3;
teaching = struct('lesson','第11讲','algorithm','二阶对象的三阶全阶LESO', ...
    'topic','角度、角速度与总扰动的三个估计状态', ...
    'reference_model',model_file,'reference_parameters',tuning_file, ...
    'document','docs/11_三阶LESO.md', ...
    'scenario_source','新增的无噪声开环角度对象，非原完整闭环模型');
fprintf('\n【第11讲：二阶对象的三阶LESO——角度、角速度与总扰动】\n');
fprintf('z1估计角度，z2估计角速度，z3估计总扰动；模型角加速度估计=z3+b0*u。\n');
fprintf('原参数：h=%g秒，b0=%g，wo=%g rad/s；教学场景持续%g秒。\n',h,b0,wo,p.end_time);

%% 对象精确ZOH更新；LESO保留原ode1对应的前向欧拉更新
time = (0:round(p.end_time/h))'*h;
y = 0; velocity = 0;
z = zeros(3,1); reference_z = zeros(3,1);
records = zeros(numel(time),15);
reference_max_error = 0;
for k = 1:numel(time)
    applied_input = p.input_value*double(k-1 >= round(p.input_time/h));
    true_f = p.load_value*double(k-1 >= round(p.load_time/h));
    measurement = y; % 只给观测器角度测量，不传真实角速度和真实扰动。
    true_acceleration = b0*applied_input+true_f;
    model_acceleration = b0*applied_input+z(3);
    z_next = adrc_leso3_step(z,applied_input,measurement,b0,p.beta1,p.beta2,p.beta3,h);
    reference_next = reference_z+h*(A_reference*reference_z ...
        + B_reference*[applied_input;measurement]);
    reference_max_error = max(reference_max_error,max(abs(z_next-reference_next)));
    records(k,:) = [time(k),applied_input,y,velocity,measurement,true_f, ...
        true_acceleration,z(1),z(2),z(3),model_acceleration, ...
        z_next(1),z_next(2),z_next(3),z(2)-velocity];
    if k < numel(time)
        y = y+h*velocity+0.5*h^2*true_acceleration;
        velocity = velocity+h*true_acceleration;
        z = z_next;
        reference_z = reference_next;
    end
end
T = array2table(records,'VariableNames',{'time','applied_input','true_y', ...
    'true_velocity','measurement','true_f','true_acceleration','z1','z2','z3', ...
    'model_acceleration','z1_next','z2_next','z3_next','velocity_error'});

%% 核对对象解析解、原矩阵、时间标签，以及固定步长下的末态偏差
tau_input = max(time-p.input_time,0);
tau_load = max(time-p.load_time,0);
analytic_y = 0.5*b0*p.input_value*tau_input.^2+0.5*p.load_value*tau_load.^2;
analytic_velocity = b0*p.input_value*tau_input+p.load_value*tau_load;
plant_max_error = max([abs(T.true_y-analytic_y); abs(T.true_velocity-analytic_velocity)]);
assert(plant_max_error < 1e-10,'ADRC:PlantMismatch','对象与分段解析解不一致。');
assert(reference_max_error < 1e-8,'ADRC:ReferenceMismatch','提取公式与原矩阵不一致。');
assert(isequal(T{1:end-1,{'z1_next','z2_next','z3_next'}},T{2:end,{'z1','z2','z3'}}), ...
    'ADRC:TimeLabelMismatch','下一拍预测与下一行状态不对应。');
assert(abs(T.z3(end)-p.load_value) < 1e-8,'ADRC:DisturbanceTracking','末态扰动估计异常。');
assert(abs(T.z1(end)-T.true_y(end)) < 1e-9,'ADRC:OutputTracking','末态角度估计异常。');
% 对象用精确ZOH，观测器用欧拉：恒加速度段的稳态速度偏差为h*a/2。
expected_velocity_bias = 0.5*h*T.true_acceleration(end);
assert(abs(T.velocity_error(end)-expected_velocity_bias) < 1e-9, ...
    'ADRC:DiscretizationBias','末段速度偏差与当前两种离散方式的解析预期不一致。');
assert(all(isfinite(records),'all'),'ADRC:NonfiniteResult','出现非有限数值。');
assert(strcmp(model_snapshot,fileread(model_file)) && strcmp(tuning_snapshot,fileread(tuning_file)), ...
    'ADRC:ReferenceChanged','参考文件发生变化。');
writetable(T,fullfile(output_dir,'leso3_sim.csv'));
save(fullfile(output_dir,'leso3_result.mat'),'T','p','teaching','original', ...
    'A_reference','B_reference','reference_max_error','plant_max_error','expected_velocity_bias');
fprintf('验证通过：%d拍原矩阵对照最大差值%.3g；对象解析解差值%.3g。\n', ...
    numel(time),reference_max_error,plant_max_error);
fprintf('末态：z3=%.9g，模型角加速度=%.9g；速度离散偏差=%.9g rad/s（预期%.9g）。\n', ...
    T.z3(end),T.model_acceleration(end),T.velocity_error(end),expected_velocity_bias);

%% 中文图：三个估计状态，以及已知输入与总扰动共同形成的加速度
fig = figure('Name','第11讲：三阶LESO——角度、角速度与总扰动', ...
    'Color','w','Position',[70 40 1220 800]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout);
plot(ax1,time,T.true_y,'k-',time,T.z1,'--','LineWidth',1.4);
title(ax1,'z1估计角度'); ylabel(ax1,'角度（rad）');
legend(ax1,'真实角度 y','角度估计 z1','Location','northwest');
ax2 = nexttile(layout);
plot(ax2,time,T.true_velocity,'k-',time,T.z2,'--','LineWidth',1.4);
title(ax2,'z2估计角速度'); ylabel(ax2,'角速度（rad/s）');
legend(ax2,'真实角速度','角速度估计 z2','Location','northwest');
ax3 = nexttile(layout);
plot(ax3,time,T.true_f,'k--',time,T.z3,'LineWidth',1.4);
title(ax3,'z3估计总扰动'); ylabel(ax3,'总扰动（rad/s²）');
legend(ax3,'真实总扰动 f','总扰动估计 z3','Location','southwest');
ax4 = nexttile(layout);
plot(ax4,time,T.true_acceleration,'k--',time,T.model_acceleration,'LineWidth',1.4);
title(ax4,'角加速度的模型估计：z3+b0*u'); ylabel(ax4,'角加速度（rad/s²）');
legend(ax4,'真实角加速度','模型角加速度估计','Location','southwest');
for ax = [ax1 ax2 ax3 ax4]
    xlabel(ax,'时间（秒）'); grid(ax,'on');
end
title(layout,'第11讲｜二阶对象的三阶LESO：角度、角速度、总扰动（无噪声开环实验）');
adrc_export_figure(fig,output_dir,"leso3_response");
