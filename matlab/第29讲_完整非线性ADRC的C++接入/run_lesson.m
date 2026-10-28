%% 第29讲：非线性TD、NLESO、NLSEF完整C++闭环
repo=fileparts(fileparts(fileparts(mfilename('fullpath'))));
result=run_example(repo);
function result=run_example(repo)
previous=path;cleanup=onCleanup(@()path(previous)); %#ok<NASGU>
addpath(fullfile(repo,'matlab','内部函数'));
ref=fullfile('D:/ACE/ADRC/ADRC1','贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');addpath(ref,'-begin');
files={fullfile(ref,'ESO2_NL.m'),fullfile(ref,'TD2_NL.m'),fullfile(ref,'NLSEF_2fal.m')};
snap=cellfun(@fileread,files,'UniformOutput',false);
assert(strcmpi(which('ESO2_NL'),files{1}) && strcmpi(which('TD2_NL'),files{2}) && strcmpi(which('NLSEF_2fal'),files{3}));
build=adrc_build_final_host(repo);p=adrc_final_parameters;
% 独立double闭环使用相同float表示的配置，但独立维护自己的对象、测量和状态。
p.eso=double(single(p.eso));p.feedback=double(single(p.feedback));p.h=double(single(p.h));
p.td_r=double(single(p.td_r));p.td_h0=double(single(p.td_h0));p.limit=double(single(p.limit));
N=8001;t=(0:N-1)'*p.h;idx=(0:N-1)';target=single(double(idx>=500)-1.5*double(idx>=3500));force=single(-2*double(idx>=2500));
rng_before=rng;stream=RandStream('mt19937ar','Seed',p.seed);noise=p.sigma*randn(stream,N,1);
out=fullfile(repo,'build','lesson29');if ~isfolder(out),mkdir(out);end
names=["clean_unlimited","clean_limited","noisy_limited"];runs=cell(3,1);references=cell(3,1);
errors=zeros(3,5);independent=zeros(3,3);performance=zeros(3,5);
fields={'td1','td2','z1','z2','z3','requested','applied','next_td1','next_td2','next_z1','next_z2','next_z3'};
for scenario=1:3
    limit=0;if scenario>1,limit=p.limit;end
    current_noise=noise*double(scenario==3);
    config=fullfile(out,char(names(scenario)+"_config.txt"));
    fid=fopen(config,'w');assert(fid>=0);fprintf(fid,'%.9g ',[p.eso,p.feedback(2:end),p.td_r,p.td_h0,limit]);fclose(fid);
    input=fullfile(out,char(names(scenario)+"_input.csv"));output=fullfile(out,char(names(scenario)+"_cpp.csv"));
    fid=fopen(input,'w');assert(fid>=0);fprintf(fid,'time,target,force,noise,dt\n');
    fprintf(fid,'%.17g,%.9g,%.9g,%.17g,%.9g\n',[t,double(target),double(force),current_noise,repmat(p.h,N,1)]');fclose(fid);
    [status,log]=system(sprintf('"%s" "%s" "%s" "%s"',build.control_runner,config,input,output));assert(status==0,'ADRC:Runner','%s',log);
    C=readtable(output);assert(height(C)==N && isequal(C.time,t));
    S=adrc_final_closed(p,t,target,force,current_noise,limit,'single',false,C.measurement);
    D=adrc_final_closed(p,t,target,force,current_noise,limit,'double',true);
    diff=abs(C{:,fields}-S{:,fields});
    errors(scenario,:)=[max(diff(:,[1 3 8 10]),[],'all'),max(diff(:,[2 4 9 11]),[],'all'), ...
        max(diff(:,[5 12]),[],'all'),max(diff(:,6)),max(diff(:,7))];
    assert(all(errors(scenario,:)<[2e-5 5e-4 5e-3 2e-4 2e-4]),'ADRC:Float','single算法回放差值超出本实验范围。');
    independent(scenario,:)=[max(abs(C.true_y-D.true_y)),max(abs(C.applied-D.applied)),max(abs(C.z3-D.z3))];
    assert(independent(scenario,1)<2e-3 && independent(scenario,2)<2e-3,'ADRC:Independent','原模块独立double闭环差值超出范围。');
    assert(isequal(single(C{1:end-1,{'next_td1','next_td2','next_z1','next_z2','next_z3'}}), ...
        single(C{2:end,{'td1','td2','z1','z2','z3'}})) && isequal(C.next_y(1:end-1),C.true_y(2:end)) ...
        && all(isfinite(C{:,:}),'all'));
    if limit>0,assert(max(abs(C.applied))<=limit && any(C.requested~=C.applied));end
    tail=idx>=7000;
    assert(abs(mean(C.true_y(tail)-double(target(tail))))<5e-4 && abs(mean(C.applied(tail))-0.05)<5e-4);
    performance(scenario,:)=[max(abs(C.requested)),max(abs(C.applied)), ...
        sum(C.requested(1:end-1)~=C.applied(1:end-1))*p.h,mean(C.true_y(tail)-double(target(tail))),std(C.requested(tail),1)];
    runs{scenario}=C;references{scenario}=D;
end
assert(isequal(rng_before,rng));for i=1:3,assert(strcmp(snap{i},fileread(files{i})));end
metrics=table(names',errors(:,1),errors(:,2),errors(:,3),errors(:,4),errors(:,5),independent(:,1),independent(:,2),independent(:,3), ...
    performance(:,1),performance(:,2),performance(:,3),performance(:,4),performance(:,5), ...
    'VariableNames',{'scenario','single_position_max','single_rate_max','single_disturbance_max','single_request_max','single_applied_max', ...
    'independent_y_max','independent_applied_max','independent_disturbance_max','peak_request','peak_applied','saturation_time_s','mean_tail_error','tail_request_std'});
writetable(metrics,fullfile(out,'integration_metrics.csv'));
result=struct('lesson',29,'teaching','第29讲：完整C++非线性ADRC，两层核对与原模块独立闭环', ...
    'p',p,'runs',{runs},'references',{references},'metrics',metrics,'build',build);
save(fullfile(out,'integration_result.mat'),'result');
fprintf('第29讲通过：24003次自主C++更新，single最大差值%s。\n',mat2str(max(errors,[],1),6));disp(metrics);
fig=figure('Name','第29讲：完整C++非线性ADRC','Color','w','Position',[50 40 1260 820]);L=tiledlayout(fig,2,2);
ax=nexttile(L);plot(ax,t,double(target),'k:',t,runs{2}.true_y,t,references{2}.true_y,'--');title(ax,'有限幅：C++自主对象与原模块独立double');ylabel(ax,'角度（rad）');legend(ax,'原始目标','C++','原模块double');
ax=nexttile(L);plot(ax,t,runs{2}.requested,t,runs{2}.applied);title(ax,'反馈给ESO的是最终实际输入');ylabel(ax,'输入（A）');legend(ax,'请求','实际');
ax=nexttile(L);plot(ax,t,double(force),'k:',t,runs{2}.z3);title(ax,'负载只作用于对象，不直接传给算法');ylabel(ax,'扰动（rad/s²）');legend(ax,'真实f','ESO z3');
ax=nexttile(L);plot(ax,t,runs{3}.requested,t,references{3}.requested,'--');xlim(ax,[7 7.3]);title(ax,'带噪末段控制请求与独立参考');ylabel(ax,'请求输入（A）');legend(ax,'C++','原模块double');
for ax=findall(fig,'Type','axes')',grid(ax,'on');xlabel(ax,'时间（秒）');end
title(L,'第29讲｜当前TD/ESO状态算控制 → 最终限幅 → 实际输入推进ESO → TD推进');adrc_export_figure(fig,out,"full_nladrc");
end
