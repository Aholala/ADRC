%% 第19讲：TD、二阶LADRC与实际输入反馈的完整C++示例
% 教学归属：工程接入；只组合既有三个核心，不新增控制器封装或硬件默认值。
% 点击“运行”（F5），需MATLAB、g++及本机ADRC1参考文件，不需要MEX。
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
addpath(fullfile(repo,'matlab','内部函数'));
lesson19=run_example(repo);

function result=run_example(repo)
output_dir=fullfile(repo,'build','lesson19');
if ~isfolder(output_dir), mkdir(output_dir); end
reference_root='D:/ACE/ADRC/ADRC1';
reference_dir=fullfile(reference_root,'LADRC实现参考','MATLABDemo-LADRC 实现(高志强老师)','LADRC Demo');
td_dir=fullfile(reference_root,'贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
tuning_file=fullfile(reference_dir,'tuning.m'); model_file=fullfile(reference_dir,'Motion_acc03.mdl');
td_file=fullfile(td_dir,'TD2_L.m'); td_model_file=fullfile(td_dir,'MODEL','TD2_L_MODEL.mdl');
assert(all(isfile(string({tuning_file,model_file,td_file,td_model_file}))), ...
    'ADRC:MissingReference','原控制模型、参数或线性TD文件不存在。');
tuning_snapshot=fileread(tuning_file); model_snapshot=fileread(model_file);
td_snapshot=fileread(td_file); td_model_snapshot=fileread(td_model_file);
original=struct;
for name={'h','b0','wo','wc'}
    token=regexp(tuning_snapshot,['(?m)^\s*' name{1} '\s*=\s*([^;]+);'],'tokens','once');
    assert(~isempty(token),'ADRC:MissingParameter','原控制参数未找到。');
    original.(name{1})=str2double(strtrim(token{1}));
end
mask=regexp(td_model_snapshot,'MaskValueString\s+"([^|"]+)\|([^"]+)"','tokens','once');
assert(~isempty(mask),'ADRC:MissingTDParameter','原TD参数未找到。');
original.td_r=str2double(mask{1}); original.td_h=str2double(mask{2});
p=struct('h',original.h,'b0',original.b0,'omega_o',original.wo,'omega_c',8,'td_r',original.td_r, ...
    'end_time',8,'target_time',0.5,'target_value',1,'reverse_time',3.5,'reverse_target',-0.5, ...
    'load_time',2.5,'load_value',-2,'input_limit',0.1,'steady_start',7);
b0_f=single(p.b0); wc_f=single(p.omega_c); wo_f=single(p.omega_o); r_f=single(p.td_r); dt_f=single(p.h);
b0=double(b0_f); wc=double(wc_f); wo=double(wo_f); hd=double(dt_f);
blocks=regexp(model_snapshot,'BlockType\s+StateSpace\s+Name\s+"LESO[^"]*"(.*?)\n\s*}','tokens');
assert(numel(blocks)==1,'ADRC:ReferenceChanged','原LESO结构变化。');
token_a=regexp(blocks{1}{1},'(?m)^\s*A\s+"([^"]+)"','tokens','once');
token_b=regexp(blocks{1}{1},'(?m)^\s*B\s+"([^"]+)"','tokens','once');
kp_token=regexp(tuning_snapshot,'(?m)(?:^|;)\s*kp\s*=\s*([^;]+);','tokens','once');
kd_token=regexp(tuning_snapshot,'(?m)(?:^|;)\s*kd\s*=\s*([^;]+);','tokens','once');
assert(~isempty(token_a) && ~isempty(token_b) && ~isempty(kp_token) && ~isempty(kd_token), ...
    'ADRC:MissingExpression','原矩阵或控制公式未找到。');
A=eval(token_a{1}); B=eval(token_b{1}); kp=eval(kp_token{1}); kd_signed=eval(kd_token{1});
assert(kp==wc^2 && kd_signed==-2*wc,'ADRC:ControlChanged','原增益表达式变化。');
teaching=struct('lesson','第19讲','algorithm','C++线性TD、二阶LADRC与三阶LESO', ...
    'topic','完整调用顺序、外部限幅、实际输入反馈和自主闭环核对', ...
    'reference_parameters',tuning_file,'reference_model',model_file,'reference_td',td_file, ...
    'document','docs/19_完整C++接入示例.md');
fprintf('\n【第19讲：完整C++接入示例——TD、二阶LADRC、限幅与实际输入反馈】\n');
fprintf('本拍读TD值计算请求，调用方处理实际输入后更新LESO；TD只推进一次供下拍使用。\n');
fprintf('实际周期%g秒，TD原模型周期%g秒；本讲统一按实际控制周期调用。\n',p.h,original.td_h);

%% 编译涉及的三个核心测试和完整主机例程，退出时恢复环境
compiler=getenv('ADRC_CXX');
if isempty(compiler)
    [status,found]=system('where g++');
    if status==0
        candidates=splitlines(strtrim(string(found))); compiler=char(candidates(1));
    elseif isfile('D:/msys64/ucrt64/bin/g++.exe')
        compiler='D:/msys64/ucrt64/bin/g++.exe';
    end
end
assert(isfile(compiler),'ADRC:MissingCompiler','未找到g++，请设置ADRC_CXX为完整路径。');
previous_path=getenv('PATH'); path_cleanup=onCleanup(@() setenv('PATH',previous_path)); %#ok<NASGU>
setenv('PATH',[fileparts(compiler) pathsep previous_path]);
compiler_version=run_checked([quote_path(compiler) ' --version'],'编译器检查');
prefix=[quote_path(compiler) ' -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -static -I ' ...
    quote_path(fullfile(repo,'main')) ' '];
tests={'ltd2_test','leso3_test','ladrc2_test'};
source_args=[quote_path(fullfile(repo,'main','ALG_ADRC.cpp')) ' '];
host_test_outputs=cell(numel(tests),1);
for index=1:numel(tests)
    executable=fullfile(output_dir,[tests{index} '.exe']);
    run_checked([prefix source_args quote_path(fullfile(repo,'tests',[tests{index} '.cpp'])) ...
        ' -o ' quote_path(executable)],['编译核心测试：' tests{index}]);
    host_test_outputs{index}=run_checked(quote_path(executable),['运行核心测试：' tests{index}]);
end
all_sources=source_args;
example_exe=fullfile(output_dir,'control_example.exe');
run_checked([prefix all_sources quote_path(fullfile(repo,'examples','19_complete_integration','control_example.cpp')) ...
    ' -o ' quote_path(example_exe)],'编译完整C++示例');
previous_matlab_path=path; reference_cleanup=onCleanup(@() path(previous_matlab_path)); %#ok<NASGU>
addpath(td_dir,'-begin');
assert(strcmpi(which('TD2_L'),td_file),'ADRC:WrongReference','TD2_L不是指定原文件。');

%% 唯一目标与负载序列；两组始终接入TD，只比较是否限幅
time=(0:round(p.end_time/p.h))'*p.h;
targets=single(p.target_value*double(time>=p.target_time));
targets(time>=p.reverse_time)=single(p.reverse_target);
forces=single(p.load_value*double(time>=p.load_time));
fixture=fullfile(output_dir,'control_input.csv'); write_fixture(fixture,time,targets,forces,dt_f);
case_names=["unlimited","limited"]; limits=single([0,p.input_limit]);
beta1=single(3)*wo_f; omega_squared=wo_f*wo_f; beta2=single(3)*omega_squared; beta3=omega_squared*wo_f;
open_transition=[[1 hd;0 1] zeros(2,3);hd*B(:,2)*[1 0] eye(3)+hd*A];
input_channel=[0.5*hd^2*b0;hd*b0;hd*B(:,1)]; disturbance_channel=[0.5*hd^2;hd;zeros(3,1)];
command_errors=zeros(2,1); state_errors=zeros(2,5); y_errors=zeros(2,1); input_errors=zeros(2,1);
peak_request=zeros(2,1); peak_applied=zeros(2,1); saturation_times=zeros(2,1);
steady_errors=zeros(2,1); steady_inputs=zeros(2,1);
cpp_runs=cell(2,1); reference_runs=cell(2,1); parts=cell(2,1);
for scenario=1:2
    output_file=fullfile(output_dir,char(case_names(scenario)+"_cpp.csv"));
    args=sprintf(' %.9g %.9g %.9g %.9g %.9g',b0_f,wc_f,wo_f,r_f,limits(scenario));
    run_checked([quote_path(example_exe) ' ' quote_path(fixture) ' ' quote_path(output_file) args], ...
        ['C++完整示例：' char(case_names(scenario))]);
    C=readtable(output_file); cpp_runs{scenario}=C;
    assert(height(C)==numel(time) && isequal(C.time,time) && isequal(single(C.raw_target),targets) ...
        && isequal(single(C.disturbance),forces) && all(single(C.dt)==dt_f) ...
        && isequal(single(C.measurement),single(C.true_y)), ...
        'ADRC:InputMismatch','原始目标、负载、周期或当前测量不一致。');
    before=single(C{:,8:12}); after=single(C{:,15:19});
    expected_before=zeros(numel(time),5,'single'); expected_after=expected_before;
    requests=zeros(numel(time),1,'single'); applied_values=requests;
    td_s=zeros(2,1,'single'); zs=zeros(3,1,'single'); xd=zeros(5,1); td_d=zeros(2,1);
    independent=zeros(numel(time),5);
    for k=1:numel(time)
        % 同输入回放：TD使用本拍旧状态，LESO使用实际限幅后输入。
        expected_before(k,:)=[td_s;zs].';
        request=adrc_control2(td_s(1),zs,b0_f,wc_f);
        if limits(scenario)>0, applied=min(max(request,-limits(scenario)),limits(scenario)); else, applied=request; end
        requests(k)=request; applied_values(k)=applied;
        zs=adrc_leso3_step(zs,applied,single(C.measurement(k)),b0_f,beta1,beta2,beta3,dt_f);
        td_s=ltd_step(td_s,targets(k),r_f,dt_f);
        expected_after(k,:)=[td_s;zs].';
        % 独立double闭环：原TD函数、原LESO矩阵与原控制增益表达式，自行推进对象。
        request_d=(kp*(td_d(1)-xd(3))+kd_signed*xd(4)-xd(5))/b0;
        if limits(scenario)>0
            applied_d=min(max(request_d,-double(limits(scenario))),double(limits(scenario)));
        else
            applied_d=request_d;
        end
        independent(k,:)=[xd(1),td_d(1),td_d(2),request_d,applied_d];
        xd=open_transition*xd+input_channel*applied_d+disturbance_channel*double(forces(k));
        td_d=TD2_L(time(k),td_d,double(targets(k)),2,double(r_f),hd); td_d=td_d(:);
    end
    command_errors(scenario)=max(abs(double(single(C.requested_input))-double(requests)));
    state_errors(scenario,:)=max([abs(double(before)-double(expected_before));abs(double(after)-double(expected_after))],[],1);
    assert(isequal(before,expected_before) && isequal(after,expected_after) ...
        && isequal(single(C.requested_input),requests) && isequal(single(C.applied_input),applied_values), ...
        'ADRC:FloatMismatch','组合顺序或同输入single状态与C++不一致。');
    assert(isequal(before(2:end,:),after(1:end-1,:)),'ADRC:TimeLabelMismatch','模块推进次数或时间标签不一致。');
    first=round(p.target_time/p.h)+1;
    assert(before(first,1)==0 && single(C.requested_input(first))==0, ...
        'ADRC:FutureStateUsed','原始目标刚跳变时不能偷用下一拍TD状态。');
    if limits(scenario)>0
        assert(all(abs(single(C.applied_input))<=limits(scenario)), ...
            'ADRC:InputLimit','实际输入超出调用方限幅。');
    end
    y_errors(scenario)=max(abs(C.true_y-independent(:,1)));
    input_errors(scenario)=max(abs(double(single(C.applied_input))-independent(:,5)));
    assert(y_errors(scenario)<1e-4 && input_errors(scenario)<5e-4, ...
        'ADRC:IndependentMismatch','与独立double完整闭环的差异超出本例范围。');
    peak_request(scenario)=max(abs(double(single(C.requested_input))));
    peak_applied(scenario)=max(abs(double(single(C.applied_input))));
    saturation_times(scenario)=sum(single(C.requested_input(1:end-1))~=single(C.applied_input(1:end-1)))*hd;
    steady=time>=p.steady_start;
    steady_errors(scenario)=mean(double(targets(steady))-C.true_y(steady));
    steady_inputs(scenario)=mean(double(single(C.applied_input(steady))));
    assert(abs(steady_errors(scenario))<5e-5 && abs(steady_inputs(scenario)+p.load_value/p.b0)<5e-5, ...
        'ADRC:SteadyMismatch','当前可达负载下的末段结果不符。');
    assert(all(isfinite(C{:,:}),'all'),'ADRC:NonfiniteResult','完整示例出现非有限数值。');
    reference_runs{scenario}=independent;
    parts{scenario}=table(repmat(case_names(scenario),numel(time),1),time,C.true_y,independent(:,1), ...
        C.td_x1_before,C.requested_input,C.applied_input,'VariableNames', ...
        {'scenario','time','cpp_y','independent_y','cpp_control_target','requested_input','applied_input'});
end
assert(saturation_times(1)==0 && saturation_times(2)>0,'ADRC:SaturationMissing','本讲需要实际触发外部限幅。');
assert(strcmp(tuning_snapshot,fileread(tuning_file)) && strcmp(model_snapshot,fileread(model_file)) ...
    && strcmp(td_snapshot,fileread(td_file)) && strcmp(td_model_snapshot,fileread(td_model_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');
comparison=vertcat(parts{:});
metrics=table(case_names(:),command_errors,state_errors(:,1),state_errors(:,2),state_errors(:,3),state_errors(:,4), ...
    state_errors(:,5),y_errors,input_errors,peak_request,peak_applied,saturation_times,steady_errors,steady_inputs, ...
    'VariableNames',{'scenario','single_request_max','single_td_x1_max','single_td_x2_max','single_z1_max', ...
    'single_z2_max','single_z3_max','independent_y_max','independent_applied_max','peak_request','peak_applied', ...
    'saturation_time_s','mean_final_raw_error','mean_final_input'});
writetable(comparison,fullfile(output_dir,'integration_comparison.csv'));
writetable(metrics,fullfile(output_dir,'integration_metrics.csv'));
result=struct('teaching',teaching,'p',p,'original',original,'compiler',compiler,'compiler_version',compiler_version, ...
    'host_test_outputs',{host_test_outputs},'metrics',metrics,'comparison',comparison);
save(fullfile(output_dir,'integration_result.mat'),'result');
fprintf('验证通过：两组各%d拍，五个状态及请求与同输入single参考完全一致。\n',numel(time));
fprintf('独立double完整闭环：位置最大差值%.3g rad，实际输入最大差值%.3g A。\n',max(y_errors),max(input_errors));
disp(table(case_names(:),peak_request,peak_applied,saturation_times,steady_errors,steady_inputs, ...
    'VariableNames',{'场景','请求绝对峰值_A','实际绝对峰值_A','累计限幅时间_s','末段原始目标误差_rad','末段电流_A'}));

%% 中文图：完整目标流程、限幅、扰动估计和独立参考
fig=figure('Name','第19讲：完整C++接入示例','Color','w','Position',[60 40 1280 820]);
layout=tiledlayout(fig,2,2,'TileSpacing','compact');
C=cpp_runs{2}; D=reference_runs{2};
ax=nexttile(layout);
plot(ax,time,double(targets),'k:',time,C.td_x1_before,'LineWidth',1.2); hold(ax,'on');
plot(ax,time,C.true_y,'LineWidth',1.3); plot(ax,time,D(:,1),'--','LineWidth',1.1);
title(ax,'限幅组：原始目标、TD给定与真实角度'); ylabel(ax,'角度（rad）');
legend(ax,'原始目标','TD的本拍v1','C++对象输出','独立double对象输出','Location','best','FontSize',8);
ax=nexttile(layout);
plot(ax,time,C.requested_input,time,C.applied_input,'--','LineWidth',1.2); hold(ax,'on');
yline(ax,p.input_limit,'k:','HandleVisibility','off'); yline(ax,-p.input_limit,'k:','HandleVisibility','off');
title(ax,'限幅组：请求与实际输入'); ylabel(ax,'电流（A）');
legend(ax,'控制请求','实际输入','Location','best','FontSize',8);
ax=nexttile(layout);
plot(ax,time,C.td_x2_before,time,C.z2_before,'LineWidth',1.2);
title(ax,'TD变化率与LESO角速度估计'); ylabel(ax,'角速度（rad/s）');
legend(ax,'TD的v2（仅记录）','LESO的z2','Location','best','FontSize',8);
ax=nexttile(layout);
plot(ax,time,double(forces),'k:',time,C.z3_before,'LineWidth',1.2);
title(ax,'限幅后实际输入反馈下的总扰动估计'); ylabel(ax,'总扰动（rad/s²）');
legend(ax,'真实总扰动','LESO的z3','Location','best','FontSize',8);
axes_list=findall(fig,'Type','axes');
for index=1:numel(axes_list), xlabel(axes_list(index),'时间（秒）'); grid(axes_list(index),'on'); end
title(layout,'第19讲｜完整C++示例：TD本拍给定 → LADRC请求 → 外部限幅 → 实际输入更新LESO');
adrc_export_figure(fig,output_dir,"complete_integration");
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

function write_fixture(file,time,target,force,dt)
fid=fopen(file,'w','n','UTF-8'); assert(fid>=0,'ADRC:WriteFailure','无法创建示例输入。');
file_cleanup=onCleanup(@() fclose(fid)); %#ok<NASGU>
fprintf(fid,'time,raw_target,disturbance,dt\n');
fprintf(fid,'%.17g,%.9g,%.9g,%.9g\n',[time,double(target),double(force),repmat(double(dt),numel(time),1)].');
end
