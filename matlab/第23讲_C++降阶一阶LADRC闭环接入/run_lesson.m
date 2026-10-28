%% 第23讲：C++降阶一阶LADRC自主闭环与MATLAB分层核对
% 教学归属：工程接入。复用第22讲核心，不新增公共控制器类，不接TD。
% 运行整个文件（F5）；需要MATLAB、g++和本机ADRC1参考文件，无需MEX。
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
result = run_alignment(repo);

function result = run_alignment(repo)
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file = fullfile(reference_dir,'ADRC_1.m');
assert(isfile(reference_file),'ADRC:MissingReference','请检查ADRC1的ADRC_1.m。');
source_snapshot = fileread(reference_file);
core_file = fullfile(repo,'src','reduced_linear_eso.cpp');
core_snapshot = fileread(core_file);
addpath(reference_dir,'-begin');
assert(strcmpi(which('ADRC_1'),reference_file),'ADRC:WrongReference','当前ADRC_1不是指定参考文件。');
output_dir = fullfile(repo,'build','lesson23');
if ~isfolder(output_dir), mkdir(output_dir); end
p = struct('b0',10,'omega_c',8,'omega_o',20,'nominal_h',0.001,'end_time',5, ...
    'initial_y',3,'initial_f_hat',0,'first_target_time',0.5,'first_target',4, ...
    'second_target_time',2.5,'second_target',2,'load_time',1.5,'load_value',-2, ...
    'input_limit',0.3,'noise_sigma',0.001,'seed',20261028,'tail_time',4.5);
teaching = struct('lesson','第23讲','algorithm','C++降阶LESO的一阶LADRC闭环', ...
    'topic','C++自主对象、同测量single回放与独立double闭环', ...
    'document','docs/23_C++降阶一阶LADRC闭环接入.md','reference_code',reference_file, ...
    'parameter_source','沿用第21讲教学场景，周期和限幅采用实际传入的float表示');
fprintf('\n【第23讲：C++降阶一阶LADRC闭环接入】\n');
fprintf('C++自己推进对象；single回放核对算法，独立double闭环核对整体轨迹。\n');

%% 只编译相关核心、已有主机测试与本讲接入例程，退出时恢复PATH
compiler = getenv('ADRC_CXX');
if isempty(compiler)
    [status,found] = system('where g++');
    if status == 0
        candidates = splitlines(strtrim(string(found))); compiler = char(candidates(1));
    elseif isfile('D:/msys64/ucrt64/bin/g++.exe')
        compiler = 'D:/msys64/ucrt64/bin/g++.exe';
    end
end
assert(isfile(compiler),'ADRC:MissingCompiler','未找到g++，请将ADRC_CXX设为编译器完整路径。');
previous_env = getenv('PATH');
env_cleanup = onCleanup(@() setenv('PATH',previous_env)); %#ok<NASGU>
setenv('PATH',[fileparts(compiler) pathsep previous_env]);
compiler_version = run_checked([quote_path(compiler) ' --version'],'编译器检查');
base = [quote_path(compiler) ' -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -static -I ' ...
    quote_path(fullfile(repo,'include')) ' ' quote_path(core_file) ' '];
test_exe = fullfile(output_dir,'reduced_linear_eso_test.exe');
runner_exe = fullfile(output_dir,'control_example.exe');
run_checked([base quote_path(fullfile(repo,'tests','reduced_linear_eso_test.cpp')) ...
    ' -o ' quote_path(test_exe)],'降阶LESO主机测试编译');
host_test_output = run_checked(quote_path(test_exe),'降阶LESO主机测试');
run_checked([base quote_path(fullfile(repo,'examples','23_reduced_ladrc_integration','control_example.cpp')) ...
    ' -o ' quote_path(runner_exe)],'C++完整闭环示例编译');

%% 目标、负载、噪声是输入；CSV里不提供对象测量或观测状态
b0_f = single(p.b0); wc_f = single(p.omega_c); wo_f = single(p.omega_o);
dt_f = single(p.nominal_h); limit_f = single(p.input_limit);
N = round(p.end_time/p.nominal_h)+1;
time = (0:N-1)'*double(dt_f);
sample = (0:N-1)';
target = repmat(single(p.initial_y),N,1);
target(sample >= round(p.first_target_time/p.nominal_h)) = single(p.first_target);
target(sample >= round(p.second_target_time/p.nominal_h)) = single(p.second_target);
force = single(p.load_value)*single(sample >= round(p.load_time/p.nominal_h));
rng_before = rng;
stream = RandStream('mt19937ar','Seed',p.seed);
noise = p.noise_sigma*randn(stream,N,1);
conditions = ["clean_unlimited","clean_limited","noisy_limited"];
runs = cell(3,1); doubles = cell(3,1); comparisons = cell(3,1);
single_errors = zeros(3,5); independent_errors = zeros(3,3);
recurrence_max = zeros(3,1); source_control_max = 0;
metrics_values = zeros(3,7);
for scenario = 1:3
    current_noise = noise*double(scenario == 3);
    current_limit = single(0);
    if scenario ~= 1, current_limit = limit_f; end
    fixture = fullfile(output_dir,char(conditions(scenario)+"_input.csv"));
    cpp_file = fullfile(output_dir,char(conditions(scenario)+"_cpp.csv"));
    write_fixture(fixture,time,target,force,current_noise,dt_f);
    args = sprintf(' %.9g %.9g %.9g %.9g %.9g %.9g', ...
        b0_f,wc_f,wo_f,current_limit,single(p.initial_y),single(p.initial_f_hat));
    run_checked([quote_path(runner_exe) ' ' quote_path(fixture) ' ' quote_path(cpp_file) args], ...
        ['C++自主闭环：' char(conditions(scenario))]);
    C = readtable(cpp_file);
    % 9位十进制日志保证float往返；先恢复float值，再转double做物理量比较。
    % 不能直接将readtable解析的十进制double与精确float边界比较。
    float_columns = {'target','disturbance','dt','measurement','q_before','f_hat', ...
        'requested_input','applied_input','observer_input','q_after'};
    C{:,float_columns} = double(single(C{:,float_columns}));
    assert(height(C) == N && isequal(C.time,time) && isequal(single(C.target),target) ...
        && isequal(single(C.disturbance),force) && isequal(C.noise,current_noise) ...
        && all(single(C.dt) == dt_f), 'ADRC:InputMismatch','C++输入或时间标签不一致。');

    %% 第一层：使用C++当前测量，独立single回放状态、控制与输出处理
    qs = single(p.initial_f_hat)-wo_f*single(C.measurement(1));
    replay = zeros(N,5,'single');
    for k = 1:N
        measurement = single(C.measurement(k));
        fs = qs+wo_f*measurement;
        requested = adrc_control(target(k),[measurement;fs],b0_f,wc_f);
        applied = requested;
        if current_limit > 0, applied = max(-current_limit,min(current_limit,requested)); end
        [next_q,checked_f] = leso_reduced_step(qs,applied,measurement,b0_f,wo_f,dt_f);
        assert(checked_f == fs,'ADRC:Reconstruction','当前估计重构不一致。');
        replay(k,:) = [qs,fs,requested,applied,next_q];
        qs = next_q;
    end
    cpp_states = single(C{:,{'q_before','f_hat','requested_input','applied_input','q_after'}});
    single_errors(scenario,:) = max(abs(double(cpp_states)-double(replay)),[],1);
    assert(isequal(cpp_states,replay),'ADRC:FloatMismatch','C++与MATLAB single逐拍不一致。');
    assert(isequal(single(C.q_after(1:end-1)),single(C.q_before(2:end))) ...
        && isequal(C.y_next(1:end-1),C.true_y(2:end)) ...
        && isequal(single(C.observer_input),single(C.applied_input)), ...
        'ADRC:TimeLabelMismatch','状态时序或实际输入反馈不一致。');

    %% 第二层：第21讲同一模型的独立double闭环，不使用C++测量、对象或状态
    yd = double(single(p.initial_y));
    qd = double(single(p.initial_f_hat))-double(wo_f)*(yd+current_noise(1));
    double_records = zeros(N,7);
    for k = 1:N
        md = yd+current_noise(k);
        fd = qd+double(wo_f)*md;
        ud = adrc_control(double(target(k)),[md;fd],double(b0_f),double(wc_f));
        original = ADRC_1(time(k),[double(target(k));0;md;fd],zeros(3,1), ...
            3,1,double(dt_f),2*double(wo_f),double(wo_f)^2,double(wc_f));
        source_control_max = max(source_control_max,abs(ud-original(1)/double(b0_f)));
        ad = ud;
        if current_limit > 0, ad = max(-double(current_limit),min(double(current_limit),ud)); end
        [next_qd,checked_fd] = leso_reduced_step(qd,ad,md,double(b0_f),double(wo_f),double(dt_f));
        assert(checked_fd == fd,'ADRC:Reconstruction','double当前估计重构不一致。');
        double_records(k,:) = [yd,md,qd,fd,ud,ad,next_qd];
        qd = next_qd;
        yd = yd+double(dt_f)*(double(b0_f)*ad+double(force(k)));
    end
    D = array2table(double_records,'VariableNames', ...
        {'true_y','measurement','q_before','f_hat','requested_input','applied_input','q_after'});
    expected_f = (1-double(wo_f)*double(dt_f))*D.f_hat(1:end-1) ...
        +double(wo_f)*double(dt_f)*double(force(1:end-1))+double(wo_f)*diff(current_noise);
    recurrence_max(scenario) = max(abs(D.f_hat(2:end)-expected_f));
    assert(recurrence_max(scenario) < 1e-10,'ADRC:RecurrenceMismatch','double闭环的独立递推不一致。');
    independent_errors(scenario,:) = [max(abs(C.true_y-D.true_y)), ...
        max(abs(C.applied_input-D.applied_input)),max(abs(C.f_hat-D.f_hat))];
    assert(independent_errors(scenario,1) < 2e-4 && independent_errors(scenario,2) < 5e-4 ...
        && all(isfinite(C{:,:}),'all') && all(isfinite(D{:,:}),'all'), ...
        'ADRC:ClosedLoopMismatch','独立double闭环差值超出本实验范围或存在非有限结果。');
    if current_limit > 0
        assert(max(abs(C.applied_input)) <= double(current_limit) && any(cpp_states(:,3) ~= cpp_states(:,4)), ...
            'ADRC:LimitMismatch','限幅边界或限幅场景异常。');
    end
    tail = sample >= round(p.tail_time/p.nominal_h);
    assert(abs(mean(C.applied_input(tail))+p.load_value/p.b0) < 5e-4, ...
        'ADRC:HoldingInput','末段负载维持输入异常。');
    if scenario <= 2
        assert(abs(mean(C.true_y(tail)-double(target(tail)))) < 1e-4, ...
            'ADRC:TrackingMismatch','干净组末段目标误差异常。');
    end
    saturated = cpp_states(:,3) ~= cpp_states(:,4);
    load_window = sample >= round(p.load_time/p.nominal_h) ...
        & sample < round(p.second_target_time/p.nominal_h);
    metrics_values(scenario,:) = [max(abs(C.requested_input)),max(abs(C.applied_input)), ...
        sum(double(saturated(1:end-1)).*C.dt(1:end-1)), ...
        max(abs(C.true_y(load_window)-double(target(load_window)))), ...
        mean(C.true_y(tail)-double(target(tail))),mean(C.applied_input(tail)),std(C.requested_input(tail),1)];
    runs{scenario} = C; doubles{scenario} = D;
    comparisons{scenario} = table(repmat(conditions(scenario),N,1),time,double(target), ...
        C.true_y,D.true_y,C.applied_input,D.applied_input,C.f_hat,D.f_hat, ...
        'VariableNames',{'scenario','time','target','cpp_y','double_y','cpp_applied', ...
        'double_applied','cpp_f_hat','double_f_hat'});
end
assert(source_control_max < 1e-10,'ADRC:SourceMismatch','double控制律与原ADRC_1不一致。');
assert(strcmp(source_snapshot,fileread(reference_file)) && strcmp(core_snapshot,fileread(core_file)), ...
    'ADRC:ReferenceChanged','原参考文件或既有C++核心发生变化。');
assert(isequal(rng_before,rng),'ADRC:RngChanged','全局随机数状态被改变。');
metrics = table(conditions(:),single_errors(:,1),single_errors(:,2),single_errors(:,3), ...
    single_errors(:,4),single_errors(:,5),independent_errors(:,1),independent_errors(:,2), ...
    independent_errors(:,3),metrics_values(:,1),metrics_values(:,2),metrics_values(:,3), ...
    metrics_values(:,4),metrics_values(:,5),metrics_values(:,6),metrics_values(:,7), ...
    'VariableNames',{'scenario','single_q_before_max','single_f_hat_max','single_request_max', ...
    'single_applied_max','single_q_after_max','independent_y_max','independent_applied_max', ...
    'independent_f_hat_max','peak_request','peak_applied','saturation_time_s', ...
    'load_window_peak_error','mean_tail_error','mean_tail_input','tail_request_std'});
comparison = vertcat(comparisons{:});
load_row = round(p.load_time/p.nominal_h)+1;
timing = runs{2}(load_row+(0:3),{'time','target','measurement','q_before','f_hat', ...
    'requested_input','applied_input','observer_input','q_after'});
writetable(metrics,fullfile(output_dir,'integration_metrics.csv'));
writetable(comparison,fullfile(output_dir,'integration_comparison.csv'));
writetable(timing,fullfile(output_dir,'load_step_timing.csv'));
result = struct('teaching',teaching,'parameters',p,'actual_dt',dt_f,'actual_limit',limit_f, ...
    'compiler',compiler,'compiler_version',compiler_version,'host_test_output',host_test_output, ...
    'runs',{runs},'independent_double',{doubles},'comparison',comparison,'metrics',metrics, ...
    'timing',timing,'source_control_max',source_control_max,'recurrence_max',recurrence_max, ...
    'rng_unchanged',isequal(rng_before,rng));
save(fullfile(output_dir,'integration_result.mat'),'result');
fprintf('验证通过：三条C++自主闭环共%d次更新，同测量single的状态、估计、请求和实际输入逐拍一致。\n',3*N);
fprintf('独立double闭环最大差值：输出%.5g rad/s，实际输入%.5g A。\n', ...
    max(independent_errors(:,1)),max(independent_errors(:,2)));
disp(metrics);

%% 中文图：C++真实对象轨迹与独立double轨迹，限幅与噪声分别观察
clean = runs{2}; clean_d = doubles{2}; noisy = runs{3}; noisy_d = doubles{3};
fig = figure('Name','第23讲：C++降阶一阶LADRC闭环','Color','w','Position',[70 40 1300 840]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout);
plot(ax1,time,double(target),'k--',time,clean.true_y,time,clean_d.true_y,'--','LineWidth',1.3);
title(ax1,'干净有限幅：C++自主对象跟踪目标'); ylabel(ax1,'角速度（rad/s）');
legend(ax1,'目标','C++自主闭环','独立MATLAB double','Location','best');
ax2 = nexttile(layout);
plot(ax2,time,clean.requested_input,'--',time,clean.applied_input,'LineWidth',1.2);
yline(ax2,double(limit_f),'k:','HandleVisibility','off');
yline(ax2,-double(limit_f),'k:','HandleVisibility','off');
title(ax2,'调用方限幅：观测器反馈最终实际输入'); ylabel(ax2,'输入（A）');
legend(ax2,'请求输入','实际输入','Location','best');
ax3 = nexttile(layout);
plot(ax3,time,double(force),'k--',time,clean.f_hat,time,clean_d.f_hat,'--','LineWidth',1.2);
xlim(ax3,[1.45 2.05]); title(ax3,'负载作用后：本拍扰动重构'); ylabel(ax3,'扰动估计（rad/s²）');
legend(ax3,'真实f','C++估计','独立double估计','Location','best');
ax4 = nexttile(layout);
plot(ax4,time,noisy.requested_input,time,noisy_d.requested_input,'--','LineWidth',1.0);
xlim(ax4,[4.5 4.75]); title(ax4,'带噪有限幅：末段请求包含测量波动'); ylabel(ax4,'请求输入（A）');
legend(ax4,'C++自主闭环','独立MATLAB double','Location','best');
for ax = [ax1 ax2 ax3 ax4]
    grid(ax,'on'); xlabel(ax,'实际时间（秒）');
end
title(layout,'第23讲｜C++降阶一阶LADRC：自主闭环、外部限幅与实际输入反馈');
adrc_export_figure(fig,output_dir,"reduced_ladrc_cpp_integration");
end

function output = run_checked(command,description)
[status,output] = system(command);
assert(status == 0,'ADRC:HostToolFailure','%s失败（退出码%d）：\n%s',description,status,output);
fprintf('%s：通过。\n',description);
end

function quoted = quote_path(file)
assert(~contains(file,'"') && ~contains(file,newline),'ADRC:InvalidPath','路径包含无效引号或换行。');
quoted = ['"' char(file) '"'];
end

function write_fixture(file,time,target,force,noise,dt)
fid = fopen(file,'w','n','UTF-8');
assert(fid >= 0,'ADRC:WriteFailure','无法创建输入文件。');
file_cleanup = onCleanup(@() fclose(fid)); %#ok<NASGU>
fprintf(fid,'time,target,disturbance,noise,dt\n');
values = [time,double(target),double(force),noise,repmat(double(dt),numel(time),1)];
fprintf(fid,'%.17g,%.9g,%.9g,%.17g,%.9g\n',values.');
end
