%% 第17讲：二阶LADRC的C++组合与完整闭环核对
% 教学归属：工程实现；C++自主运行对象和控制器，不由MATLAB代算控制输入。
% 点击“运行”（F5）。需要MATLAB、g++和ADRC1参考文件，不需要MEX。
repo=fileparts(fileparts(fileparts(mfilename('fullpath'))));
addpath(fullfile(repo,'matlab','内部函数'));
lesson17=run_alignment(repo);

function result=run_alignment(repo)
output_dir=fullfile(repo,'build','lesson17');
if ~isfolder(output_dir), mkdir(output_dir); end
reference_dir=fullfile('D:/ACE/ADRC/ADRC1','LADRC实现参考', ...
    'MATLABDemo-LADRC 实现(高志强老师)','LADRC Demo');
tuning_file=fullfile(reference_dir,'tuning.m'); model_file=fullfile(reference_dir,'Motion_acc03.mdl');
assert(isfile(tuning_file) && isfile(model_file),'ADRC:MissingReference','原参数或模型不存在。');
tuning_snapshot=fileread(tuning_file); model_snapshot=fileread(model_file);
original=struct;
for name={'h','b0','wo','wc'}
    token=regexp(tuning_snapshot,['(?m)^\s*' name{1} '\s*=\s*([^;]+);'],'tokens','once');
    assert(~isempty(token),'ADRC:MissingParameter','原参数未找到。');
    original.(name{1})=str2double(strtrim(token{1}));
end
p=struct('h',original.h,'b0',original.b0,'omega_o',original.wo,'omega_c',8, ...
    'end_time',6,'target_time',0.5,'target_value',1,'load_time',2.5,'load_value',-2,'steady_start',5);
b0_f=single(p.b0); wc_f=single(p.omega_c); wo_f=single(p.omega_o); dt_f=single(p.h);
b0=double(b0_f); wc=double(wc_f); wo=double(wo_f); hd=double(dt_f);
blocks=regexp(model_snapshot,'BlockType\s+StateSpace\s+Name\s+"LESO[^"]*"(.*?)\n\s*}','tokens');
assert(numel(blocks)==1,'ADRC:ReferenceChanged','原LESO块结构变化。');
token_a=regexp(blocks{1}{1},'(?m)^\s*A\s+"([^"]+)"','tokens','once');
token_b=regexp(blocks{1}{1},'(?m)^\s*B\s+"([^"]+)"','tokens','once');
kp_token=regexp(tuning_snapshot,'(?m)(?:^|;)\s*kp\s*=\s*([^;]+);','tokens','once');
kd_token=regexp(tuning_snapshot,'(?m)(?:^|;)\s*kd\s*=\s*([^;]+);','tokens','once');
assert(~isempty(token_a) && ~isempty(token_b) && ~isempty(kp_token) && ~isempty(kd_token), ...
    'ADRC:MissingExpression','原公式未找到。');
A=eval(token_a{1}); B=eval(token_b{1}); kp=eval(kp_token{1}); kd_signed=eval(kd_token{1});
assert(kp==wc^2 && kd_signed==-2*wc,'ADRC:ControlChanged','原控制增益表达式变化。');
teaching=struct('lesson','第17讲','algorithm','C++二阶LADRC与三阶LESO', ...
    'topic','分开Calc和UpdateObserver，验证无限幅与外部限幅的自主闭环', ...
    'reference_parameters',tuning_file,'reference_model',model_file, ...
    'document','docs/17_二阶LADRC的C++组合.md');
fprintf('\n【第17讲：二阶LADRC的C++组合与完整闭环核对】\n');

%% 编译真实C++模块、主机测试和自主闭环驱动
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
    quote_path(fullfile(repo,'include')) ' ' quote_path(fullfile(repo,'src','leso3.cpp')) ' ' ...
    quote_path(fullfile(repo,'src','ladrc2.cpp')) ' '];
test_exe=fullfile(output_dir,'ladrc2_test.exe'); runner_exe=fullfile(output_dir,'ladrc2_runner.exe');
run_checked([base quote_path(fullfile(repo,'tests','ladrc2_test.cpp')) ' -o ' quote_path(test_exe)], ...
    'C++接口与解析闭环测试编译');
host_test_output=run_checked(quote_path(test_exe),'C++主机测试');
run_checked([base quote_path(fullfile(repo,'examples','17_ladrc2_alignment','ladrc2_runner.cpp')) ...
    ' -o ' quote_path(runner_exe)],'C++自主闭环程序编译');

%% 两个场景：控制器不含限幅，限制只放在驱动程序的调用方
time=(0:round(p.end_time/p.h))'*p.h;
targets=single(p.target_value*double(time>=p.target_time));
forces=single(p.load_value*double(time>=p.load_time));
fixture=fullfile(output_dir,'closed_loop_input.csv');
write_fixture(fixture,time,targets,forces,dt_f);
case_names=["unlimited","limited"]; limits=single([0,0.2]); % 仅主机驱动中0表示不启用限幅。
omega_squared=wo_f*wo_f;
beta1_f=single(3)*wo_f; beta2_f=single(3)*omega_squared; beta3_f=omega_squared*wo_f;
open_transition=[[1 hd;0 1] zeros(2,3);hd*B(:,2)*[1 0] eye(3)+hd*A];
input_channel=[0.5*hd^2*b0;hd*b0;hd*B(:,1)];
disturbance_channel=[0.5*hd^2;hd;zeros(3,1)];
command_errors=zeros(2,1); state_errors=zeros(2,3);
independent_y_errors=zeros(2,1); independent_input_errors=zeros(2,1);
steady_errors=zeros(2,1); steady_inputs=zeros(2,1); peak_applied=zeros(2,1);
cpp_runs=cell(2,1); reference_runs=cell(2,1); comparison_parts=cell(2,1);
for scenario=1:2
    output_file=fullfile(output_dir,char(case_names(scenario)+"_cpp.csv"));
    args=sprintf(' %.9g %.9g %.9g %.9g',b0_f,wc_f,wo_f,limits(scenario));
    run_checked([quote_path(runner_exe) ' ' quote_path(fixture) ' ' quote_path(output_file) args], ...
        ['C++自主闭环：' char(case_names(scenario))]);
    C=readtable(output_file); cpp_runs{scenario}=C;
    assert(height(C)==numel(time) && isequal(C.time,time) && isequal(single(C.target),targets) ...
        && isequal(single(C.disturbance),forces) && all(single(C.dt)==dt_f), ...
        'ADRC:InputMismatch','C++目标、负载、周期或采样标签不一致。');
    assert(isequal(single(C.measurement),single(C.true_y)), ...
        'ADRC:MeasurementMismatch','C++观测器测量未对应本拍对象输出。');
    cpp_before=single(C{:,8:10}); cpp_after=single(C{:,13:15});
    zs=zeros(3,1,'single'); single_before=zeros(numel(time),3,'single'); single_after=single_before;
    single_commands=zeros(numel(time),1,'single'); single_applied=single_commands;
    xd=zeros(5,1); independent=zeros(numel(time),4);
    for k=1:numel(time)
        % 对齐层：回放C++真实测量，严格比较同输入的Calc和UpdateObserver。
        single_before(k,:)=zs.';
        request=adrc_control2(targets(k),zs,b0_f,wc_f);
        if limits(scenario)>0
            applied=min(max(request,-limits(scenario)),limits(scenario));
        else
            applied=request;
        end
        single_commands(k)=request; single_applied(k)=applied;
        zs=adrc_leso3_step(zs,applied,single(C.measurement(k)),b0_f,beta1_f,beta2_f,beta3_f,dt_f);
        single_after(k,:)=zs.';
        % 独立层：原增益和原LESO矩阵自行闭环，使用自身状态与输出，不回放C++测量。
        request_d=(kp*(double(targets(k))-xd(3))+kd_signed*xd(4)-xd(5))/b0;
        if limits(scenario)>0
            applied_d=min(max(request_d,-double(limits(scenario))),double(limits(scenario)));
        else
            applied_d=request_d;
        end
        independent(k,:)=[xd(1),xd(2),request_d,applied_d];
        xd=open_transition*xd+input_channel*applied_d+disturbance_channel*double(forces(k));
    end
    command_errors(scenario)=max(abs(double(single(C.requested_input))-double(single_commands)));
    state_errors(scenario,:)=max([abs(double(cpp_before)-double(single_before)); ...
        abs(double(cpp_after)-double(single_after))],[],1);
    assert(isequal(single(C.requested_input),single_commands) && isequal(single(C.applied_input),single_applied) ...
        && isequal(cpp_before,single_before) && isequal(cpp_after,single_after), ...
        'ADRC:FloatMismatch','C++计算与相同输入的MATLAB single不一致。');
    assert(isequal(cpp_before(2:end,:),cpp_after(1:end-1,:)), ...
        'ADRC:TimeLabelMismatch','观测器推进次数或时间标签不一致。');
    if limits(scenario)>0
        assert(all(abs(single(C.applied_input))<=limits(scenario)), ...
            'ADRC:InputLimit','实际输入超出调用方限幅。');
    end
    independent_y_errors(scenario)=max(abs(C.true_y-independent(:,1)));
    independent_input_errors(scenario)=max(abs(double(single(C.applied_input))-independent(:,4)));
    assert(independent_y_errors(scenario)<1e-4 && independent_input_errors(scenario)<5e-4, ...
        'ADRC:ClosedLoopMismatch','自主C++闭环与独立double模型差异超出本实验范围。');
    steady=time>=p.steady_start;
    steady_errors(scenario)=mean(double(targets(steady))-C.true_y(steady));
    steady_inputs(scenario)=mean(double(single(C.applied_input(steady))));
    peak_applied(scenario)=max(abs(double(single(C.applied_input))));
    assert(abs(steady_errors(scenario))<3e-5 && abs(steady_inputs(scenario)+p.load_value/p.b0)<3e-5, ...
        'ADRC:SteadyMismatch','当前可达负载下的末态异常。');
    assert(all(isfinite(C{:,:}),'all'),'ADRC:NonfiniteResult','C++结果存在非有限值。');
    reference_runs{scenario}=independent;
    comparison_parts{scenario}=table(repmat(case_names(scenario),numel(time),1),time, ...
        C.true_y,independent(:,1),double(single(C.applied_input)),independent(:,4), ...
        'VariableNames',{'scenario','time','cpp_y','double_y','cpp_applied','double_applied'});
end
assert(strcmp(tuning_snapshot,fileread(tuning_file)) && strcmp(model_snapshot,fileread(model_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');
comparison=vertcat(comparison_parts{:});
metrics=table(case_names(:),command_errors,state_errors(:,1),state_errors(:,2),state_errors(:,3), ...
    independent_y_errors,independent_input_errors,peak_applied,steady_errors,steady_inputs, ...
    'VariableNames',{'scenario','single_command_max','single_z1_max','single_z2_max','single_z3_max', ...
    'independent_y_max','independent_applied_max','peak_applied','mean_final_tracking_error','mean_final_input'});
writetable(comparison,fullfile(output_dir,'closed_loop_comparison.csv'));
writetable(metrics,fullfile(output_dir,'alignment_metrics.csv'));
result=struct('teaching',teaching,'p',p,'original',original,'compiler',compiler, ...
    'compiler_version',compiler_version,'host_test_output',host_test_output,'metrics',metrics,'comparison',comparison);
save(fullfile(output_dir,'alignment_result.mat'),'result');
fprintf('验证通过：两组各%d拍，控制请求和更新前后状态与同输入single参考完全一致。\n',numel(time));
fprintf('独立double闭环对照：位置最大差值%.3g rad，实际输入最大差值%.3g A。\n', ...
    max(independent_y_errors),max(independent_input_errors));
disp(table(case_names(:),peak_applied,steady_errors,steady_inputs, ...
    'VariableNames',{'场景','实际电流绝对峰值_A','末段平均跟踪误差_rad','末段平均电流_A'}));

%% 中文图：C++确实自主闭环，再与独立double轨迹比较
fig=figure('Name','第17讲：C++二阶LADRC闭环对齐','Color','w','Position',[60 40 1260 820]);
layout=tiledlayout(fig,2,2,'TileSpacing','compact');
for scenario=1:2
    C=cpp_runs{scenario}; D=reference_runs{scenario};
    ax=nexttile(layout,scenario);
    plot(ax,time,double(targets),'k:',time,C.true_y,time,D(:,1),'--','LineWidth',1.2);
    if scenario==1, title(ax,'无限幅：C++自主闭环'); else, title(ax,'外部限幅±0.2 A：C++自主闭环'); end
    ylabel(ax,'角度（rad）'); xlabel(ax,'时间（秒）'); grid(ax,'on');
    legend(ax,'目标','C++对象输出','独立MATLAB double','Location','best','FontSize',8);
    ax=nexttile(layout,scenario+2);
    plot(ax,time,C.requested_input,time,C.applied_input,'--','LineWidth',1.2);
    if scenario==1, title(ax,'无限幅：请求与实际输入'); else, title(ax,'外部限幅：请求与实际输入'); end
    ylabel(ax,'电流（A）'); xlabel(ax,'时间（秒）'); grid(ax,'on');
    legend(ax,'C++控制请求','调用方实际输入','Location','best','FontSize',8);
end
title(layout,'第17讲｜C++二阶LADRC组合：Calc请求 → 调用方处理 → UpdateObserver实际输入');
adrc_export_figure(fig,output_dir,"ladrc2_cpp_alignment");
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

function write_fixture(file,time,target,disturbance,dt)
fid=fopen(file,'w','n','UTF-8');
assert(fid>=0,'ADRC:WriteFailure','无法创建闭环输入文件。');
file_cleanup=onCleanup(@() fclose(fid)); %#ok<NASGU>
fprintf(fid,'time,target,disturbance,dt\n');
values=[time,double(target),double(disturbance),repmat(double(dt),numel(time),1)];
fprintf(fid,'%.17g,%.9g,%.9g,%.9g\n',values.');
end
