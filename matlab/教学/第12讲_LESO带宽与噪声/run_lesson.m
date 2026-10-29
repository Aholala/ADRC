%% 第12讲：三阶LESO的带宽与测量噪声——快慢和抖动的取舍
% 教学归属：LESO专题；同一对象轨迹、同一份测量噪声，对比三个带宽。
% 直接点击“运行”（F5），运行整个文件。对应docs/12_LESO带宽与噪声.md。
% 本讲复用第11讲三阶LESO公式，不接TD或LADRC闭环。
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
output_dir = fullfile(repo,'build','lesson12');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 读取原h、b0、wo和矩阵表达式；只改变wo，不执行完整原模型
original = struct;
for name = {'h','b0','wo'}
    token = regexp(tuning_snapshot,['(?m)^\s*' name{1} '\s*=\s*([^;]+);'],'tokens','once');
    assert(~isempty(token),'ADRC:MissingParameter','原参数未找到。');
    original.(name{1}) = str2double(strtrim(token{1}));
end
h = original.h; b0 = original.b0;
omega_values = original.wo*[0.5 1 2];
assert(all(isfinite([h b0 omega_values])) && h > 0 && b0 ~= 0 ...
    && all(omega_values > 0) && all(omega_values*h < 2), ...
    'ADRC:InvalidReference','检查当前带宽和采样周期。');
blocks = regexp(model_snapshot, ...
    'BlockType\s+StateSpace\s+Name\s+"LESO[^"]*"(.*?)\n\s*}', 'tokens');
assert(numel(blocks)==1,'ADRC:ReferenceChanged','原LESO块结构变化。');
token_a = regexp(blocks{1}{1},'(?m)^\s*A\s+"([^"]+)"','tokens','once');
token_b = regexp(blocks{1}{1},'(?m)^\s*B\s+"([^"]+)"','tokens','once');
assert(~isempty(token_a) && ~isempty(token_b),'ADRC:MissingMatrix','原LESO矩阵未找到。');

% 教学假设：沿用第11讲对象、输入和扰动，时长延至6秒；新增离散测量噪声。
p = struct('h',h,'b0',b0,'end_time',6,'input_time',0.5,'input_value',0.075, ...
    'load_time',1.5,'load_value',-2,'noise_sigma',0.001,'noise_seed',20261028, ...
    'steady_start',4,'settling_window_end',3,'settling_fraction',0.02);
teaching = struct('lesson','第12讲','algorithm','二阶对象的三阶LESO', ...
    'topic','相同对象与噪声下比较带宽、无噪声收敛和稳态噪声增量', ...
    'reference_model',model_file,'reference_parameters',tuning_file, ...
    'document','docs/12_LESO带宽与噪声.md', ...
    'noise_source','本讲新增离散高斯噪声，不复刻原Simulink噪声序列');
fprintf('\n【第12讲：三阶LESO的带宽与测量噪声】\n');
fprintf('基准wo=%g；对比带宽=%s rad/s，全部使用同一对象、输入、扰动和噪声样本。\n', ...
    original.wo,mat2str(omega_values));
fprintf('教学噪声标准差=%g rad，固定种子=%d；噪声只加入角度测量。\n', ...
    p.noise_sigma,p.noise_seed);

%% 先生成唯一对象轨迹和唯一噪声序列，再交给各个观测器
time = (0:round(p.end_time/h))'*h;
tau_input = max(time-p.input_time,0);
tau_load = max(time-p.load_time,0);
true_y = 0.5*b0*p.input_value*tau_input.^2+0.5*p.load_value*tau_load.^2;
true_velocity = b0*p.input_value*tau_input+p.load_value*tau_load;
applied_input = p.input_value*double((0:numel(time)-1)' >= round(p.input_time/h));
true_f = p.load_value*double((0:numel(time)-1)' >= round(p.load_time/h));
stream = RandStream('mt19937ar','Seed',p.noise_seed); % 独立流，不更改用户全局rng。
noise = p.noise_sigma*randn(stream,numel(time),1);
measurement_vectors = [true_y,true_y+noise];
scenario_names = ["clean","noisy"];
states = zeros(numel(time),3,numel(omega_values),2);
reference_errors = zeros(numel(omega_values),2);
records = zeros(numel(time)*numel(omega_values)*2,13);
row = 0;
for scenario = 1:2
    for band = 1:numel(omega_values)
        wo = omega_values(band);
        beta1 = 3*wo; beta2 = 3*wo^2; beta3 = wo^3;
        A_reference = eval(token_a{1}); % 原矩阵表达式，仅替换当前wo。
        B_reference = eval(token_b{1});
        z = zeros(3,1); reference_z = zeros(3,1);
        for k = 1:numel(time)
            measurement = measurement_vectors(k,scenario);
            states(k,:,band,scenario) = z.';
            row = row+1;
            records(row,:) = [scenario,wo,time(k),applied_input(k),true_y(k), ...
                true_velocity(k),true_f(k),measurement,noise(k)*(scenario==2), ...
                z(1),z(2),z(3),b0*applied_input(k)+z(3)];
            z_next = adrc_leso3_step(z,applied_input(k),measurement,b0,beta1,beta2,beta3,h);
            reference_next = reference_z+h*(A_reference*reference_z ...
                + B_reference*[applied_input(k);measurement]);
            reference_errors(band,scenario) = max(reference_errors(band,scenario), ...
                max(abs(z_next-reference_next)));
            z = z_next;
            reference_z = reference_next;
        end
    end
end
T = array2table(records(:,2:end),'VariableNames',{'omega_o','time','applied_input', ...
    'true_y','true_velocity','true_f','measurement','noise','z1','z2','z3','model_acceleration'});
scenario_column = scenario_names(records(:,1));
T = addvars(T,scenario_column(:),'Before',1,'NewVariableNames','scenario');

%% 指标：无噪声收敛时间，与有噪声减去无噪声的稳态增量RMS
steady_rows = time >= p.steady_start;
settling_rows = find(time >= p.load_time & time <= p.settling_window_end);
settling_times = NaN(numel(omega_values),1);
noise_rms = zeros(numel(omega_values),3);
noisy_disturbance_rmse = zeros(numel(omega_values),1);
for band = 1:numel(omega_values)
    clean = states(:,:,band,1);
    noisy = states(:,:,band,2);
    delta = noisy-clean;
    noise_rms(band,:) = sqrt(mean(delta(steady_rows,:).^2,1));
    noisy_disturbance_rmse(band) = sqrt(mean((noisy(steady_rows,3)-true_f(steady_rows)).^2));
    outside = abs(clean(settling_rows,3)-true_f(settling_rows)) ...
        > p.settling_fraction*abs(p.load_value);
    last_outside = find(outside,1,'last');
    if isempty(last_outside)
        settling_times(band) = 0;
    elseif last_outside < numel(settling_rows)
        settling_times(band) = time(settling_rows(last_outside+1))-p.load_time;
    end
    % 核对实际导出的比较条件，而不只检查参数声明。
    for scenario = 1:2
        rows = T(T.omega_o==omega_values(band) & T.scenario==scenario_names(scenario),:);
        assert(isequal(rows.true_y,true_y) && isequal(rows.applied_input,applied_input) ...
            && isequal(rows.true_f,true_f) && isequal(rows.measurement,measurement_vectors(:,scenario)), ...
            'ADRC:UnequalInputs','带宽对比使用了不同轨迹、输入或测量。');
    end
end
metrics = table(omega_values(:),settling_times,noise_rms(:,1),noise_rms(:,2),noise_rms(:,3), ...
    noisy_disturbance_rmse,'VariableNames',{'omega_o','clean_settling_s','z1_noise_rms', ...
    'z2_noise_rms','z3_noise_rms','noisy_z3_rmse'});
assert(all(isfinite(records),'all') && all(isfinite(metrics{:,:}),'all'), ...
    'ADRC:NonfiniteResult','当前实验未收敛或结果存在非有限数值。');
assert(max(reference_errors,[],'all') < 1e-8,'ADRC:ReferenceMismatch','与原矩阵更新不一致。');
assert(strcmp(model_snapshot,fileread(model_file)) && strcmp(tuning_snapshot,fileread(tuning_file)), ...
    'ADRC:ReferenceChanged','参考文件发生变化。');
metrics_cn = table(omega_values(:),settling_times,noise_rms(:,1),noise_rms(:,2),noise_rms(:,3), ...
    'VariableNames',{'带宽参数_rad_s','无噪声收敛时间_s','角度噪声增量RMS_rad', ...
    '角速度噪声增量RMS_rad_s','扰动噪声增量RMS_rad_s2'});
disp(metrics_cn);
fprintf('验证通过：六组观测器与原矩阵最大差值%.3g；各带宽共用同一轨迹和测量。\n', ...
    max(reference_errors,[],'all'));
writetable(T,fullfile(output_dir,'bandwidth_noise_sim.csv'));
writetable(metrics,fullfile(output_dir,'bandwidth_noise_metrics.csv'));
save(fullfile(output_dir,'bandwidth_noise_result.mat'),'T','metrics','p','teaching','original', ...
    'omega_values','noise','measurement_vectors','states','reference_errors');

%% 中文图：同一对象上，收敛快慢与噪声影响分别比较
fig = figure('Name','第12讲：LESO带宽与测量噪声','Color','w','Position',[70 40 1280 820]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout); hold(ax1,'on');
ax2 = nexttile(layout); hold(ax2,'on');
ax3 = nexttile(layout); hold(ax3,'on');
ax4 = nexttile(layout); hold(ax4,'on');
plot(ax1,time,true_f,'k--','DisplayName','真实总扰动');
plot(ax2,time,true_f,'k--','DisplayName','真实总扰动');
colors = [0.16 0.52 0.36; 0.26 0.39 0.67; 0.82 0.42 0.15];
for band = 1:numel(omega_values)
    label = sprintf('带宽参数=%g rad/s',omega_values(band));
    if omega_values(band)==original.wo, label = [label '（原例基准）']; end
    clean = states(:,:,band,1); noisy = states(:,:,band,2);
    plot(ax1,time,clean(:,3),'Color',colors(band,:),'LineWidth',1.3,'DisplayName',label);
    plot(ax2,time,noisy(:,3),'Color',colors(band,:),'LineWidth',1,'DisplayName',label);
    plot(ax3,time,noisy(:,2)-clean(:,2),'Color',colors(band,:),'DisplayName',label);
    plot(ax4,time,noisy(:,3)-clean(:,3),'Color',colors(band,:),'DisplayName',label);
end
title(ax1,'无噪声：总扰动估计收敛快慢'); ylabel(ax1,'总扰动（rad/s²）'); xlim(ax1,[1.4 2.1]);
title(ax2,'同一噪声：总扰动估计的跟踪与抖动'); ylabel(ax2,'总扰动（rad/s²）'); xlim(ax2,[1.4 2.1]);
title(ax3,'稳态：噪声引起的角速度估计增量'); ylabel(ax3,'有噪声z2－无噪声z2（rad/s）'); xlim(ax3,[4 4.5]);
title(ax4,'稳态：噪声引起的总扰动估计增量'); ylabel(ax4,'有噪声z3－无噪声z3（rad/s²）'); xlim(ax4,[4 4.5]);
for ax = [ax1 ax2 ax3 ax4]
    xlabel(ax,'时间（秒）'); grid(ax,'on'); legend(ax,'show','FontSize',8,'Location','best');
end
title(layout,'第12讲｜三阶LESO的带宽与测量噪声：相同对象、相同噪声，仅改变带宽');
adrc_export_figure(fig,output_dir,"bandwidth_noise_response");
