%% 第18讲：线性TD的最小C++实现与原函数、MATLAB对齐
% 教学归属：工程实现；只实现独立TD，不在本讲修改LADRC或组合控制策略。
% 点击“运行”（F5）。需要MATLAB、g++及ADRC1的TD2_L参考文件，无需MEX。
repo=fileparts(fileparts(fileparts(mfilename('fullpath'))));
addpath(fullfile(repo,'matlab','内部函数'));
lesson18=run_alignment(repo);

function result=run_alignment(repo)
output_dir=fullfile(repo,'build','lesson18');
if ~isfolder(output_dir), mkdir(output_dir); end
reference_dir=fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file=fullfile(reference_dir,'TD2_L.m');
model_file=fullfile(reference_dir,'MODEL','TD2_L_MODEL.mdl');
assert(isfile(reference_file) && isfile(model_file),'ADRC:MissingReference','原线性TD文件或模型不存在。');
reference_snapshot=fileread(reference_file); model_snapshot=fileread(model_file);
mask=regexp(model_snapshot,'MaskValueString\s+"([^|"]+)\|([^"]+)"','tokens','once');
constant=regexp(model_snapshot,'Name\s+"Constant".*?Value\s+"([^"]+)"','tokens','once');
sine=regexp(model_snapshot,'Name\s+"Sine Wave".*?Frequency\s+"([^"]+)"','tokens','once');
stop=regexp(model_snapshot,'StopTime\s+"([^"]+)"','tokens','once');
assert(~isempty(mask) && ~isempty(constant) && ~isempty(sine) && ~isempty(stop), ...
    'ADRC:ReferenceChanged','原模型参数结构变化。');
p=struct('r',str2double(mask{1}),'h',str2double(mask{2}), ...
    'constant_amplitude',str2double(constant{1}),'sine_frequency',eval(sine{1}), ...
    'end_time',str2double(stop{1}));
assert(all(isfinite([p.r p.h p.constant_amplitude p.sine_frequency p.end_time])) ...
    && p.r>0 && p.h>0 && p.r*p.h<2, ...
    'ADRC:InvalidParameter','检查原TD参数与实际步长。');
teaching=struct('lesson','第18讲','algorithm','C++线性二阶TD', ...
    'topic','相同float输入下与MATLAB single对齐，并对照原TD2_L double更新', ...
    'reference_code',reference_file,'reference_model',model_file, ...
    'document','docs/18_线性TD的C++实现.md');
fprintf('\n【第18讲：线性TD的C++实现与逐拍核对】\n');
fprintf('原模型r=%g、h=%g秒，恒值输入=%g，正弦角频率=%g rad/s。\n', ...
    p.r,p.h,p.constant_amplitude,p.sine_frequency);

%% 编译独立C++核心、主机测试和逐拍驱动；函数退出时恢复环境
compiler=getenv('ADRC_CXX');
if isempty(compiler)
    [status,found]=system('where g++');
    if status==0
        candidates=splitlines(strtrim(string(found))); compiler=char(candidates(1));
    elseif isfile('D:/msys64/ucrt64/bin/g++.exe')
        compiler='D:/msys64/ucrt64/bin/g++.exe';
    end
end
assert(isfile(compiler),'ADRC:MissingCompiler','未找到g++，请设置ADRC_CXX为编译器完整路径。');
previous_path=getenv('PATH');
path_cleanup=onCleanup(@() setenv('PATH',previous_path)); %#ok<NASGU>
setenv('PATH',[fileparts(compiler) pathsep previous_path]);
compiler_version=run_checked([quote_path(compiler) ' --version'],'编译器检查');
base=[quote_path(compiler) ' -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -static -I ' ...
    quote_path(fullfile(repo,'include')) ' ' quote_path(fullfile(repo,'src','linear_td2.cpp')) ' '];
test_exe=fullfile(output_dir,'linear_td2_test.exe'); runner_exe=fullfile(output_dir,'td_runner.exe');
run_checked([base quote_path(fullfile(repo,'tests','linear_td2_test.cpp')) ' -o ' quote_path(test_exe)], ...
    'C++解析响应与接口测试编译');
host_test_output=run_checked(quote_path(test_exe),'C++主机测试');
run_checked([base quote_path(fullfile(repo,'examples','18_td_alignment','td_runner.cpp')) ...
    ' -o ' quote_path(runner_exe)],'C++逐拍驱动编译');
previous_matlab_path=path;
reference_cleanup=onCleanup(@() path(previous_matlab_path)); %#ok<NASGU>
addpath(reference_dir,'-begin');
assert(strcmpi(which('TD2_L'),reference_file),'ADRC:WrongReference','当前TD2_L不是指定原函数。');

%% 恒值与正弦：使用相同float输入、实际周期及零初态
time=(0:round(p.end_time/p.h))'*p.h;
signals=single([p.constant_amplitude*ones(numel(time),1),sin(p.sine_frequency*time)]);
r_f=single(p.r); dt_f=single(p.h);
case_names=["constant","sine"];
single_errors=zeros(2,2); original_errors=zeros(2,2);
cpp_states=zeros(numel(time),2,2); original_states=cpp_states;
comparison_parts=cell(2,1);
for scenario=1:2
    fixture=fullfile(output_dir,char(case_names(scenario)+"_input.csv"));
    cpp_file=fullfile(output_dir,char(case_names(scenario)+"_cpp.csv"));
    write_fixture(fixture,time,signals(:,scenario),dt_f);
    args=sprintf(' %.9g 0 0',r_f);
    run_checked([quote_path(runner_exe) ' ' quote_path(fixture) ' ' quote_path(cpp_file) args], ...
        ['C++执行：' char(case_names(scenario))]);
    C=readtable(cpp_file);
    assert(height(C)==numel(time) && isequal(C.time,time) ...
        && isequal(single(C.input),signals(:,scenario)) && all(single(C.dt)==dt_f), ...
        'ADRC:InputMismatch','C++输入或时间标签不一致。');
    before=single(C{:,4:5}); after=single(C{:,6:7});
    single_before=zeros(numel(time),2,'single'); single_after=single_before;
    original_before=zeros(numel(time),2); original_after=original_before;
    xs=zeros(2,1,'single'); xd=zeros(2,1);
    for k=1:numel(time)
        single_before(k,:)=xs.'; original_before(k,:)=xd.';
        xs=td_linear_step(xs,signals(k,scenario),r_f,dt_f);
        xd=TD2_L(time(k),xd,double(signals(k,scenario)),2,double(r_f),double(dt_f));
        xd=xd(:);
        single_after(k,:)=xs.'; original_after(k,:)=xd.';
    end
    single_errors(scenario,:)=max([abs(double(before)-double(single_before)); ...
        abs(double(after)-double(single_after))],[],1);
    original_errors(scenario,:)=max([abs(double(before)-original_before); ...
        abs(double(after)-original_after)],[],1);
    assert(isequal(before,single_before) && isequal(after,single_after), ...
        'ADRC:FloatMismatch','当前C++构建与MATLAB single不一致。');
    assert(isequal(before(2:end,:),after(1:end-1,:)),'ADRC:TimeLabelMismatch','状态推进或采样标签不一致。');
    assert(all(isfinite(C{:,:}),'all') && max(original_errors(scenario,:))<2e-4, ...
        'ADRC:OriginalMismatch','原double函数对照超出本实验范围或存在非有限结果。');
    cpp_states(:,:,scenario)=double(before); original_states(:,:,scenario)=original_before;
    comparison_parts{scenario}=table(repmat(case_names(scenario),numel(time),1),time, ...
        double(signals(:,scenario)),double(before(:,1)),double(before(:,2)), ...
        original_before(:,1),original_before(:,2),'VariableNames', ...
        {'scenario','time','input','cpp_x1','cpp_x2','original_double_x1','original_double_x2'});
end
assert(strcmp(reference_snapshot,fileread(reference_file)) && strcmp(model_snapshot,fileread(model_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');
comparison=vertcat(comparison_parts{:});
metrics=table(case_names(:),single_errors(:,1),single_errors(:,2),original_errors(:,1),original_errors(:,2), ...
    'VariableNames',{'scenario','single_x1_max','single_x2_max','original_double_x1_max','original_double_x2_max'});
writetable(comparison,fullfile(output_dir,'td_comparison.csv'));
writetable(metrics,fullfile(output_dir,'alignment_metrics.csv'));
result=struct('teaching',teaching,'p',p,'compiler',compiler,'compiler_version',compiler_version, ...
    'host_test_output',host_test_output,'comparison',comparison,'metrics',metrics);
save(fullfile(output_dir,'alignment_result.mat'),'result');
fprintf('验证通过：两组各%d拍，更新前后状态与MATLAB single完全一致。\n',numel(time));
fprintf('相同输入下与原TD2_L double更新的最大差值：%s。\n',mat2str(max(original_errors,[],1),6));

%% 中文图：展示真实C++输出与原函数对照，阶跃输入不伪造有限导数
fig=figure('Name','第18讲：C++线性TD与原函数对齐','Color','w','Position',[70 40 1230 800]);
layout=tiledlayout(fig,2,2,'TileSpacing','compact');
for scenario=1:2
    ax=nexttile(layout); hold(ax,'on');
    plot(ax,time,double(signals(:,scenario)),'k:','DisplayName','输入信号');
    plot(ax,time,cpp_states(:,1,scenario),'LineWidth',1.3,'DisplayName','C++ float平滑值');
    plot(ax,time,original_states(:,1,scenario),'--','LineWidth',1.2,'DisplayName','原函数double平滑值');
    if scenario==1, title(ax,'恒值输入：平滑输出x1'); else, title(ax,'正弦输入：平滑输出x1'); end
    ylabel(ax,'输入单位'); xlabel(ax,'时间（秒）'); grid(ax,'on'); legend(ax,'show','Location','best','FontSize',8);
    ax=nexttile(layout); hold(ax,'on');
    if scenario==2
        plot(ax,time,p.sine_frequency*cos(p.sine_frequency*time),'k:','DisplayName','原正弦输入解析导数');
    end
    plot(ax,time,cpp_states(:,2,scenario),'LineWidth',1.3,'DisplayName','C++ float变化率');
    plot(ax,time,original_states(:,2,scenario),'--','LineWidth',1.2,'DisplayName','原函数double变化率');
    if scenario==1, title(ax,'恒值输入：变化率状态x2'); else, title(ax,'正弦输入：变化率状态x2'); end
    ylabel(ax,'输入单位/秒'); xlabel(ax,'时间（秒）'); grid(ax,'on'); legend(ax,'show','Location','best','FontSize',8);
end
title(layout,'第18讲｜C++线性二阶TD：相同float输入，与MATLAB single及原TD2_L核对');
adrc_export_figure(fig,output_dir,"linear_td_cpp_alignment");
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

function write_fixture(file,time,input,dt)
fid=fopen(file,'w','n','UTF-8');
assert(fid>=0,'ADRC:WriteFailure','无法创建输入文件。');
file_cleanup=onCleanup(@() fclose(fid)); %#ok<NASGU>
fprintf(fid,'time,input,dt\n');
values=[time,double(input),repmat(double(dt),numel(time),1)];
fprintf(fid,'%.17g,%.9g,%.9g\n',values.');
end
