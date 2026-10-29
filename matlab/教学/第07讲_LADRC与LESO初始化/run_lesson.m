%% 第7讲：一阶LADRC与二阶LESO——非零状态初始化与接管
% 教学对象：LADRC（线性自抗扰控制器）+ LESO（线性扩张状态观测器）。
% 本讲目标：比较四种初始化，观察接管首拍电流和后续估计瞬态。
% 对应讲义：docs/07_初始化与无扰接管.md。
% 这是可直接点击“运行”的教学脚本，无需填写函数参数。
% 先看“接管瞬间的实际输入”图，再对照 z1、z2 的初始值。
% ADRC1 是公式结构的优先参考。本例单独设计教学场景，不声称复现某个原始模型。
repo = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
addpath(fullfile(repo,'matlab','内部函数'));
output_dir = fullfile(repo,'build','lesson07');
if ~isfolder(output_dir), mkdir(output_dir); end
teaching = struct('lesson','第7讲','algorithm','一阶LADRC + 二阶LESO', ...
    'topic','非零状态初始化与接管','document','docs/07_初始化与无扰接管.md');
fprintf('\n【第7讲：一阶LADRC与二阶LESO——非零状态初始化与接管】\n');
fprintf('学习目标：解释为什么只初始化速度仍可能使电流跳变。\n');

%% 参数：沿用前几节教学配置，不是实际机器人默认值
p = struct('b0',10,'omega_c',8,'omega_o',40,'dt',0.001, ...
    'initial_output',2,'load',-3,'manual_input',0.3,'enable_time',1,'end_time',3);
p.beta1 = 2*p.omega_o;
p.beta2 = p.omega_o^2;
assert(abs(p.b0*p.manual_input+p.load) < 1e-12, ...
    '教学初态必须是手动输入与负载平衡的稳态。');
assert(p.omega_o*p.dt < 2 && p.omega_c*p.dt < 2, ...
    '当前教学离散配置超出理想模型稳定范围。');
names = ["zero_init", "output_only", "output_and_input", "observer_tracking"];
samples = round(p.end_time/p.dt)+1;
enable_sample = round(p.enable_time/p.dt);
records = zeros(numel(names)*samples,9);
first_commands = zeros(numel(names),1);
peak_deviations = zeros(numel(names),1);
row = 0;

%% 模型 dy/dt=b0*u+f，四个独立对象均从同一非零稳态开始
for strategy = 1:numel(names)
    y = p.initial_output;
    z = zeros(2,1);
    last_applied = p.manual_input;
    for k = 0:samples-1
        time = k*p.dt;
        target = p.initial_output; % 接管时不改变目标，隔离初始化本身的影响。
        enabled = k >= enable_sample;
        if k == enable_sample
            if strategy == 1
                z = [0;0];
            elseif strategy == 2
                z = [y;0];
            elseif strategy == 3
                % 从 u_prev=[omega_c*(r-z1)-z2]/b0 反解。
                z = [y; p.omega_c*(target-y)-p.b0*last_applied];
            end
            % 第四组保留接管前一直更新的 ESO，不在此时清零。
        end
        if enabled
            applied = adrc_control(target,z,p.b0,p.omega_c);
            peak_deviations(strategy) = max(peak_deviations(strategy),abs(y-target));
        else
            applied = p.manual_input;
        end
        if k == enable_sample, first_commands(strategy) = applied; end

        row = row+1;
        records(row,:) = [strategy,time,enabled,target,y,applied,z(1),z(2),p.load];
        if k < samples-1
            if enabled || strategy == 4
                % 输出未接管时，第四组仍用真实手动输入和测量更新 ESO。
                z = adrc_eso_step(z,applied,y,p.b0,p.beta1,p.beta2,p.dt);
            end
            y = y+p.dt*(p.b0*applied+p.load); % 区间内恒值输入下的精确对象更新。
            last_applied = applied;
        end
    end
end

T = array2table(records(:,2:end),'VariableNames', ...
    {'time','enabled','target','true_output','applied_input','z1','z2','true_disturbance'});
strategy_column = names(records(:,1));
T = addvars(T,strategy_column(:),'Before',1,'NewVariableNames','strategy');
metrics = table(names(:),first_commands,first_commands-p.manual_input,peak_deviations, ...
    'VariableNames',{'strategy','first_command','handoff_jump','peak_output_deviation'});
writetable(T,fullfile(output_dir,'handoff_sim.csv'));
writetable(metrics,fullfile(output_dir,'handoff_metrics.csv'));
save(fullfile(output_dir,'handoff_result.mat'),'T','metrics','p','teaching');
labels = ["全零初始化", "只对齐当前速度", "同时对齐速度和原输入", "LESO提前跟踪"];
metrics_cn = table(labels(:),first_commands,first_commands-p.manual_input,peak_deviations, ...
    'VariableNames',{'初始化方式','接管首拍电流_A','电流跳变量_A','最大速度偏差_rad_s'});
disp(metrics_cn);

%% 核对首拍控制的解析结果，以及匹配稳态下的平衡保持
assert(abs(first_commands(1)-1.6) < 1e-10, '全零初始化的首拍应为 1.6 A。');
assert(abs(first_commands(2)) < 1e-10, '只初始化输出时首拍应为 0 A。');
assert(abs(first_commands(3)-p.manual_input) < 1e-10, '输入输出对齐应保持首拍输入。');
assert(peak_deviations(3) < 1e-10, '本理想稳态例应保持输出。');
assert(abs(first_commands(4)-p.manual_input) < 1e-7, '充分跟踪后应接近原输入。');
assert(all(isfinite(T{:,2:end}),'all'), '仿真结果必须有限。');
fprintf('验证通过：首拍解析结果、稳态平衡、LESO提前跟踪和日志有限性。\n');

%% 图：接管瞬间的物理响应，以及观测器状态从何而来
fig = figure('Name','第7讲：一阶LADRC与二阶LESO——初始化与接管','Color','w', ...
    'Position',[80 50 1160 820]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout); hold(ax1,'on');
ax2 = nexttile(layout); hold(ax2,'on');
ax3 = nexttile(layout); hold(ax3,'on');
ax4 = nexttile(layout); hold(ax4,'on');
colors = [0.81 0.32 0.20; 0.78 0.60 0.12; 0.14 0.52 0.35; 0.25 0.40 0.74];
for strategy = 1:numel(names)
    rows = T(T.strategy==names(strategy),:);
    style = '-';
    if strategy == 4, style = '--'; end
    plot(ax1,rows.time,rows.true_output,style,'Color',colors(strategy,:), ...
        'LineWidth',1.4,'DisplayName',labels(strategy));
    plot(ax2,rows.time,rows.applied_input,style,'Color',colors(strategy,:), ...
        'LineWidth',1.4,'DisplayName',labels(strategy));
    plot(ax3,rows.time,rows.z1,style,'Color',colors(strategy,:), ...
        'LineWidth',1.4,'DisplayName',labels(strategy));
    plot(ax4,rows.time,rows.z2,style,'Color',colors(strategy,:), ...
        'LineWidth',1.4,'DisplayName',labels(strategy));
end
yline(ax1,p.initial_output,'k:','HandleVisibility','off');
yline(ax2,p.manual_input,'k:','HandleVisibility','off');
yline(ax4,p.load,'k:','HandleVisibility','off');
title(ax1,'接管前后的实际速度'); ylabel(ax1,'角速度（rad/s）'); xlim(ax1,[0.8 2.5]);
title(ax2,'接管瞬间的实际输入'); ylabel(ax2,'电流（A）'); xlim(ax2,[0.95 1.45]);
title(ax3,'LESO的速度估计 z1'); ylabel(ax3,'估计角速度（rad/s）'); xlim(ax3,[0 2]);
title(ax4,'LESO的总扰动估计 z2'); ylabel(ax4,'总扰动估计（rad/s²）'); xlim(ax4,[0 2]);
for ax = [ax1 ax2 ax3 ax4]
    xline(ax,p.enable_time,'k:','HandleVisibility','off');
    grid(ax,'on'); xlabel(ax,'时间（s）'); legend(ax,'show','FontSize',8,'Interpreter','none');
end
title(layout,'第7讲｜一阶LADRC与二阶LESO：非零状态初始化与接管（同一目标、同一负载）');
adrc_export_figure(fig,output_dir,"handoff_response");
