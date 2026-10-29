%% 第20讲：一阶对象的降阶LESO——一个内部状态如何估计扰动
% 教学归属：LESO专题，开环同轨迹比较；不接TD和控制律，不新增C++核心。
% 点击“运行”（F5）执行整个文件，对应docs/20_一阶对象的降阶LESO.md。
repo = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
result = run_example(repo);

function result = run_example(repo)
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file = fullfile(reference_dir,'ADRC_1.m');
template_file = fullfile(reference_dir,'ESO_1rd.m');
assert(isfile(reference_file) && isfile(template_file), ...
    'ADRC:MissingReference','请检查ADRC1的ADRC_1.m和ESO_1rd.m。');
source_snapshot = fileread(reference_file);
template_snapshot = fileread(template_file);
addpath(reference_dir,'-begin');
assert(strcmpi(which('ADRC_1'),reference_file), ...
    'ADRC:WrongReference','当前ADRC_1不是指定参考文件。');
output_dir = fullfile(repo,'build','lesson20');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 与第10讲相同的开环对象，补充非零初态和成对噪声实验
p = struct('b0',10,'omega_o',20,'h',0.001,'end_time',3, ...
    'initial_y',3,'initial_f_hat',0,'input_time',0.5,'input_value',0.3, ...
    'load_time',1.5,'load_value',-2,'noise_sigma',0.001,'seed',20261028);
p.beta1 = 2*p.omega_o;
p.beta2 = p.omega_o^2;
time = (0:round(p.end_time/p.h))'*p.h;
N = numel(time);
applied_input = p.input_value*double(time >= p.input_time);
true_f = p.load_value*double(time >= p.load_time);
% 区间内输入和扰动为常量，对本讲一阶对象这是精确解析轨迹。
true_y = p.initial_y+p.b0*p.input_value*max(time-p.input_time,0) ...
    +p.load_value*max(time-p.load_time,0);
rng_before = rng;
stream = RandStream('mt19937ar','Seed',p.seed);
noise = p.noise_sigma*randn(stream,N,1);
stream_repeat = RandStream('mt19937ar','Seed',p.seed);
assert(isequal(noise,p.noise_sigma*randn(stream_repeat,N,1)), ...
    'ADRC:NoiseRepeat','局部噪声序列不一致。');
teaching = struct('lesson','第20讲','algorithm','一阶对象的降阶线性ESO', ...
    'topic','辅助状态变换、当前测量重构与全阶形式的区别', ...
    'reference_code',reference_file,'empty_reference_template',template_file, ...
    'document','docs/20_一阶对象的降阶LESO.md', ...
    'parameter_source','沿用第10讲教学参数，非原完整模型或实机参数', ...
    'supplemental_source','https://doi.org/10.3390/engproc2026133085');
fprintf('\n【第20讲：一阶对象的降阶LESO】\n');
fprintf('只积分q：f_hat=q+omega_o*y_meas；q_dot=-omega_o*(f_hat+b0*u)。\n');
fprintf('原ESO_1rd.m是空模板；全阶比较仍核对ADRC_1.m第3、4个状态。\n');
fprintf('初始速度%g rad/s，期望初始扰动估计%g，因此干净组q初值=%g。\n', ...
    p.initial_y,p.initial_f_hat,p.initial_f_hat-p.omega_o*p.initial_y);

%% 两组复用相同真值；每组的全阶/降阶使用同一测量和实际输入
case_names = ["clean","noisy"];
data = cell(2,1);
recurrence_max = zeros(2,1);
reference_max = zeros(2,1);
for case_id = 1:2
    measurement_noise = noise*double(case_id == 2);
    measurement = true_y+measurement_noise;
    q = p.initial_f_hat-p.omega_o*measurement(1);
    z = [measurement(1);p.initial_f_hat];
    reference_x = [0;0;z];
    records = zeros(N,14);
    for k = 1:N
        [q_next,f_hat] = leso_reduced_step(q,applied_input(k),measurement(k), ...
            p.b0,p.omega_o,p.h);
        z_next = adrc_eso_step(z,applied_input(k),measurement(k), ...
            p.b0,p.beta1,p.beta2,p.h);
        reference_next = ADRC_1(time(k),reference_x, ...
            [0;measurement(k);p.b0*applied_input(k)], ...
            2,1,p.h,p.beta1,p.beta2,1);
        reference_next = reference_next(:);
        reference_max(case_id) = max(reference_max(case_id), ...
            max(abs(z_next-reference_next(3:4))));
        records(k,:) = [time(k),applied_input(k),true_y(k),true_f(k), ...
            measurement(k),measurement_noise(k),q,f_hat,q_next, ...
            z(1),z(2),z_next(1),z_next(2),p.h];
        if k < N
            q = q_next;
            z = z_next;
            reference_x = reference_next;
        end
    end
    T = array2table(records,'VariableNames',{'time','applied_input','true_y', ...
        'true_f','measurement','noise','q','f_hat','q_next', ...
        'z1','z2','z1_next','z2_next','dt'});
    % 独立核对消去辅助状态后的离散等式，含相邻两次测量噪声差。
    expected_next = (1-p.omega_o*p.h)*T.f_hat(1:end-1) ...
        +p.omega_o*p.h*T.true_f(1:end-1)+p.omega_o*diff(T.noise);
    recurrence_max(case_id) = max(abs(T.f_hat(2:end)-expected_next));
    assert(recurrence_max(case_id) < 1e-10, ...
        'ADRC:RecurrenceMismatch','降阶估计与独立离散递推不一致。');
    assert(reference_max(case_id) < 1e-10, ...
        'ADRC:ReferenceMismatch','全阶比较组与原ADRC_1不一致。');
    assert(max(abs(T.q_next(1:end-1)-T.q(2:end))) == 0 ...
        && max(abs(T.z1_next(1:end-1)-T.z1(2:end))) == 0 ...
        && max(abs(T.z2_next(1:end-1)-T.z2(2:end))) == 0, ...
        'ADRC:TimeLabelMismatch','当前状态和下一拍预测的标签不一致。');
    assert(abs(T.f_hat(1)-p.initial_f_hat) < 1e-12 ...
        && all(isfinite(records),'all'), ...
        'ADRC:StateMismatch','初始化或有限数值检查失败。');
    writetable(T,fullfile(output_dir,char(case_names(case_id)+"_observer.csv")));
    data{case_id} = T;
end
clean = data{1}; noisy = data{2};

%% 干净组的独立解析解、已知输入抵消和扰动阶跃因果性
load_row = round(p.load_time/p.h)+1;
after_load = load_row:N;
n = (0:numel(after_load)-1)';
a = 1-p.omega_o*p.h;
expected_reduced = p.load_value*(1-a.^n);
expected_full = p.load_value*(1-a.^n-n*p.omega_o*p.h.*a.^(n-1));
analytic_max = max([abs(clean.f_hat(after_load)-expected_reduced); ...
    abs(clean.z2(after_load)-expected_full)]);
assert(analytic_max < 1e-10,'ADRC:AnalyticMismatch','与阶跃离散解析解不一致。');
assert(max(abs(clean.f_hat(1:load_row))) < 1e-10 ...
    && abs(clean.f_hat(load_row+1)-p.omega_o*p.h*p.load_value) < 1e-10 ...
    && abs(clean.z2(load_row+1)) < 1e-10, ...
    'ADRC:Causality','已知输入抵消或阶跃首次可见时序异常。');
% 固定q时的测量直通增益应为omega_o，无需对测量作数值微分。
probe_q = p.initial_f_hat-p.omega_o*p.initial_y;
[~,probe0] = leso_reduced_step(probe_q,0,p.initial_y,p.b0,p.omega_o,p.h);
[~,probe1] = leso_reduced_step(probe_q,0,p.initial_y+0.01,p.b0,p.omega_o,p.h);
direct_gain_error = abs((probe1-probe0)-p.omega_o*0.01);
assert(direct_gain_error < 1e-12,'ADRC:DirectGain','测量直通增益检查失败。');
assert(isequal(rng_before,rng),'ADRC:RngChanged','全局随机数状态被改变。');
assert(strcmp(source_snapshot,fileread(reference_file)) ...
    && strcmp(template_snapshot,fileread(template_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');

%% 指标：同带宽参数不代表相同阶跃响应或相同噪声滤波能力
tail = time >= 2.5;
reduced_noise = noisy.f_hat-clean.f_hat;
full_noise = noisy.z2-clean.z2;
settling = [settling_time(clean.time,clean.f_hat,p);settling_time(clean.time,clean.z2,p)];
noise_rms = [sqrt(mean(reduced_noise(tail).^2));sqrt(mean(full_noise(tail).^2))];
metrics = table(["reduced";"full"],settling,noise_rms, ...
    'VariableNames',{'observer','clean_settling_2pct_s','disturbance_noise_rms'});
timing = clean(load_row+(0:3),{'time','true_y','true_f','q','f_hat','z1','z2'});
writetable(metrics,fullfile(output_dir,'observer_metrics.csv'));
writetable(timing,fullfile(output_dir,'step_timing.csv'));
result = struct('teaching',teaching,'parameters',p,'clean',clean,'noisy',noisy, ...
    'metrics',metrics,'timing',timing,'recurrence_max',recurrence_max, ...
    'reference_max',reference_max,'analytic_max',analytic_max, ...
    'direct_gain_error',direct_gain_error,'rng_unchanged',isequal(rng_before,rng));
save(fullfile(output_dir,'reduced_leso_result.mat'),'result');
fprintf('验证通过：原全阶函数最大差值%.3g，降阶递推残差%.3g，阶跃解析解差值%.3g。\n', ...
    max(reference_max),max(recurrence_max),analytic_max);
fprintf('干净阶跃2%%收敛时间：降阶%.3f秒，全阶%.3f秒（相同带宽参数）。\n',settling(1),settling(2));
fprintf('末段扰动估计噪声RMS：降阶%.5g，全阶%.5g rad/s²。\n',noise_rms(1),noise_rms(2));
disp(timing);

%% 中文教学图：状态数量、阶跃和噪声代价
fig = figure('Name','第20讲：一阶对象的降阶LESO', ...
    'Color','w','Position',[70 40 1260 820]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout);
plot(ax1,time,clean.true_y,'k-',time,clean.z1,'--','LineWidth',1.3);
title(ax1,'全阶估计y；降阶直接使用测量y');
ylabel(ax1,'角速度（rad/s）');
legend(ax1,'真实速度（干净组也等于测量）','全阶输出估计z1','Location','northwest');
ax2 = nexttile(layout);
plot(ax2,time,clean.true_f,'k--',time,clean.f_hat,time,clean.z2,'LineWidth',1.3);
xlim(ax2,[1.45 1.95]);
title(ax2,'干净组：同带宽参数下的扰动阶跃响应');
ylabel(ax2,'扰动估计（rad/s²）');
legend(ax2,'真实f','降阶f_hat','全阶z2','Location','northeast');
ax3 = nexttile(layout);
plot(ax3,time,clean.q,time,p.omega_o*clean.measurement,time,clean.f_hat,'LineWidth',1.2);
title(ax3,'辅助状态q不是扰动：f_hat=q+ωo*y');
ylabel(ax3,'辅助量与扰动（rad/s²）');
legend(ax3,'内部状态q','测量项ωo*y','重构扰动f_hat','Location','best');
ax4 = nexttile(layout);
plot(ax4,time,reduced_noise,time,full_noise,'LineWidth',1.0);
xlim(ax4,[2.5 2.75]);
title(ax4,'同一份测量噪声：降阶输出含测量直通项');
ylabel(ax4,'相对干净组的估计差（rad/s²）');
legend(ax4,'降阶估计噪声','全阶估计噪声','Location','best');
for ax = [ax1 ax2 ax3 ax4]
    grid(ax,'on'); xlabel(ax,'时间（秒）');
end
title(layout,'第20讲｜一阶对象的降阶LESO：一个动态状态、当前测量与噪声依赖');
adrc_export_figure(fig,output_dir,"reduced_leso_comparison");
end

function elapsed = settling_time(time,estimate,p)
rows = find(time >= p.load_time);
outside = find(abs(estimate(rows)-p.load_value) > 0.02*abs(p.load_value),1,'last');
if isempty(outside)
    elapsed = 0;
elseif outside == numel(rows)
    elapsed = NaN;
else
    elapsed = time(rows(outside+1))-p.load_time;
end
assert(isfinite(elapsed),'ADRC:NotSettled','干净组在观察区间内未收敛。');
end
