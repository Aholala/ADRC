function result=adrc_run_application(repo,id,build)
% 应用入口：直接观察完整方案。与按课次学习的教学入口分开。
previous=path;cleanup=onCleanup(@()path(previous)); %#ok<NASGU>
addpath(fullfile(repo,'matlab','内部函数'));
if nargin<3,build=adrc_build_final_host(repo);end
p=adrc_application_parameters(id);out=fullfile(repo,'build','applications',sprintf('case%02d',id));
if ~isfolder(out),mkdir(out);end
t=(0:round(p.duration/p.h))'*p.h;N=numel(t);goal=zeros(N,4);rate=goal;load=goal;
stream=RandStream('mt19937ar','Seed',p.seed+id);noise=0.0003*randn(stream,N,4);
switch id
    case 1
        goal(:,1)=5*double(t>=0.5)-8*double(t>=5);load(:,1)=-5*double(t>=3 & t<4.5);
    case 2
        continuous=179*pi/180+20*pi/180*sin(0.8*t);
        goal(:,1)=wrap_angle(continuous);rate(:,1)=16*pi/180*cos(0.8*t);load(:,1)=2*double(t>=4);
    case 3
        source_time=max(0,floor((t-p.vision_delay)*p.vision_hz)/p.vision_hz);
        active=source_time>=0.5;phase=max(0,source_time-0.5);
        goal(:,1)=0.35*sin(1.4*phase).*active;rate(:,1)=0.49*cos(1.4*phase).*active;
        load(:,1)=2*double(t>=4);continuous=0.35*sin(1.4*max(0,t-0.5)).*double(t>=0.5);
    case 4
        goal(:,:)=repmat(6*double(t>=0.5)-4*double(t>=5.5),1,4);
        goal(:,[2 4])=goal(:,[2 4])*0.8;
        load(:,1)=-2*double(t>=3);load(:,4)=-3*double(t>=4);noise=noise*10;
end
values=[t,goal,rate,noise,load,repmat(p.h,N,1)];
names={'time','target0','target1','target2','target3','rate0','rate1','rate2','rate3', ...
    'noise0','noise1','noise2','noise3','load0','load1','load2','load3','dt'};
F=array2table(values,'VariableNames',names);fixture=fullfile(out,'fixture.csv');write_fixture(fixture,values,names);
native=fullfile(out,'native.csv');command=sprintf('"%s" %d "%s" "%s"',build.application_runner,id,fixture,native);
[status,output]=system(command);assert(status==0,'ADRC:Application','C++应用运行失败：%s',output);
C=readtable(native);assert(height(C)==N*p.variants*p.wheels && all(isfinite(C{:,:}),'all'));
R=adrc_application_reference(p,F);
% 一层为独立闭环，另一层在相同输入与当前状态上逐次核对C++控制/ESO公式。
closed_max=max(abs(C{:,{'y','v','applied','next_y','next_v'}}-R{:,{'y','v','applied','next_y','next_v'}}),[],1);
assert(all(closed_max<[0.005 0.01 0.01 0.005 0.01]),'ADRC:ClosedMismatch','独立闭环差值超过本实验允许范围。');
replay_max=replay_check(p,C);
assert(max(abs(C.applied))<=double(single(p.limit))+1e-6);
if id==4,assert(max(C.power)<=p.power_limit+1e-4);end
if id==3,assert(max(abs(C.handover_error))<1e-6);end
metrics=zeros(p.variants,6);
for a=1:p.variants
    D=C(C.variant==a-1,:);active=D.time>=0.5;
    metrics(a,:)=[sqrt(mean((D.y(active)-D.target(active)).^2)),max(abs(D.request)), ...
        max(abs(D.applied)),sum(abs(D.request-D.applied)>1e-5)*p.h/p.wheels, ...
        sqrt(mean((D.y(D.time>=7)-D.target(D.time>=7)).^2)),max(abs(D.handover_error))];
end
M=array2table(metrics,'VariableNames',{'tracking_rms','peak_request_A','peak_applied_A','limited_time_s','tail_rms','handover_error_A'});
M.scheme=p.labels';M=movevars(M,'scheme','Before',1);writetable(M,fullfile(out,'metrics.csv'));
fig=figure('Name',sprintf('应用方案%02d：%s',id,p.name),'Position',[60 60 1250 850]);
L=tiledlayout(fig,2,2,'TileSpacing','compact','Padding','compact');scale=1;unit='rad/s';if id==2 || id==3,scale=180/pi;unit='度';end
ax=nexttile;hold(ax,'on');labels="目标";
if id==2,desired=continuous;elseif id==3,desired=continuous;else,desired=goal(:,1);end
plot(ax,t,scale*desired,'k--','LineWidth',1.1);
for a=1:p.variants,D=C(C.variant==a-1 & C.wheel==0,:);plot(ax,D.time,scale*D.y,'LineWidth',1);labels(end+1)=p.labels(a);end %#ok<AGROW>
legend(ax,labels,'Location','best');ylabel(ax,['被控量（' unit '）']);
if id==4,title(ax,'目标与实际响应（显示第1轮）');else,title(ax,'目标与实际响应');end
selected=1;if id==2 || id==3,selected=2;end
D=C(C.variant==selected & C.wheel==0,:);
ax=nexttile;plot(ax,D.time,D.request,D.time,D.applied,'LineWidth',1.1);legend(ax,'控制请求','限制后最终命令');ylabel(ax,'电流命令（A）');title(ax,p.labels(selected+1));
ax=nexttile;
true_f=p.b0*(D.next_actuator-D.applied)+D.load;
if id==1 || id==4,true_f=true_f-p.damping*D.y;else,true_f=true_f-p.damping*D.v;end
estimate=D.z3;if id==3,estimate(D.time<2)=nan;end
plot(ax,D.time,true_f,'k--',D.time,estimate,'LineWidth',1);legend(ax,'本例模型总扰动','ESO当前估计（启用后）');ylabel(ax,'总扰动（rad/s²）');title(ax,'包含电机响应差异的总扰动');
ax=nexttile;
if id==4
    hold(ax,'on');for a=1:p.variants,E=C(C.variant==a-1 & C.wheel==0,:);plot(ax,E.time,E.power,'LineWidth',1);end
    yline(ax,p.power_limit,'k--');legend(ax,[p.labels,"教学功率上限"]);ylabel(ax,'教学功率（W）');title(ax,'模型不用于替代实车功率算法');
elseif id==2
    plot(ax,t,goal(:,1)*180/pi,D.time,D.target*180/pi,'LineWidth',1.1);legend(ax,'跨±180度的原始目标','展开并经TD处理后的目标');ylabel(ax,'角度（度）');title(ax,'连续角度处理属于应用层');
elseif id==3
    window=D.time>=1.8 & D.time<=2.2;plot(ax,D.time(window),D.applied(window),'LineWidth',1.2);xline(ax,2,'k--');ylabel(ax,'最终命令（A）');title(ax,'2秒由示例PID接管到LADRC2');
else
    plot(ax,D.time,D.applied,D.time,D.next_actuator,'LineWidth',1.1);legend(ax,'最终命令','模拟实际电流');ylabel(ax,'电流（A）');title(ax,'两拍延迟与电流响应');
end
for ax=findall(fig,'Type','axes')',grid(ax,'on');xlabel(ax,'时间（s）');end
title(L,sprintf('应用方案%02d｜%s｜参数为仿真假设，尚未上板验证',id,p.name));
adrc_export_figure(fig,out,"application");
result=struct('purpose','应用方案','id',id,'parameters',p,'metrics',M,'closed_max',closed_max, ...
    'replay_max',replay_max,'native_updates',height(C),'native_output',output, ...
    'host_test_count',build.test_count,'passed',true,'hardware_tested',false);
save(fullfile(out,'result.mat'),'result','C','R','F');
fid=fopen(fullfile(out,'result.json'),'w','n','UTF-8');assert(fid>=0);fprintf(fid,'%s',jsonencode(result,'PrettyPrint',true));fclose(fid);
fprintf('应用方案%02d通过：%d次自主C++更新；独立闭环最大差值%s；逐次公式最大差值%s。\n',id,height(C),mat2str(closed_max,4),mat2str(replay_max,4));disp(M);
end
function maximum=replay_check(p,C)
maximum=zeros(1,4);b=single(p.b0);wc=single(p.wc);wo=single(p.wo);
variants=C.variant;times=C.time;r_all=single(C.target);rd_all=single(C.target_rate);
states=single(C{:,{'z1','z2','z3'}});u_all=single(C.applied);y_all=single(C.measurement);h_all=single(C.dt);
expected=C{:,{'request','next_z1','next_z2','next_z3'}};requests=single(C.request);
for k=1:height(C)
    variant=variants(k);if variant==0 || (p.id==3 && variant==2 && times(k)<2),continue;end
    r=r_all(k);rd=rd_all(k);z=states(k,:)';u=u_all(k);y=y_all(k);h=h_all(k);
    if p.id==1 && variant==2
        request=adrc_nlesf1(r,z(1),z(3),p.nlesf1);next=adrc_eso2_step(z([1 3]),u,y,p.eso2,h);next=[next(1);single(0);next(2)];
    elseif p.id==1 || p.id==4
        request=adrc_control(r,z([1 3]),b,wc);input=u;if p.id==4 && variant==2,input=requests(k);end
        next=adrc_eso_step(z([1 3]),input,y,b,single(2)*wo,wo*wo,h);next=[next(1);single(0);next(2)];
    elseif p.id==3 && variant==3
        request=adrc_sef(r,rd,z,p.nlesf2);next=adrc_eso3_step(z,u,y,p.eso3,h);
    else
        if variant==1,request=adrc_control2(r,z,b,wc);
        else,request=(wc*wc*(r-z(1))+single(2)*wc*(rd-z(2))-z(3))/b;end
        next=adrc_leso3_step(z,u,y,b,single(3)*wo,single(3)*wo*wo,wo*wo*wo,h);
    end
    maximum=max(maximum,abs([double(request),double(next')]-expected(k,:)));
end
peaks=max(abs(C{:,{'request','next_z1','next_z2','next_z3'}}),[],1);
assert(all(maximum<=max([2e-5 2e-5 2e-4 2e-3],2e-5*peaks)),'ADRC:ReplayMismatch','同状态同输入逐次核对失败。');
end
function write_fixture(file,values,names)
fid=fopen(file,'w');assert(fid>=0);cleanup=onCleanup(@()fclose(fid)); %#ok<NASGU>
fprintf(fid,'%s\n',strjoin(names,','));format=[repmat('%.17g,',1,size(values,2)-1),'%.17g\n'];fprintf(fid,format,values');
end
function x=wrap_angle(x)
x=x-2*pi*floor((x+pi)/(2*pi));
end
