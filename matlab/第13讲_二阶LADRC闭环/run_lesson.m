%% 第13讲：二阶LADRC闭环——位置反馈、速度反馈与扰动补偿
% 教学归属：LADRC专题；将第11讲三阶LESO与二阶控制律按时序组合。
% 直接点击“运行”（F5），运行整个文件。对应docs/13_二阶LADRC闭环.md。
% 两组都用估计状态反馈，只比较是否启用扰动补偿；无噪声、无TD、理想无限幅输入。
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1','LADRC实现参考', ...
    'MATLABDemo-LADRC 实现(高志强老师)','LADRC Demo');
model_file = fullfile(reference_dir,'Motion_acc03.mdl');
tuning_file = fullfile(reference_dir,'tuning.m');
assert(isfile(model_file) && isfile(tuning_file),'ADRC:MissingReference', ...
    '请检查ADRC1原模型和tuning.m的位置。');
model_snapshot = fileread(model_file);
tuning_snapshot = fileread(tuning_file);
output_dir = fullfile(repo,'build','lesson13');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 读取原观测器参数、控制增益表达式与矩阵，不执行原sim调用
original = struct;
for name = {'h','b0','wo','wc'}
    token = regexp(tuning_snapshot,['(?m)^\s*' name{1} '\s*=\s*([^;]+);'],'tokens','once');
    assert(~isempty(token),'ADRC:MissingParameter','原参数未找到。');
    original.(name{1}) = str2double(strtrim(token{1}));
end
kp_token = regexp(tuning_snapshot,'(?m)(?:^|;)\s*kp\s*=\s*([^;]+);','tokens','once');
kd_token = regexp(tuning_snapshot,'(?m)(?:^|;)\s*kd\s*=\s*([^;]+);','tokens','once');
blocks = regexp(model_snapshot, ...
    'BlockType\s+StateSpace\s+Name\s+"LESO[^"]*"(.*?)\n\s*}', 'tokens');
assert(numel(blocks)==1 && ~isempty(kp_token) && ~isempty(kd_token), ...
    'ADRC:ReferenceChanged','原观测器或增益表达式发生变化。');
token_a = regexp(blocks{1}{1},'(?m)^\s*A\s+"([^"]+)"','tokens','once');
token_b = regexp(blocks{1}{1},'(?m)^\s*B\s+"([^"]+)"','tokens','once');
assert(~isempty(token_a) && ~isempty(token_b),'ADRC:MissingMatrix','原LESO矩阵未找到。');
h = original.h; b0 = original.b0; wo = original.wo;
% wc=8是本讲教学选择；原tuning的wc=150单独保留，不冒充原模型参数复现。
p = struct('h',h,'b0',b0,'omega_o',wo,'omega_c',8,'end_time',6, ...
    'target_time',0.5,'target_value',1,'load_time',2.5,'load_value',-2,'steady_start',5);
wc = p.omega_c;
kp = eval(kp_token{1}); kd_signed = eval(kd_token{1});
A_reference = eval(token_a{1}); B_reference = eval(token_b{1});
assert(all(isfinite([h b0 wo wc])) && h > 0 && b0 ~= 0 && wo > 0 && wc > 0 && wo*h < 2, ...
    'ADRC:InvalidParameter','检查参数与当前离散方式。');
assert(kp==wc^2 && kd_signed==-2*wc,'ADRC:ControlChanged', ...
    '原控制增益表达式变化，需要重新核对教学控制律。');
beta1 = 3*wo; beta2 = 3*wo^2; beta3 = wo^3;
teaching = struct('lesson','第13讲','algorithm','二阶LADRC与三阶LESO', ...
    'topic','恒定目标下的位置反馈、速度反馈与总扰动补偿', ...
    'reference_model',model_file,'reference_parameters',tuning_file, ...
    'document','docs/13_二阶LADRC闭环.md', ...
    'changes','wc改为教学值8；对象、阶跃目标和负载为教学场景；不运行原完整模型');
fprintf('\n【第13讲：二阶LADRC闭环——位置反馈、速度反馈与扰动补偿】\n');
fprintf('u=[wc²*(目标-z1)-2*wc*z2-z3]/b0；反馈和补偿整体除以b0。\n');
fprintf('h=%g秒，b0=%g，wo=%g rad/s；教学wc=%g，原例wc=%g。\n',h,b0,wo,wc,original.wc);

%% 从原矩阵与原增益表达式构造五状态离散闭环，独立核对整个更新时序
plant_transition = [1 h;0 1];
plant_input = [0.5*h^2*b0;h*b0];
plant_disturbance = [0.5*h^2;h];
open_transition = [plant_transition zeros(2,3); ...
    h*B_reference(:,2)*[1 0] eye(3)+h*A_reference];
combined_input = [plant_input;h*B_reference(:,1)];
combined_disturbance = [plant_disturbance;zeros(3,1)];
scenario_names = ["without_compensation","ladrc"];
labels = ["估计状态反馈，关闭补偿","二阶LADRC，开启补偿"];
time = (0:round(p.end_time/h))'*h;
records = zeros(numel(time)*2,15);
reference_errors = zeros(2,1);
control_residuals = zeros(2,1);
closed_loop_radii = zeros(2,1);
row = 0;
for scenario = 1:2
    compensation_enabled = scenario==2;
    feedback_row = [0 0 -kp/b0 kd_signed/b0 -double(compensation_enabled)/b0];
    closed_transition = open_transition+combined_input*feedback_row;
    reference_target_input = combined_input*(kp/b0);
    closed_loop_radii(scenario) = max(abs(eig(closed_transition)));
    assert(closed_loop_radii(scenario) < 1,'ADRC:UnstableConfiguration', ...
        '当前理想固定步长闭环矩阵不稳定。');
    y = 0; velocity = 0; z = zeros(3,1); reference_state = zeros(5,1);
    for k = 1:numel(time)
        target = p.target_value*double(k-1 >= round(p.target_time/h));
        true_f = p.load_value*double(k-1 >= round(p.load_time/h));
        measurement = y;
        control_z = z;
        if ~compensation_enabled, control_z(3) = 0; end % 只关闭控制补偿，LESO不清零。
        applied_input = adrc_control2(target,control_z,b0,wc);
        feedback_input = (kp*(target-z(1))+kd_signed*z(2))/b0;
        compensation_input = -double(compensation_enabled)*z(3)/b0;
        control_residuals(scenario) = max(control_residuals(scenario), ...
            abs(applied_input-(feedback_input+compensation_input)));
        row = row+1;
        records(row,:) = [scenario,time(k),target,y,velocity,true_f,z.', ...
            feedback_input,compensation_input,applied_input,target-y,measurement,z(3)+b0*applied_input];
        z_next = adrc_leso3_step(z,applied_input,measurement,b0,beta1,beta2,beta3,h);
        acceleration = b0*applied_input+true_f;
        y_next = y+h*velocity+0.5*h^2*acceleration;
        velocity_next = velocity+h*acceleration;
        reference_next = closed_transition*reference_state ...
            + reference_target_input*target+combined_disturbance*true_f;
        reference_errors(scenario) = max(reference_errors(scenario), ...
            max(abs([y_next;velocity_next;z_next]-reference_next)));
        if k < numel(time)
            y = y_next; velocity = velocity_next; z = z_next; reference_state = reference_next;
        end
    end
end
T = array2table(records(:,2:end),'VariableNames',{'time','target','true_y','true_velocity', ...
    'true_f','z1','z2','z3','feedback_input','compensation_input','applied_input', ...
    'tracking_error','measurement','model_acceleration'});
scenario_column = scenario_names(records(:,1));
T = addvars(T,scenario_column(:),'Before',1,'NewVariableNames','scenario');

%% 检查两种稳态：无补偿偏差=-f/kp；有补偿趋于零；两者都需抵消实际负载
steady_errors = zeros(2,1); peak_errors_after_load = zeros(2,1); steady_inputs = zeros(2,1);
for scenario = 1:2
    rows = T(T.scenario==scenario_names(scenario),:);
    steady = rows.time >= p.steady_start;
    steady_errors(scenario) = mean(rows.tracking_error(steady));
    peak_errors_after_load(scenario) = max(abs(rows.tracking_error(rows.time>=p.load_time)));
    steady_inputs(scenario) = mean(rows.applied_input(steady));
    first_target = round(p.target_time/h)+1;
    assert(abs(rows.applied_input(first_target)-kp*p.target_value/b0) < 1e-12, ...
        'ADRC:ControlDirection','零状态下正目标阶跃的首拍控制不符。');
end
expected_error_without_compensation = -p.load_value/kp;
expected_steady_input = -p.load_value/b0;
assert(abs(steady_errors(1)-expected_error_without_compensation) < 1e-7 ...
    && abs(steady_errors(2)) < 1e-7,'ADRC:SteadyError','稳态位置偏差与当前模型预期不一致。');
assert(all(abs(steady_inputs-expected_steady_input)<1e-7), ...
    'ADRC:SteadyInput','实际负载应由稳态控制输入平衡。');
assert(max(reference_errors)<1e-8 && max(control_residuals)<1e-12, ...
    'ADRC:ReferenceMismatch','控制律或完整离散闭环与参考矩阵不一致。');
assert(all(isfinite(records),'all'),'ADRC:NonfiniteResult','闭环结果存在非有限数值。');
assert(strcmp(model_snapshot,fileread(model_file)) && strcmp(tuning_snapshot,fileread(tuning_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');
metrics = table(scenario_names(:),steady_errors,peak_errors_after_load,steady_inputs,closed_loop_radii, ...
    'VariableNames',{'scenario','steady_tracking_error','peak_error_after_load','steady_input','closed_loop_radius'});
metrics_cn = table(labels(:),steady_errors,peak_errors_after_load,steady_inputs, ...
    'VariableNames',{'控制方式','稳态位置误差_rad','负载后最大误差_rad','稳态电流_A'});
disp(metrics_cn);
fprintf('验证通过：离散闭环对照最大差值%.3g，控制分解残差%.3g。\n', ...
    max(reference_errors),max(control_residuals));
writetable(T,fullfile(output_dir,'ladrc2_sim.csv'));
writetable(metrics,fullfile(output_dir,'ladrc2_metrics.csv'));
save(fullfile(output_dir,'ladrc2_result.mat'),'T','metrics','p','original','teaching', ...
    'reference_errors','control_residuals','closed_loop_radii', ...
    'expected_error_without_compensation','expected_steady_input');

%% 中文图：位置跟踪、负载后的偏差、实际输入与总扰动估计
fig = figure('Name','第13讲：二阶LADRC闭环','Color','w','Position',[70 40 1240 800]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout); hold(ax1,'on'); ax2 = nexttile(layout); hold(ax2,'on');
ax3 = nexttile(layout); hold(ax3,'on'); ax4 = nexttile(layout); hold(ax4,'on');
base_rows = T(T.scenario==scenario_names(1),:);
plot(ax1,time,base_rows.target,'k--','DisplayName','角度目标');
plot(ax4,time,base_rows.true_f,'k--','DisplayName','真实总扰动');
colors = [0.75 0.39 0.18;0.20 0.43 0.69];
for scenario = 1:2
    rows = T(T.scenario==scenario_names(scenario),:);
    plot(ax1,time,rows.true_y,'Color',colors(scenario,:),'LineWidth',1.4,'DisplayName',labels(scenario));
    plot(ax2,time,rows.tracking_error,'Color',colors(scenario,:),'LineWidth',1.4,'DisplayName',labels(scenario));
    plot(ax3,time,rows.applied_input,'Color',colors(scenario,:),'LineWidth',1.2,'DisplayName',labels(scenario));
    plot(ax4,time,rows.z3,'Color',colors(scenario,:),'LineWidth',1.2,'DisplayName',labels(scenario));
end
title(ax1,'角度目标与真实输出'); ylabel(ax1,'角度（rad）');
title(ax2,'负载后的目标跟踪误差'); ylabel(ax2,'目标－真实角度（rad）'); xlim(ax2,[2.3 4.5]);
xline(ax2,p.load_time,'k:','HandleVisibility','off');
title(ax3,'实际作用于对象的输入'); ylabel(ax3,'电流（A）');
title(ax4,'三阶LESO的总扰动估计 z3'); ylabel(ax4,'总扰动（rad/s²）');
for ax = [ax1 ax2 ax3 ax4]
    xlabel(ax,'时间（秒）'); grid(ax,'on'); legend(ax,'show','Location','best','FontSize',8);
end
title(layout,'第13讲｜二阶LADRC闭环：估计状态反馈与扰动补偿（无噪声、理想无限幅输入）');
adrc_export_figure(fig,output_dir,"ladrc2_response");
