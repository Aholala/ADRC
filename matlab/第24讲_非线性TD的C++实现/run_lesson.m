%% 第24讲：非线性二阶TD的C++实现、原TD2_NL与MATLAB逐拍对齐
% 教学归属：非线性TD工程实现。本讲独立观察TD，不接控制器或对象。
% 点击“运行”（F5）执行整个文件；需要MATLAB、g++及本机ADRC1文件，无需MEX。
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
result = run_alignment(repo);

function result = run_alignment(repo)
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file = fullfile(reference_dir,'TD2_NL.m');
library_file = fullfile(reference_dir,'ADRC.mdl');
model_file = fullfile(reference_dir,'MODEL','TD2_NL_MODEL.mdl');
assert(isfile(reference_file) && isfile(library_file) && isfile(model_file), ...
    'ADRC:MissingReference','请检查ADRC1的TD2_NL代码、库模型和独立模型。');
source_snapshot = fileread(reference_file);
library_snapshot = fileread(library_file); model_snapshot = fileread(model_file);
block = regexp(library_snapshot,'FunctionName\s+"TD2_NL"(.*?)MaskValueString\s+"([^"]+)"','tokens','once');
assert(~isempty(block),'ADRC:ReferenceChanged','原库TD2_NL参数结构变化。');
values = str2double(split(string(block{2}),'|'));
assert(numel(values) == 3 && all(isfinite(values)) && all(values > 0), ...
    'ADRC:InvalidReference','原库默认r、h、h0需重新核对。');
override = regexp(model_snapshot,'(?m)^\s*h0\s+"([^"]+)"','tokens','once');
stop = regexp(model_snapshot,'StopTime\s+"([^"]+)"','tokens','once');
sine = regexp(model_snapshot,'Name\s+"Sine Wave".*?Frequency\s+"([^"]+)"','tokens','once');
assert(~isempty(override) && ~isempty(stop) && ~isempty(sine), ...
    'ADRC:ReferenceChanged','原独立模型参数结构变化。');
frequency_text = strtrim(sine{1});
if strcmp(frequency_text,'pi/2'), sine_frequency = pi/2;
else, sine_frequency = str2double(frequency_text); end
p = struct('r',values(1),'nominal_h',values(2),'h0_base',values(3), ...
    'h0_values',values(3)*[1 2 4],'end_time',str2double(stop{1}), ...
    'constant_amplitude',1,'sine_frequency',sine_frequency,'model_h0_override',str2double(override{1}));
assert(all(isfinite([p.r p.nominal_h p.h0_values p.end_time p.sine_frequency p.model_h0_override])) ...
    && p.end_time > 0 && p.sine_frequency > 0,'ADRC:InvalidReference','参考参数非有限或无效。');
teaching = struct('lesson','第24讲','algorithm','C++非线性二阶跟踪微分器', ...
    'topic','原fhan分支、旧状态欧拉更新、实际h与内部h0的区分', ...
    'document','docs/24_非线性TD的C++实现.md','reference_code',reference_file, ...
    'reference_library',library_file,'reference_model',model_file, ...
    'parameter_source','原库正值默认参数；另比较2倍与4倍h0，不复现零h0覆盖配置');
fprintf('\n【第24讲：非线性TD的最小C++实现】\n');
fprintf('库基准r=%g、标称h=%g秒、h0=%g秒；独立模型保存的h0覆盖值=%g。\n', ...
    p.r,p.nominal_h,p.h0_base,p.model_h0_override);
fprintf('r是内部加速度尺度，不是线性TD的频率，也不是速度上限。\n');
addpath(reference_dir,'-begin');
assert(strcmpi(which('TD2_NL'),reference_file),'ADRC:WrongReference','当前TD2_NL不是指定原文件。');
if p.model_h0_override == 0
    singular = TD2_NL(0,[0;0],0,2,p.r,p.nominal_h,0);
    assert(any(~isfinite(singular)),'ADRC:ReferenceChanged','原零h0平衡点行为发生变化。');
    fprintf('本讲采用原库正值h0；原独立模型的零h0覆盖值只记录，不代入库配置。\n');
end

%% 自动编译核心、主机测试与CSV逐拍驱动
compiler = getenv('ADRC_CXX');
if isempty(compiler)
    [status,found] = system('where g++');
    if status == 0
        candidates = splitlines(strtrim(string(found))); compiler = char(candidates(1));
    elseif isfile('D:/msys64/ucrt64/bin/g++.exe')
        compiler = 'D:/msys64/ucrt64/bin/g++.exe';
    end
end
assert(isfile(compiler),'ADRC:MissingCompiler','未找到g++，请设置ADRC_CXX为完整编译器路径。');
previous_env = getenv('PATH');
env_cleanup = onCleanup(@() setenv('PATH',previous_env)); %#ok<NASGU>
setenv('PATH',[fileparts(compiler) pathsep previous_env]);
compiler_version = run_checked([quote_path(compiler) ' --version'],'编译器检查');
base = [quote_path(compiler) ' -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -static -I ' ...
    quote_path(fullfile(repo,'include')) ' ' quote_path(fullfile(repo,'src','nltd2.cpp')) ' '];
output_dir = fullfile(repo,'build','lesson24');
if ~isfolder(output_dir), mkdir(output_dir); end
test_exe = fullfile(output_dir,'nltd2_test.exe');
runner_exe = fullfile(output_dir,'td_runner.exe');
run_checked([base quote_path(fullfile(repo,'tests','nltd2_test.cpp')) ' -o ' quote_path(test_exe)], ...
    'C++分支、手算和解析阶段测试编译');
host_test_output = run_checked(quote_path(test_exe),'C++主机测试');
run_checked([base quote_path(fullfile(repo,'examples','24_nltd_alignment','td_runner.cpp')) ...
    ' -o ' quote_path(runner_exe)],'C++逐拍驱动编译');

%% 恒值/正弦各比较三个h0，time按实际float周期累计
dt_f = single(p.nominal_h); r_f = single(p.r);
N = round(p.end_time/p.nominal_h)+1;
time = (0:N-1)'*double(dt_f);
signals = single([p.constant_amplitude*ones(N,1),sin(p.sine_frequency*time)]);
signal_names = ["constant","sine"];
parts = cell(6,1); cpp_runs = cell(2,3); original_runs = cell(2,3);
single_errors = zeros(6,3); original_errors = zeros(6,3); peaks = zeros(6,2);
case_names = strings(6,1); h0_column = zeros(6,1);
for scenario = 1:2
    for scale = 1:3
        row = (scenario-1)*3+scale;
        h0_f = single(p.h0_values(scale));
        case_names(row) = signal_names(scenario)+"_h0x"+string(2^(scale-1));
        h0_column(row) = double(h0_f);
        fixture = fullfile(output_dir,char(case_names(row)+"_input.csv"));
        cpp_file = fullfile(output_dir,char(case_names(row)+"_cpp.csv"));
        write_fixture(fixture,time,signals(:,scenario),dt_f);
        args = sprintf(' %.9g %.9g 0 0',r_f,h0_f);
        run_checked([quote_path(runner_exe) ' ' quote_path(fixture) ' ' quote_path(cpp_file) args], ...
            ['C++执行：' char(case_names(row))]);
        C = readtable(cpp_file);
        C{:,2:end} = double(single(C{:,2:end})); % float日志往返恢复，time保留double。
        assert(height(C) == N && isequal(C.time,time) && isequal(single(C.input),signals(:,scenario)) ...
            && all(single(C.dt) == dt_f),'ADRC:InputMismatch','C++输入、时间或周期不一致。');
        cpp = single(C{:,4:8});
        replay = zeros(N,5,'single'); original = zeros(N,5);
        xs = zeros(2,1,'single'); xd = zeros(2,1);
        for k = 1:N
            before_s = xs; before_d = xd;
            [xs,acceleration] = nltd_step(xs,signals(k,scenario),r_f,dt_f,h0_f);
            xd = TD2_NL(time(k),xd,double(signals(k,scenario)),2,double(r_f),double(dt_f),double(h0_f));
            xd = xd(:);
            source_acceleration = (xd(2)-before_d(2))/double(dt_f);
            replay(k,:) = [before_s',acceleration,xs'];
            original(k,:) = [before_d',source_acceleration,xd'];
        end
        single_errors(row,:) = [max(abs(double(cpp(:,[1 4]))-double(replay(:,[1 4]))),[],'all'), ...
            max(abs(double(cpp(:,[2 5]))-double(replay(:,[2 5]))),[],'all'), ...
            max(abs(double(cpp(:,3))-double(replay(:,3))))];
        original_errors(row,:) = [max(abs(double(cpp(:,[1 4]))-original(:,[1 4])),[],'all'), ...
            max(abs(double(cpp(:,[2 5]))-original(:,[2 5])),[],'all'), ...
            max(abs(double(cpp(:,3))-original(:,3)))];
        assert(isequal(cpp,replay),'ADRC:FloatMismatch','C++与MATLAB single逐拍不一致。');
        assert(isequal(cpp(1:end-1,4:5),cpp(2:end,1:2)), ...
            'ADRC:StateTiming','当前状态和下一拍预测不连续。');
        assert(all(isfinite(C{:,:}),'all') && max(original_errors(row,1:2)) < 2e-4 ...
            && original_errors(row,3) < 0.02,'ADRC:OriginalMismatch','原函数double对照超出本实验范围。');
        assert(max(abs(C.acceleration)) <= double(r_f)+1e-4 ...
            && max(abs(C.x2_after-C.x2_before)) <= double(dt_f)*double(r_f)+1e-5, ...
            'ADRC:AccelerationBound','加速度或单步变化率界限异常。');
        if scenario == 1
            assert(abs(C.x1_before(end)-p.constant_amplitude) < 3e-5 && abs(C.x2_before(end)) < 3e-4, ...
                'ADRC:ConstantTracking','恒值输入没有在观察区间内收敛。');
        end
        peaks(row,:) = [max(abs(C.x2_before)),max(abs(C.acceleration))];
        cpp_runs{scenario,scale} = C; original_runs{scenario,scale} = original;
        parts{row} = table(repmat(case_names(row),N,1),repmat(double(h0_f),N,1),time, ...
            C.input,C.x1_before,C.x2_before,C.acceleration,original(:,1),original(:,2),original(:,3), ...
            'VariableNames',{'scenario','h0','time','input','cpp_x1','cpp_x2','cpp_acceleration', ...
            'original_double_x1','original_double_x2','original_double_acceleration'});
    end
end
assert(strcmp(source_snapshot,fileread(reference_file)) && strcmp(library_snapshot,fileread(library_file)) ...
    && strcmp(model_snapshot,fileread(model_file)), ...
    'ADRC:ReferenceChanged','原参考代码或模型发生变化。');
metrics = table(case_names,h0_column,single_errors(:,1),single_errors(:,2),single_errors(:,3), ...
    original_errors(:,1),original_errors(:,2),original_errors(:,3),peaks(:,1),peaks(:,2), ...
    'VariableNames',{'scenario','h0','single_x1_max','single_x2_max','single_acceleration_max', ...
    'original_double_x1_max','original_double_x2_max','original_double_acceleration_max','peak_rate','peak_acceleration'});
comparison = vertcat(parts{:});
writetable(metrics,fullfile(output_dir,'alignment_metrics.csv'));
writetable(comparison,fullfile(output_dir,'nltd_comparison.csv'));
result = struct('teaching',teaching,'parameters',p,'actual_dt',dt_f, ...
    'compiler',compiler,'compiler_version',compiler_version,'host_test_output',host_test_output, ...
    'cpp',{cpp_runs},'original_double',{original_runs},'metrics',metrics,'comparison',comparison);
save(fullfile(output_dir,'alignment_result.mat'),'result');
fprintf('验证通过：六组共%d次更新，两个状态更新前后与内部加速度均和single逐拍一致。\n',6*N);
fprintf('原TD2_NL double最大差值：x1 %.5g、x2 %.5g、内部加速度 %.5g。\n',max(original_errors,[],1));
disp(metrics);

%% 中文图：固定实际周期，比较内部h0；恒值输入不伪造有限导数
fig = figure('Name','第24讲：C++非线性TD与原函数对齐','Color','w','Position',[60 40 1340 820]);
layout = tiledlayout(fig,2,3,'TileSpacing','compact');
colors = [0.16 0.52 0.36;0.26 0.39 0.67;0.82 0.42 0.15];
for scenario = 1:2
    ax1 = nexttile(layout); hold(ax1,'on');
    ax2 = nexttile(layout); hold(ax2,'on');
    ax3 = nexttile(layout); hold(ax3,'on');
    plot(ax1,time,double(signals(:,scenario)),'k:','DisplayName','输入信号');
    if scenario == 2
        plot(ax2,time,p.sine_frequency*cos(p.sine_frequency*time),'k:','DisplayName','输入解析导数');
    end
    for scale = 1:3
        C = cpp_runs{scenario,scale};
        label = sprintf('C++ h0=%g秒',p.h0_values(scale));
        plot(ax1,time,C.x1_before,'Color',colors(scale,:),'LineWidth',1.2,'DisplayName',label);
        plot(ax2,time,C.x2_before,'Color',colors(scale,:),'LineWidth',1.2,'DisplayName',label);
        plot(ax3,time,C.acceleration,'Color',colors(scale,:),'LineWidth',1.1,'DisplayName',label);
    end
    original = original_runs{scenario,1};
    plot(ax1,time,original(:,1),'k--','DisplayName','原函数double（基准h0）');
    plot(ax2,time,original(:,2),'k--','DisplayName','原函数double（基准h0）');
    yline(ax3,p.r,'k:','HandleVisibility','off'); yline(ax3,-p.r,'k:','HandleVisibility','off');
    if scenario == 1
        title(ax1,'恒值：平滑输出x1'); title(ax2,'恒值：先加速再制动'); title(ax3,'恒值：内部加速度fh');
        window = [0 1.5];
    else
        title(ax1,'正弦：平滑与滞后'); title(ax2,'正弦：变化率与输入导数'); title(ax3,'正弦：内部加速度fh');
        window = [0 8];
    end
    ylabel(ax1,'输入单位'); ylabel(ax2,'输入单位/秒'); ylabel(ax3,'输入单位/秒²');
    for ax = [ax1 ax2 ax3]
        xlim(ax,window); grid(ax,'on'); xlabel(ax,'实际时间（秒）'); legend(ax,'show','Location','best','FontSize',8);
    end
end
title(layout,'第24讲｜C++非线性二阶TD：原fhan、固定实际h与不同内部h0');
adrc_export_figure(fig,output_dir,"nltd_cpp_alignment");
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

function write_fixture(file,time,input,dt)
fid = fopen(file,'w','n','UTF-8');
assert(fid >= 0,'ADRC:WriteFailure','无法创建输入文件。');
file_cleanup = onCleanup(@() fclose(fid)); %#ok<NASGU>
fprintf(fid,'time,input,dt\n');
values = [time,double(input),repmat(double(dt),numel(time),1)];
fprintf(fid,'%.17g,%.9g,%.9g\n',values.');
end
