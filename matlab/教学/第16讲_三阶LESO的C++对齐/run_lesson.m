%% 第16讲：三阶LESO的最小C++实现与MATLAB逐拍对齐
% 教学归属：工程实现；本讲只实现三阶LESO，不增加C++控制律或TD。
% 直接点击“运行”（F5）。需要MATLAB、本机g++和ADRC1参考参数。
% 本入口自动编译C++测试与数据驱动程序，不需要配置MEX。
repo = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
addpath(fullfile(repo,'matlab','内部函数'));
lesson16 = run_alignment(repo);

function result = run_alignment(repo)
output_dir = fullfile(repo,'build','lesson16');
if ~isfolder(output_dir), mkdir(output_dir); end
reference_file = fullfile('D:/ACE/ADRC/ADRC1','LADRC实现参考', ...
    'MATLABDemo-LADRC 实现(高志强老师)','LADRC Demo','tuning.m');
assert(isfile(reference_file),'ADRC:MissingReference','原tuning.m不存在。');
reference_snapshot = fileread(reference_file);
original = struct;
for name = {'h','b0','wo'}
    token = regexp(reference_snapshot,['(?m)^\s*' name{1} '\s*=\s*([^;]+);'],'tokens','once');
    assert(~isempty(token),'ADRC:MissingParameter','原参数未找到。');
    original.(name{1}) = str2double(strtrim(token{1}));
end
p = struct('h',original.h,'b0',original.b0,'omega_o',original.wo, ...
    'end_time',3,'input_time',0.5,'input_value',0.075,'load_time',1.5, ...
    'load_value',-2,'noise_sigma',0.001,'noise_seed',20261028);
teaching = struct('lesson','第16讲','algorithm','C++三阶LESO', ...
    'topic','同一份float输入，C++与MATLAB单精度和双精度逐拍比较', ...
    'reference_parameters',reference_file,'document','docs/16_三阶LESO的C++实现.md');
fprintf('\n【第16讲：三阶LESO的C++实现与MATLAB逐拍对齐】\n');

%% 编译器路径与运行依赖只在本函数内临时设置，成功或失败都会恢复PATH
compiler = getenv('ADRC_CXX');
if isempty(compiler)
    [status,found] = system('where g++');
    if status==0
        candidates = splitlines(strtrim(string(found)));
        compiler = char(candidates(1));
    elseif isfile('D:/msys64/ucrt64/bin/g++.exe')
        compiler = 'D:/msys64/ucrt64/bin/g++.exe'; % 本机已安装位置的后备探测。
    end
end
assert(isfile(compiler),'ADRC:MissingCompiler', ...
    '未找到g++。请安装编译器，或设置ADRC_CXX为g++.exe完整路径。');
previous_path = getenv('PATH');
path_cleanup = onCleanup(@() setenv('PATH',previous_path)); %#ok<NASGU>
setenv('PATH',[fileparts(compiler) pathsep previous_path]);
compiler_version = run_checked([quote_path(compiler) ' --version'],'编译器检查');
flags = ' -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -static ';
base_command = [quote_path(compiler) flags '-I ' quote_path(fullfile(repo,'main')) ...
    ' ' quote_path(fullfile(repo,'main','ALG_ADRC.cpp')) ' '];
test_exe = fullfile(output_dir,'leso3_test.exe');
runner_exe = fullfile(output_dir,'leso3_runner.exe');
run_checked([base_command quote_path(fullfile(repo,'tests','leso3_test.cpp')) ...
    ' -o ' quote_path(test_exe)],'C++主机测试编译');
host_test_output = run_checked(quote_path(test_exe),'C++解析响应与接口边界测试');
run_checked([base_command quote_path(fullfile(repo,'examples','16_leso3_alignment','leso3_runner.cpp')) ...
    ' -o ' quote_path(runner_exe)],'C++数据驱动程序编译');

%% 数据由MATLAB生成；先量化为float，确保C++和两个参考使用相同输入
time = (0:round(p.end_time/p.h))'*p.h;
input_values = single(p.input_value*double(time>=p.input_time));
true_f = p.load_value*double(time>=p.load_time);
tau_input = max(time-p.input_time,0); tau_load = max(time-p.load_time,0);
true_y = 0.5*p.b0*p.input_value*tau_input.^2+0.5*p.load_value*tau_load.^2;
true_velocity = p.b0*p.input_value*tau_input+p.load_value*tau_load;
stream = RandStream('mt19937ar','Seed',p.noise_seed);
noise = p.noise_sigma*randn(stream,numel(time),1);
measurements = single([true_y,true_y+noise]);
b0_f=single(p.b0); omega_f=single(p.omega_o); dt_f=single(p.h);
omega_squared=omega_f*omega_f;
beta1_f=single(3)*omega_f; beta2_f=single(3)*omega_squared; beta3_f=omega_squared*omega_f;
case_names=["clean","noisy"];
cpp_before_all=zeros(numel(time),3,2);
double_before_all=zeros(numel(time),3,2);
single_max_errors=zeros(2,3); double_max_errors=zeros(2,3);
comparison_parts=cell(2,1);
for scenario=1:2
    fixture=fullfile(output_dir,char(case_names(scenario)+"_input.csv"));
    cpp_file=fullfile(output_dir,char(case_names(scenario)+"_cpp.csv"));
    write_fixture(fixture,time,input_values,measurements(:,scenario),dt_f);
    args=sprintf(' %.9g %.9g 0 0 0',b0_f,omega_f);
    run_checked([quote_path(runner_exe) ' ' quote_path(fixture) ' ' quote_path(cpp_file) args], ...
        ['C++逐拍执行：' char(case_names(scenario))]);
    C=readtable(cpp_file);
    assert(height(C)==numel(time),'ADRC:RowMismatch','C++输出行数不一致。');
    assert(isequal(single(C.input),input_values) && isequal(single(C.measurement),measurements(:,scenario)) ...
        && all(single(C.dt)==dt_f) && isequal(C.time,time), ...
        'ADRC:InputMismatch','C++接收的输入、测量、周期或时间标签不同。');
    cpp_before=single(C{:,5:7}); cpp_after=single(C{:,8:10});
    single_before=zeros(numel(time),3,'single'); single_after=single_before;
    double_before=zeros(numel(time),3); double_after=double_before;
    zs=zeros(3,1,'single'); zd=zeros(3,1);
    for k=1:numel(time)
        single_before(k,:)=zs.'; double_before(k,:)=zd.';
        zs=adrc_leso3_step(zs,input_values(k),measurements(k,scenario), ...
            b0_f,beta1_f,beta2_f,beta3_f,dt_f);
        zd=adrc_leso3_step(zd,double(input_values(k)),double(measurements(k,scenario)), ...
            double(b0_f),double(beta1_f),double(beta2_f),double(beta3_f),double(dt_f));
        single_after(k,:)=zs.'; double_after(k,:)=zd.';
    end
    single_max_errors(scenario,:)=max([abs(double(cpp_before)-double(single_before)); ...
        abs(double(cpp_after)-double(single_after))],[],1);
    double_max_errors(scenario,:)=max([abs(double(cpp_before)-double_before); ...
        abs(double(cpp_after)-double_after)],[],1);
    assert(isequal(cpp_before,single_before) && isequal(cpp_after,single_after), ...
        'ADRC:FloatMismatch','当前构建的C++与MATLAB single逐拍结果不一致，检查运算顺序与编译选项。');
    assert(isequal(cpp_before(2:end,:),cpp_after(1:end-1,:)), ...
        'ADRC:TimeLabelMismatch','更新后的状态未对应下一行的更新前状态。');
    cpp_before_all(:,:,scenario)=double(cpp_before); double_before_all(:,:,scenario)=double_before;
    comparison_parts{scenario}=table(repmat(case_names(scenario),numel(time),1),time, ...
        double(cpp_before(:,1)),double(cpp_before(:,2)),double(cpp_before(:,3)), ...
        double_before(:,1),double_before(:,2),double_before(:,3), ...
        'VariableNames',{'scenario','time','cpp_z1','cpp_z2','cpp_z3','double_z1','double_z2','double_z3'});
end
comparison=vertcat(comparison_parts{:});
assert(all(isfinite(comparison{:,2:end}),'all'),'ADRC:NonfiniteResult','对照结果存在非有限值。');
assert(strcmp(reference_snapshot,fileread(reference_file)),'ADRC:ReferenceChanged','原参考参数发生变化。');
metrics=table(case_names(:),single_max_errors(:,1),single_max_errors(:,2),single_max_errors(:,3), ...
    double_max_errors(:,1),double_max_errors(:,2),double_max_errors(:,3), ...
    'VariableNames',{'scenario','single_z1_max','single_z2_max','single_z3_max', ...
    'double_z1_max','double_z2_max','double_z3_max'});
writetable(comparison,fullfile(output_dir,'leso3_comparison.csv'));
writetable(metrics,fullfile(output_dir,'alignment_metrics.csv'));
result=struct('teaching',teaching,'p',p,'original',original,'compiler',compiler, ...
    'compiler_version',compiler_version,'host_test_output',host_test_output,'metrics',metrics, ...
    'comparison',comparison,'single_max_errors',single_max_errors,'double_max_errors',double_max_errors);
save(fullfile(output_dir,'alignment_result.mat'),'result');
fprintf('验证通过：两组各%d拍，更新前后状态与MATLAB single完全一致。\n',numel(time));
fprintf('相对同输入double参考，各状态最大差值：%s。\n',mat2str(max(double_max_errors,[],1),6));

%% 中文图：上排观察含噪估计，下排区分float舍入与double参考的差异
fig=figure('Name','第16讲：C++三阶LESO与MATLAB对齐','Color','w','Position',[50 35 1340 820]);
layout=tiledlayout(fig,2,3,'TileSpacing','compact');
truth=[true_y,true_velocity,true_f];
names=["角度 z1","角速度 z2","总扰动 z3"];
units=["rad","rad/s","rad/s²"];
for state=1:3
    ax=nexttile(layout,state);
    plot(ax,time,truth(:,state),'k:',time,cpp_before_all(:,state,2), ...
        time,double_before_all(:,state,2),'--','LineWidth',1.1);
    title(ax,"含噪声："+names(state)); ylabel(ax,units(state));
    legend(ax,'模型真值','C++ float','MATLAB double','Location','best','FontSize',8);
    grid(ax,'on'); xlabel(ax,'时间（秒）');
    ax=nexttile(layout,state+3);
    plot(ax,time,cpp_before_all(:,state,1)-double_before_all(:,state,1), ...
        time,cpp_before_all(:,state,2)-double_before_all(:,state,2));
    title(ax,"C++与double参考差值："+names(state)); ylabel(ax,units(state));
    legend(ax,'无噪声输入','含噪声输入','Location','best','FontSize',8);
    grid(ax,'on'); xlabel(ax,'时间（秒）');
end
title(layout,'第16讲｜三阶LESO的C++实现：同一份float输入，与MATLAB single逐拍对齐');
adrc_export_figure(fig,output_dir,"leso3_cpp_alignment");
end

function output=run_checked(command,description)
[status,output]=system(command);
assert(status==0,'ADRC:HostToolFailure','%s失败（退出码%d）：\n%s',description,status,output);
fprintf('%s：通过。\n',description);
end

function q=quote_path(path)
assert(~contains(path,'"') && ~contains(path,newline),'ADRC:InvalidPath','路径包含无效引号或换行。');
q=['"' char(path) '"'];
end

function write_fixture(file,time,input,measurement,dt)
fid=fopen(file,'w','n','UTF-8');
assert(fid>=0,'ADRC:WriteFailure','无法创建C++输入文件。');
file_cleanup=onCleanup(@() fclose(fid)); %#ok<NASGU>
fprintf(fid,'time,input,measurement,dt\n');
values=[time,double(input),double(measurement),repmat(double(dt),numel(time),1)];
fprintf(fid,'%.17g,%.9g,%.9g,%.9g\n',values.');
end
