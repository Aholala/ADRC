%% 第22讲：一阶对象降阶LESO的最小C++实现与MATLAB逐拍对齐
% 教学归属：工程实现；只加入独立降阶观测器，不加入控制器封装。
% 点击“运行”（F5）。需要MATLAB、g++和本机ADRC1参考文件，无需MEX。
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
result = run_alignment(repo);

function result = run_alignment(repo)
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(fullfile(repo,'matlab','内部函数'));
output_dir = fullfile(repo,'build','lesson22');
if ~isfolder(output_dir), mkdir(output_dir); end
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file = fullfile(reference_dir,'ADRC_1.m');
template_file = fullfile(reference_dir,'ESO_1rd.m');
formula_file = fullfile(repo,'matlab','内部函数','leso_reduced_step.m');
assert(isfile(reference_file) && isfile(template_file), ...
    'ADRC:MissingReference','请检查ADRC1中的ADRC_1.m和ESO_1rd.m。');
source_snapshot = fileread(reference_file); template_snapshot = fileread(template_file);
formula_snapshot = fileread(formula_file);
teaching = struct('lesson','第22讲','algorithm','C++一阶对象的降阶LESO', ...
    'topic','当前测量重构与实际输入更新分开；更新前后q和本拍扰动估计核对', ...
    'document','docs/22_降阶LESO的C++实现.md','formula_file',formula_file, ...
    'reference_code',reference_file,'empty_reference_template',template_file, ...
    'parameter_source','沿用第20讲教学配置；不是原完整模型或实机参数');
p = struct('b0',10,'omega_o',20,'nominal_h',0.001,'end_time',3, ...
    'initial_y',3,'initial_f_hat',0,'input_time',0.5,'input_value',0.3, ...
    'load_time',1.5,'load_value',-2,'noise_sigma',0.001,'seed',20261028);
fprintf('\n【第22讲：降阶LESO的最小C++实现】\n');
fprintf('Calc使用当前测量重构本拍扰动，Update用实际输入预测下一拍q。\n');
fprintf('原ESO_1rd.m为空模板；本讲对齐第20讲推导公式，不冒充原完整模型复现。\n');

%% 编译相关核心与主机测试，不需要MEX；退出时恢复PATH
compiler = getenv('ADRC_CXX');
if isempty(compiler)
    [status,found] = system('where g++');
    if status == 0
        candidates = splitlines(strtrim(string(found))); compiler = char(candidates(1));
    elseif isfile('D:/msys64/ucrt64/bin/g++.exe')
        compiler = 'D:/msys64/ucrt64/bin/g++.exe';
    end
end
assert(isfile(compiler),'ADRC:MissingCompiler','未找到g++，请将ADRC_CXX设为完整编译器路径。');
previous_env_path = getenv('PATH');
env_cleanup = onCleanup(@() setenv('PATH',previous_env_path)); %#ok<NASGU>
setenv('PATH',[fileparts(compiler) pathsep previous_env_path]);
compiler_version = run_checked([quote_path(compiler) ' --version'],'编译器检查');
base = [quote_path(compiler) ' -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -static -I ' ...
    quote_path(fullfile(repo,'include')) ' ' quote_path(fullfile(repo,'src','adrc.cpp')) ' '];
test_exe = fullfile(output_dir,'rleso1_test.exe');
runner_exe = fullfile(output_dir,'reduced_leso_runner.exe');
run_checked([base quote_path(fullfile(repo,'tests','rleso1_test.cpp')) ...
    ' -o ' quote_path(test_exe)],'C++解析响应与接口测试编译');
host_test_output = run_checked(quote_path(test_exe),'C++主机测试');
run_checked([base quote_path(fullfile(repo,'examples','22_reduced_leso_alignment','reduced_leso_runner.cpp')) ...
    ' -o ' quote_path(runner_exe)],'C++逐拍驱动编译');

%% 独立对象与同输入回放：float周期的实际累积时间与测量量化明确记录
b0_f = single(p.b0); omega_f = single(p.omega_o); dt_f = single(p.nominal_h);
N = round(p.end_time/p.nominal_h)+1;
time = (0:N-1)'*double(dt_f);
input_row = round(p.input_time/p.nominal_h)+1;
load_row = round(p.load_time/p.nominal_h)+1;
applied = zeros(N,1,'single'); applied(input_row:end) = single(p.input_value);
true_f = zeros(N,1); true_f(load_row:end) = p.load_value;
true_y = zeros(N,1); true_y(1) = p.initial_y;
for k = 1:N-1
    true_y(k+1) = true_y(k)+double(dt_f)*(double(b0_f)*double(applied(k))+true_f(k));
end
analytic_y = p.initial_y+double(b0_f)*double(single(p.input_value))*max(time-time(input_row),0) ...
    +p.load_value*max(time-time(load_row),0);
plant_max = max(abs(true_y-analytic_y));
assert(plant_max < 1e-10,'ADRC:PlantMismatch','对象与独立分段解析解不一致。');
rng_before = rng;
stream = RandStream('mt19937ar','Seed',p.seed);
noise = p.noise_sigma*randn(stream,N,1);
case_names = ["clean","noisy"];
single_errors = zeros(2,3); double_errors = zeros(2,2); recurrence_max = zeros(2,1);
cpp_parts = cell(2,1); comparison_parts = cell(2,1);
for scenario = 1:2
    measurement = single(true_y+noise*double(scenario == 2));
    fixture = fullfile(output_dir,char(case_names(scenario)+"_input.csv"));
    cpp_file = fullfile(output_dir,char(case_names(scenario)+"_cpp.csv"));
    write_fixture(fixture,time,applied,measurement,dt_f);
    args = sprintf(' %.9g %.9g %.9g %.9g',b0_f,omega_f,measurement(1),single(p.initial_f_hat));
    run_checked([quote_path(runner_exe) ' ' quote_path(fixture) ' ' quote_path(cpp_file) args], ...
        ['C++执行：' char(case_names(scenario))]);
    C = readtable(cpp_file);
    assert(height(C) == N && isequal(C.time,time) && isequal(single(C.input),applied) ...
        && isequal(single(C.measurement),measurement) && all(single(C.dt) == dt_f), ...
        'ADRC:InputMismatch','C++收到的时间、输入、测量或周期不一致。');
    cpp = single(C{:,5:7}); % q_before、f_hat[k]、q_after。
    replay_single = zeros(N,3,'single'); replay_double = zeros(N,3);
    qs = single(p.initial_f_hat)-omega_f*measurement(1);
    qd = double(single(p.initial_f_hat))-double(omega_f)*double(measurement(1));
    for k = 1:N
        before_s = qs; before_d = qd;
        [qs,fs] = leso_reduced_step(qs,applied(k),measurement(k),b0_f,omega_f,dt_f);
        [qd,fd] = leso_reduced_step(qd,double(applied(k)),double(measurement(k)), ...
            double(b0_f),double(omega_f),double(dt_f));
        replay_single(k,:) = [before_s,fs,qs];
        replay_double(k,:) = [before_d,fd,qd];
    end
    single_errors(scenario,:) = max(abs(double(cpp)-double(replay_single)),[],1);
    double_errors(scenario,:) = [max(abs(double(cpp(:,[1 3]))-replay_double(:,[1 3])),[],'all'), ...
        max(abs(double(cpp(:,2))-replay_double(:,2)))];
    assert(isequal(cpp,replay_single),'ADRC:FloatMismatch','C++与MATLAB single逐拍不一致。');
    assert(isequal(cpp(1:end-1,3),cpp(2:end,1)), ...
        'ADRC:StateTiming','q_after与下一行q_before不一致。');
    assert(all(isfinite(C{:,:}),'all') && max(double_errors(scenario,:)) < 2e-3, ...
        'ADRC:DoubleMismatch','与double参考差值超出本实验范围或结果非有限。');
    % 消去q的独立递推包含测量量化；clean仅表示未额外添加随机噪声。
    effective_noise = double(measurement)-true_y;
    expected_next = (1-double(omega_f)*double(dt_f))*replay_double(1:end-1,2) ...
        +double(omega_f)*double(dt_f)*true_f(1:end-1)+double(omega_f)*diff(effective_noise);
    recurrence_max(scenario) = max(abs(replay_double(2:end,2)-expected_next));
    assert(recurrence_max(scenario) < 1e-10, ...
        'ADRC:RecurrenceMismatch','独立离散递推与double参考不一致。');
    assert(abs(double(cpp(1,2))-p.initial_f_hat) < 5e-5, ...
        'ADRC:InitialMismatch','非零初态对齐结果异常。');
    comparison_parts{scenario} = table(repmat(case_names(scenario),N,1),time,true_y,true_f, ...
        double(measurement),double(cpp(:,1)),double(cpp(:,2)),double(cpp(:,3)), ...
        replay_double(:,1),replay_double(:,2),replay_double(:,3), ...
        'VariableNames',{'scenario','time','true_y','true_f','measurement', ...
        'cpp_q_before','cpp_f_hat','cpp_q_after','double_q_before','double_f_hat','double_q_after'});
    cpp_parts{scenario} = C;
end
assert(strcmp(source_snapshot,fileread(reference_file)) && strcmp(template_snapshot,fileread(template_file)) ...
    && strcmp(formula_snapshot,fileread(formula_file)), ...
    'ADRC:ReferenceChanged','参考文件或已学MATLAB公式发生变化。');
assert(isequal(rng_before,rng),'ADRC:RngChanged','全局随机数状态被改变。');
comparison = vertcat(comparison_parts{:});
metrics = table(case_names(:),single_errors(:,1),single_errors(:,2),single_errors(:,3), ...
    double_errors(:,1),double_errors(:,2),recurrence_max,'VariableNames', ...
    {'scenario','single_q_before_max','single_f_hat_max','single_q_after_max', ...
    'double_q_max','double_f_hat_max','double_recurrence_max'});
writetable(comparison,fullfile(output_dir,'reduced_leso_comparison.csv'));
writetable(metrics,fullfile(output_dir,'alignment_metrics.csv'));
result = struct('teaching',teaching,'parameters',p,'actual_dt',dt_f, ...
    'compiler',compiler,'compiler_version',compiler_version,'host_test_output',host_test_output, ...
    'cpp',{cpp_parts},'comparison',comparison,'metrics',metrics,'plant_max',plant_max, ...
    'rng_unchanged',isequal(rng_before,rng));
save(fullfile(output_dir,'alignment_result.mat'),'result');
fprintf('验证通过：两组共%d次更新，q更新前后与本拍扰动估计均与single逐拍一致。\n',2*N);
fprintf('相同float输入下与double参考的最大差值：q %.5g、扰动估计 %.5g。\n', ...
    max(double_errors(:,1)),max(double_errors(:,2)));
disp(metrics);

%% 中文图：独立动态状态、当前估计与数值误差分开观察
clean = comparison_parts{1}; noisy = comparison_parts{2};
fig = figure('Name','第22讲：C++降阶LESO逐拍对齐','Color','w','Position',[70 40 1270 820]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout);
plot(ax1,time,true_f,'k--',time,clean.cpp_f_hat,time,clean.double_f_hat,'--','LineWidth',1.3);
xlim(ax1,[1.45 1.95]); title(ax1,'无附加噪声：本拍扰动估计'); ylabel(ax1,'扰动（rad/s²）');
legend(ax1,'真实f','C++ float估计','MATLAB double参考','Location','best');
ax2 = nexttile(layout);
plot(ax2,time,clean.cpp_q_before,time,clean.double_q_before,'--','LineWidth',1.3);
title(ax2,'内部状态q：它本身不是扰动估计'); ylabel(ax2,'辅助状态（rad/s²）');
legend(ax2,'C++ float的q','MATLAB double的q','Location','best');
ax3 = nexttile(layout);
plot(ax3,time,true_f,'k--',time,noisy.cpp_f_hat,time,noisy.double_f_hat,'--','LineWidth',1.1);
xlim(ax3,[2.5 2.75]); title(ax3,'带噪组：当前测量直接影响估计'); ylabel(ax3,'扰动（rad/s²）');
legend(ax3,'真实f','C++ float估计','MATLAB double参考','Location','best');
ax4 = nexttile(layout);
plot(ax4,time,clean.cpp_f_hat-clean.double_f_hat,time,noisy.cpp_f_hat-noisy.double_f_hat,'LineWidth',1.1);
title(ax4,'C++与double的差值：不是物理估计误差'); ylabel(ax4,'数值差值（rad/s²）');
legend(ax4,'无附加噪声','带噪组','Location','best');
for ax = [ax1 ax2 ax3 ax4]
    grid(ax,'on'); xlabel(ax,'时间（秒）');
end
title(layout,'第22讲｜C++一阶降阶LESO：Calc当前重构与Update状态推进分别核对');
adrc_export_figure(fig,output_dir,"reduced_leso_cpp_alignment");
end

function output = run_checked(command,description)
[status,output] = system(command);
assert(status == 0,'ADRC:HostToolFailure','%s失败（退出码%d）：\n%s',description,status,output);
fprintf('%s：通过。\n',description);
end

function quoted = quote_path(file)
assert(~contains(file,'"') && ~contains(file,newline), ...
    'ADRC:InvalidPath','路径包含无效引号或换行。');
quoted = ['"' char(file) '"'];
end

function write_fixture(file,time,input,measurement,dt)
fid = fopen(file,'w','n','UTF-8');
assert(fid >= 0,'ADRC:WriteFailure','无法创建输入文件。');
file_cleanup = onCleanup(@() fclose(fid)); %#ok<NASGU>
fprintf(fid,'time,input,measurement,dt\n');
values = [time,double(input),double(measurement),repmat(double(dt),numel(time),1)];
fprintf(fid,'%.17g,%.9g,%.9g,%.9g\n',values.');
end
