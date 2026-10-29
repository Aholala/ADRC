%% 第28讲：原NLSEF_2fal、速度误差与非线性闭环
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
result=run_example(repo);
function result=run_example(repo)
previous=path;cleanup=onCleanup(@()path(previous)); %#ok<NASGU>
addpath(fullfile(repo,'matlab','内部函数'));
ref=fullfile('D:/ACE/ADRC/ADRC1','贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');addpath(ref,'-begin');
files={fullfile(ref,'NLSEF_2fal.m'),fullfile(ref,'ESO2_NL.m'),fullfile(ref,'TD2_NL.m')};
snap=cellfun(@fileread,files,'UniformOutput',false);
assert(strcmpi(which('NLSEF_2fal'),files{1}));
p=adrc_final_parameters;t=(0:8000)'*p.h;N=numel(t);
target=double(t>=0.5)-1.5*double(t>=3.5);force=-2*double(t>=2.5);
rng_before=rng;stream=RandStream('mt19937ar','Seed',p.seed);noise=p.sigma*randn(stream,N,1);
runs=cell(2,2);reference_max=zeros(2,2);metrics=zeros(4,4);labels=strings(4,1);
for scenario=1:2
    for kind=1:2
        current=p;
        if kind==1,current.feedback=[p.b0,p.wc^2,1,2*p.wc,1,p.feedback_delta];end
        n=noise*double(scenario==2);
        T=adrc_final_closed(current,t,target,force,n,p.limit,'double',false);
        R=adrc_final_closed(current,t,target,force,n,p.limit,'double',true);
        reference_max(scenario,kind)=max(abs(T{:,:}-R{:,:}),[],'all');
        assert(reference_max(scenario,kind)<1e-8,'ADRC:Reference','完整闭环与原三个模块独立组合不一致。');
        assert(max(abs(T.applied))<=p.limit && any(T.requested~=T.applied));
        tail=t>=7;
        assert(abs(mean(T.true_y(tail)-target(tail)))<5e-4 && abs(mean(T.applied(tail))-0.05)<5e-4);
        row=(scenario-1)*2+kind;labels(row)=sprintf('noise%d_feedback%d',scenario-1,kind);
        metrics(row,:)=[max(abs(T.requested)),sum(T.requested(1:end-1)~=T.applied(1:end-1))*p.h, ...
            mean(T.true_y(tail)-target(tail)),std(T.requested(tail),1)];
        runs{scenario,kind}=T;
    end
end
assert(isequal(rng_before,rng));for i=1:3,assert(strcmp(snap{i},fileread(files{i})));end
metrics=table(labels,metrics(:,1),metrics(:,2),metrics(:,3),metrics(:,4), ...
    'VariableNames',{'scenario','peak_request','saturation_time_s','mean_tail_error','tail_request_std'});
out=fullfile(repo,'build','lesson28');if ~isfolder(out),mkdir(out);end
for s=1:2,for k=1:2,writetable(runs{s,k},fullfile(out,sprintf('closed_s%d_k%d.csv',s,k)));end,end
writetable(metrics,fullfile(out,'feedback_metrics.csv'));
result=struct('lesson',28,'teaching','第28讲：非线性反馈与完整MATLAB闭环，局部反馈增益匹配', ...
    'p',p,'runs',{runs},'metrics',metrics,'reference_max',reference_max);
save(fullfile(out,'feedback_result.mat'),'result');
fprintf('第28讲通过：32004次更新，原TD/ESO/NLSEF独立组合最大差值%.3g。\n',max(reference_max,[],'all'));disp(metrics);
fig=figure('Name','第28讲：非线性误差反馈','Color','w','Position',[50 40 1260 820]);L=tiledlayout(fig,2,2);
ax=nexttile(L);plot(ax,t,target,'k:',t,runs{1,1}.true_y,t,runs{1,2}.true_y);title(ax,'同一非线性TD和ESO：只比较反馈形式');ylabel(ax,'角度（rad）');legend(ax,'原始目标','线性反馈','非线性反馈');
ax=nexttile(L);plot(ax,t,runs{1,2}.requested,t,runs{1,2}.applied);title(ax,'非线性反馈请求与实际输入');ylabel(ax,'输入（A）');legend(ax,'请求','实际');
ax=nexttile(L);plot(ax,t,runs{1,2}.td2,t,runs{1,2}.z2);title(ax,'速度误差使用TD变化率减估计变化率');ylabel(ax,'变化率（rad/s）');legend(ax,'TD目标变化率','ESO估计变化率');
ax=nexttile(L);plot(ax,t,runs{2,1}.requested,t,runs{2,2}.requested);xlim(ax,[7 7.3]);title(ax,'带噪声闭环：没有预设非线性一定更好');ylabel(ax,'请求输入（A）');legend(ax,'线性反馈','非线性反馈');
for ax=findall(fig,'Type','axes')',grid(ax,'on');xlabel(ax,'时间（秒）');end
title(L,'第28讲｜原两项fal反馈 + 显式扰动补偿；限幅由调用方处理');adrc_export_figure(fig,out,"sef_closed");
end
