%% 第26讲：二阶对象的三阶非线性ESO——匹配局部增益的开环比较
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
result=run_example(repo);
function result=run_example(repo)
previous=path;cleanup=onCleanup(@()path(previous)); %#ok<NASGU>
addpath(fullfile(repo,'matlab','内部函数'));
ref=fullfile('D:/ACE/ADRC/ADRC1','贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');addpath(ref,'-begin');
files={fullfile(ref,'ESO2_NL.m'),fullfile(ref,'ESO_2L.m')};snap=cellfun(@fileread,files,'UniformOutput',false);
assert(strcmpi(which('ESO2_NL'),files{1}) && strcmpi(which('ESO_2L'),files{2}),'ADRC:Reference','参考函数路径不正确。');
p=adrc_final_parameters; p.end_time=6; p.initial_estimate=[0.1;0;0];
t=(0:round(p.end_time/p.h))'*p.h;N=numel(t);
u=0.075*double(t>=0.5);f=-2*double(t>=1.5);
y=0.5*p.b0*0.075*max(t-0.5,0).^2-max(t-1.5,0).^2;
v=p.b0*0.075*max(t-0.5,0)-2*max(t-1.5,0);
rng_before=rng;stream=RandStream('mt19937ar','Seed',p.seed);noise=p.sigma*randn(stream,N,1);
g=p.eso(7).^(p.eso(5:6)-1);
A=[-p.eso(2),1,0;-p.eso(3)*g(1),0,1;-p.eso(4)*g(2),0,0];
radius=max(abs(eig(eye(3)+p.h*A)));
assert(radius<1 && max(abs([p.eso(2),p.eso(3)*g(1),p.eso(4)*g(2)]-p.leso_beta))<1e-9, ...
    'ADRC:LocalGains','局部增益或离散极点检查失败。');
states=zeros(N,3,2,2);records=cell(2,2);reference_max=zeros(2,2);outer_count=zeros(2,1);
for scenario=1:2
    measurement=y+noise*double(scenario==2);
    for kind=1:2
        z=p.initial_estimate; zr=z;rows=zeros(N,11);
        for k=1:N
            if kind==1
                zn=adrc_leso3_step(z,u(k),measurement(k),p.b0,p.leso_beta(1),p.leso_beta(2),p.leso_beta(3),p.h);
                dz=ESO_2L(t(k),zr,[u(k);measurement(k)],1,p.leso_beta,p.b0);
            else
                zn=adrc_eso3_step(z,u(k),measurement(k),p.eso,p.h);
                dz=ESO2_NL(t(k),zr,[measurement(k);u(k)],1,p.alpha1,p.alpha2,p.delta,p.eso(2:4),p.b0);
                outer_count(scenario)=outer_count(scenario)+double(abs(z(1)-measurement(k))>=p.delta);
            end
            zrn=zr+p.h*dz(:);reference_max(scenario,kind)=max(reference_max(scenario,kind),max(abs(zn-zrn)));
            states(k,:,kind,scenario)=z';rows(k,:)=[t(k),u(k),measurement(k),y(k),f(k),z',zn'];
            z=zn;zr=zrn;
        end
        records{scenario,kind}=array2table(rows,'VariableNames', ...
            {'time','input','measurement','true_y','true_f','z1','z2','z3','next_z1','next_z2','next_z3'});
    end
end
assert(max(reference_max,[],'all')<1e-8 && all(outer_count>0),'ADRC:Reference','原函数轨迹对照或外侧分支覆盖失败。');
tail=t>=4;win=find(t>=1.5 & t<=4);settling=zeros(2,1);noise_rms=zeros(2,1);
for kind=1:2
    e=abs(states(win,3,kind,1)-f(win));last=find(e>0.04,1,'last');
    assert(~isempty(last) && last<numel(win),'ADRC:NotSettled','观测器在规定窗口未收敛。');
    settling(kind)=t(win(last+1))-1.5;
    delta=states(:,3,kind,2)-states(:,3,kind,1);noise_rms(kind)=sqrt(mean(delta(tail).^2));
end
assert(isequal(rng_before,rng));
for i=1:2,assert(strcmp(snap{i},fileread(files{i})));end
metrics=table(["LESO";"ESO"],settling,noise_rms,'VariableNames',{'observer','settling_2pct_s','disturbance_noise_increment_rms'});
out=fullfile(repo,'build','lesson26');if ~isfolder(out),mkdir(out);end
for s=1:2,for k=1:2,writetable(records{s,k},fullfile(out,sprintf('observer_s%d_k%d.csv',s,k)));end,end
writetable(metrics,fullfile(out,'observer_metrics.csv'));
result=struct('lesson',26,'teaching','第26讲：完整三阶非线性ESO，与LESO匹配局部增益', ...
    'p',p,'time',t,'input',u,'true_y',y,'true_rate',v,'true_f',f,'noise',noise, ...
    'states',states,'records',{records},'metrics',metrics,'reference_max',reference_max,'local_radius',radius,'outer_count',outer_count);
save(fullfile(out,'observer_result.mat'),'result');
fprintf('第26讲通过：24004次更新，原函数最大差值%.3g，局部离散谱半径%.6f。\n',max(reference_max,[],'all'),radius);disp(metrics);
fig=figure('Name','第26讲：完整非线性ESO','Color','w','Position',[50 40 1260 820]);L=tiledlayout(fig,2,2);
ax=nexttile(L);plot(ax,t,y,'k:',t,states(:,1,1,1),t,states(:,1,2,1));xlim(ax,[0 0.7]);title(ax,'初始输出估计偏差0.1：大误差暂态');ylabel(ax,'输出（rad）');legend(ax,'真值','LESO','ESO');
ax=nexttile(L);plot(ax,t,f,'k:',t,states(:,3,1,1),t,states(:,3,2,1));xlim(ax,[1.4 2.2]);title(ax,'相同实际输入：干净组扰动阶跃');ylabel(ax,'扰动（rad/s²）');legend(ax,'真实f','LESO','ESO');
ax=nexttile(L);plot(ax,t,states(:,3,1,2)-states(:,3,1,1),t,states(:,3,2,2)-states(:,3,2,1));xlim(ax,[4 4.4]);title(ax,'同轨迹同噪声：相对干净组的扰动估计增量');ylabel(ax,'估计增量（rad/s²）');legend(ax,'LESO','ESO');
ax=nexttile(L);plot(ax,t,v,'k:',t,states(:,2,1,1),t,states(:,2,2,1));title(ax,'变化率估计：状态含义不随fal改变');ylabel(ax,'变化率（rad/s）');legend(ax,'真实变化率','LESO','ESO');
for ax=findall(fig,'Type','axes')',grid(ax,'on');xlabel(ax,'时间（秒）');end
title(L,'第26讲｜原连续导数明确采用前向欧拉；局部增益匹配不等于全局动态相同');
adrc_export_figure(fig,out,"eso_vs_leso");
end
