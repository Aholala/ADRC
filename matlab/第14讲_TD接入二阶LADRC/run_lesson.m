%% 第14讲：TD接入二阶LADRC——平滑目标、控制峰值与到位时间
% 教学归属：TD与LADRC组合；仅将线性TD的v1接入位置给定，不改变控制律。
% 直接点击“运行”（F5），运行整个文件。对应docs/14_TD接入二阶LADRC.md。
% v2只记录观察，不接速度前馈；无噪声、理想无限幅输入。
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
addpath(fullfile(repo,'matlab','内部函数'));
reference_root = 'D:/ACE/ADRC/ADRC1';
reference_dir = fullfile(reference_root,'LADRC实现参考', ...
    'MATLABDemo-LADRC 实现(高志强老师)','LADRC Demo');
td_dir = fullfile(reference_root,'贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
model_file = fullfile(reference_dir,'Motion_acc03.mdl');
tuning_file = fullfile(reference_dir,'tuning.m');
td_file = fullfile(td_dir,'TD2_L.m');
td_model_file = fullfile(td_dir,'MODEL','TD2_L_MODEL.mdl');
assert(all(isfile(string({model_file,tuning_file,td_file,td_model_file}))), ...
    'ADRC:MissingReference','请检查ADRC1的控制模型、参数和线性TD文件。');
model_snapshot = fileread(model_file); tuning_snapshot = fileread(tuning_file);
td_snapshot = fileread(td_file); td_model_snapshot = fileread(td_model_file);
output_dir = fullfile(repo,'build','lesson14');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 控制器沿用第13讲；TD的频率参数来自第8讲的原模型
original = struct;
for name = {'h','b0','wo','wc'}
    token = regexp(tuning_snapshot,['(?m)^\s*' name{1} '\s*=\s*([^;]+);'],'tokens','once');
    assert(~isempty(token),'ADRC:MissingParameter','原控制参数未找到。');
    original.(name{1}) = str2double(strtrim(token{1}));
end
td_mask = regexp(td_model_snapshot,'MaskValueString\s+"([^|"]+)\|([^"]+)"','tokens','once');
assert(~isempty(td_mask),'ADRC:MissingTDParameter','原线性TD参数未找到。');
original.td_r = str2double(td_mask{1}); original.td_h = str2double(td_mask{2});
h = original.h; b0 = original.b0; wo = original.wo;
p = struct('h',h,'b0',b0,'omega_o',wo,'omega_c',8,'td_r',original.td_r, ...
    'end_time',6,'target_time',0.5,'target_value',1,'load_time',2.5,'load_value',-2, ...
    'nominal_window_end',2.3,'settling_fraction',0.02,'steady_start',5);
wc = p.omega_c;
assert(all(isfinite([h b0 wo wc p.td_r original.td_h])) && b0 ~= 0 && h > 0 ...
    && wo > 0 && wc > 0 && p.td_r > 0 && max(wo,p.td_r)*h < 2, ...
    'ADRC:InvalidParameter','检查实际步长、观测器与TD参数。');
blocks = regexp(model_snapshot, ...
    'BlockType\s+StateSpace\s+Name\s+"LESO[^"]*"(.*?)\n\s*}', 'tokens');
assert(numel(blocks)==1,'ADRC:ReferenceChanged','原LESO块结构变化。');
token_a = regexp(blocks{1}{1},'(?m)^\s*A\s+"([^"]+)"','tokens','once');
token_b = regexp(blocks{1}{1},'(?m)^\s*B\s+"([^"]+)"','tokens','once');
assert(~isempty(token_a) && ~isempty(token_b),'ADRC:MissingMatrix','原矩阵未找到。');
A_reference = eval(token_a{1}); B_reference = eval(token_b{1});
teaching = struct('lesson','第14讲','algorithm','线性TD与二阶LADRC、三阶LESO', ...
    'topic','只改变位置给定，比较原始目标的到位时间与控制峰值', ...
    'reference_model',model_file,'reference_parameters',tuning_file, ...
    'reference_td',td_file,'reference_td_model',td_model_file, ...
    'document','docs/14_TD接入二阶LADRC.md', ...
    'changes','TD频率取原值；实际步长统一为控制周期；v2不接控制律；教学wc=8');
fprintf('\n【第14讲：TD接入二阶LADRC——平滑目标与控制峰值】\n');
fprintf('控制器沿用wc=%g、wo=%g、b0=%g；只比较直接给定与TD的v1给定。\n',wc,wo,b0);
fprintf('TD参数r=%g；原TD模型h=%g秒，本讲实际调用周期统一为%g秒。\n', ...
    p.td_r,original.td_h,h);

%% TD仅由原始目标驱动，无闭环反馈，因此可先生成同一时间标签下的因果轨迹
time = (0:round(p.end_time/h))'*h;
raw_target = p.target_value*double((0:numel(time)-1)' >= round(p.target_time/h));
td_states = zeros(numel(time),2); original_td_states = zeros(numel(time),2);
td = [0;0]; reference_td = [0;0]; td_reference_error = 0;
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(td_dir,'-begin');
assert(strcmpi(which('TD2_L'),td_file),'ADRC:WrongReference','当前TD2_L不是指定原文件。');
for k = 1:numel(time)
    td_states(k,:) = td.';
    original_td_states(k,:) = reference_td.';
    td_next = ltd_step(td,raw_target(k),p.td_r,h);
    original_next = TD2_L(time(k),reference_td,raw_target(k),2,p.td_r,h);
    original_next = original_next(:);
    td_reference_error = max(td_reference_error,max(abs(td_next-original_next)));
    td = td_next; reference_td = original_next;
end
clear restore_path;
td_trajectory = table(time,raw_target,td_states(:,1),td_states(:,2), ...
    'VariableNames',{'time','raw_target','v1','v2'});

%% 两组闭环保持相同对象、控制器、负载和初态，只有位置给定不同
plant_transition = [1 h;0 1];
plant_input = [0.5*h^2*b0;h*b0];
open_transition = [plant_transition zeros(2,3); ...
    h*B_reference(:,2)*[1 0] eye(3)+h*A_reference];
combined_input = [plant_input;h*B_reference(:,1)];
combined_disturbance = [0.5*h^2;h;zeros(3,1)];
closed_transition = open_transition+combined_input*[0 0 -wc^2/b0 -2*wc/b0 -1/b0];
target_input = combined_input*(wc^2/b0);
closed_loop_radius = max(abs(eig(closed_transition)));
assert(closed_loop_radius<1,'ADRC:UnstableConfiguration','当前闭环矩阵不稳定。');
scenario_names = ["direct","ltd"];
labels = ["直接阶跃给定","线性TD的v1给定"];
records = zeros(numel(time)*2,14);
reference_errors = zeros(2,1);
row = 0;
for scenario = 1:2
    y = 0; velocity = 0; z = zeros(3,1); reference_state = zeros(5,1);
    for k = 1:numel(time)
        if scenario==1
            control_target = raw_target(k);
            reference_target = raw_target(k);
        else
            control_target = td_states(k,1); % k拍v1，不能偷用k+1拍预测。
            reference_target = original_td_states(k,1);
        end
        true_f = p.load_value*double(k-1 >= round(p.load_time/h));
        applied_input = adrc_control2(control_target,z,b0,wc);
        row = row+1;
        records(row,:) = [scenario,time(k),raw_target(k),control_target,y,velocity,true_f, ...
            z.',applied_input,raw_target(k)-y,control_target-y,y];
        z_next = adrc_leso3_step(z,applied_input,y,b0,3*wo,3*wo^2,wo^3,h);
        acceleration = b0*applied_input+true_f;
        y_next = y+h*velocity+0.5*h^2*acceleration;
        velocity_next = velocity+h*acceleration;
        reference_next = closed_transition*reference_state ...
            + target_input*reference_target+combined_disturbance*true_f;
        reference_errors(scenario) = max(reference_errors(scenario), ...
            max(abs([y_next;velocity_next;z_next]-reference_next)));
        if k < numel(time)
            y = y_next; velocity = velocity_next; z = z_next; reference_state = reference_next;
        end
    end
end
T = array2table(records(:,2:end),'VariableNames',{'time','raw_target','control_target', ...
    'true_y','true_velocity','true_f','z1','z2','z3','applied_input','raw_tracking_error', ...
    'control_tracking_error','measurement'});
scenario_column = scenario_names(records(:,1));
T = addvars(T,scenario_column(:),'Before',1,'NewVariableNames','scenario');

%% 在同一原始目标和同一负载前窗口内比较；TD不是限流器
peak_inputs = zeros(2,1); settling_times = NaN(2,1);
steady_raw_errors = zeros(2,1); steady_inputs = zeros(2,1);
for scenario = 1:2
    rows = T(T.scenario==scenario_names(scenario),:);
    nominal = find(rows.time>=p.target_time & rows.time<=p.nominal_window_end);
    peak_inputs(scenario) = max(abs(rows.applied_input(nominal)));
    outside = abs(rows.raw_tracking_error(nominal)) > p.settling_fraction*abs(p.target_value);
    last_outside = find(outside,1,'last');
    if isempty(last_outside)
        settling_times(scenario) = 0;
    elseif last_outside < numel(nominal)
        settling_times(scenario) = rows.time(nominal(last_outside+1))-p.target_time;
    end
    steady = rows.time>=p.steady_start;
    steady_raw_errors(scenario) = mean(rows.raw_tracking_error(steady));
    steady_inputs(scenario) = mean(rows.applied_input(steady));
    assert(isequal(rows.raw_target,raw_target),'ADRC:UnequalTargets','两组原始目标不一致。');
end
metrics = table(scenario_names(:),peak_inputs,settling_times,steady_raw_errors,steady_inputs, ...
    'VariableNames',{'scenario','nominal_peak_input','raw_target_settling_s', ...
    'steady_raw_error','steady_input'});
assert(all(isfinite(records),'all') && all(isfinite(metrics{:,2:end}),'all'), ...
    'ADRC:NonfiniteResult','当前窗口内未到位或结果存在非有限值。');
assert(td_reference_error<1e-10 && max(reference_errors)<1e-8, ...
    'ADRC:ReferenceMismatch','TD或闭环与参考更新不一致。');
assert(all(abs(steady_raw_errors)<1e-7) && all(abs(steady_inputs+p.load_value/b0)<1e-7), ...
    'ADRC:SteadyMismatch','末段位置误差或维持输入与当前模型预期不一致。');
assert(strcmp(model_snapshot,fileread(model_file)) && strcmp(tuning_snapshot,fileread(tuning_file)) ...
    && strcmp(td_snapshot,fileread(td_file)) && strcmp(td_model_snapshot,fileread(td_model_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');
metrics_cn = table(labels(:),peak_inputs,settling_times,steady_raw_errors, ...
    'VariableNames',{'目标处理方式','负载前电流峰值_A','原始目标到位时间_s','稳态原始目标误差_rad'});
disp(metrics_cn);
fprintf('验证通过：TD原函数最大差值%.3g；完整闭环对照最大差值%.3g。\n', ...
    td_reference_error,max(reference_errors));
writetable(td_trajectory,fullfile(output_dir,'td_trajectory.csv'));
writetable(T,fullfile(output_dir,'td_ladrc_sim.csv'));
writetable(metrics,fullfile(output_dir,'td_ladrc_metrics.csv'));
save(fullfile(output_dir,'td_ladrc_result.mat'),'T','td_trajectory','metrics','p','original', ...
    'teaching','td_reference_error','reference_errors','closed_loop_radius');

%% 中文图：同时展示原始目标、平滑目标和真实输出，避免混淆跟踪指标
fig = figure('Name','第14讲：TD接入二阶LADRC','Color','w','Position',[70 40 1260 820]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout); hold(ax1,'on'); ax2 = nexttile(layout); hold(ax2,'on');
ax3 = nexttile(layout); hold(ax3,'on'); ax4 = nexttile(layout); hold(ax4,'on');
plot(ax1,time,raw_target,'k--','DisplayName','原始阶跃目标');
plot(ax1,time,td_states(:,1),'Color',[0.18 0.55 0.32],'LineWidth',1.3,'DisplayName','TD平滑目标 v1');
colors = [0.78 0.40 0.18;0.22 0.42 0.70];
for scenario = 1:2
    rows = T(T.scenario==scenario_names(scenario),:);
    plot(ax1,time,rows.true_y,'Color',colors(scenario,:),'LineWidth',1.4,'DisplayName',labels(scenario));
    plot(ax2,time,rows.applied_input,'Color',colors(scenario,:),'LineWidth',1.3,'DisplayName',labels(scenario));
    plot(ax4,time,rows.raw_tracking_error,'Color',colors(scenario,:),'LineWidth',1.3,'DisplayName',labels(scenario));
end
td_rows = T(T.scenario==scenario_names(2),:);
plot(ax3,time,td_states(:,2),'Color',[0.18 0.55 0.32],'LineWidth',1.3,'DisplayName','TD变化率 v2（仅记录）');
plot(ax3,time,td_rows.z2,'Color',colors(2,:),'LineWidth',1.3,'DisplayName','LESO角速度估计 z2');
title(ax1,'原始目标、TD平滑目标与真实角度'); ylabel(ax1,'角度（rad）'); xlim(ax1,[0.4 2.4]);
title(ax2,'相同控制律下的输入峰值'); ylabel(ax2,'电流（A）'); xlim(ax2,[0.4 2.3]);
title(ax3,'TD变化率与对象角速度估计'); ylabel(ax3,'角速度（rad/s）'); xlim(ax3,[0.4 2.4]);
title(ax4,'按同一原始目标比较跟踪误差'); ylabel(ax4,'原始目标－真实角度（rad）'); xlim(ax4,[0.4 2.4]);
for ax = [ax1 ax2 ax3 ax4]
    xlabel(ax,'时间（秒）'); grid(ax,'on'); legend(ax,'show','Location','best','FontSize',8);
end
title(layout,'第14讲｜线性TD接入二阶LADRC：仅改变位置给定，v2不接控制律');
adrc_export_figure(fig,output_dir,"td_ladrc_response");
