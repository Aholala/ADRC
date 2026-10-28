%% 第27讲：三阶非线性ESO的C++实现与分层核对
repo=fileparts(fileparts(fileparts(mfilename('fullpath'))));
result=run_example(repo);
function result=run_example(repo)
previous=path;cleanup=onCleanup(@()path(previous)); %#ok<NASGU>
addpath(fullfile(repo,'matlab','内部函数'));
ref=fullfile('D:/ACE/ADRC/ADRC1','贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');addpath(ref,'-begin');
file=fullfile(ref,'ESO2_NL.m');snapshot=fileread(file);assert(strcmpi(which('ESO2_NL'),file));
build=adrc_build_final_host(repo);p=adrc_final_parameters;
h=single(p.h);c=single(p.eso);N=6001;t=(0:N-1)'*double(h);idx=(0:N-1)';
u=single(0.075)*single(idx>=500);f=-2*double(idx>=1500);
y=0.5*double(c(1))*double(single(0.075))*max(t-t(501),0).^2-max(t-t(1501),0).^2;
rng_before=rng;stream=RandStream('mt19937ar','Seed',p.seed);noise=p.sigma*randn(stream,N,1);
out=fullfile(repo,'build','lesson27');if ~isfolder(out),mkdir(out);end
config=fullfile(out,'observer_config.txt');write_matrix(config,[double(c),double(single([0.1 0 0]))],'%.9g ');
single_error=zeros(2,3);source_error=zeros(2,3);runs=cell(2,1);
for scenario=1:2
    measurement=single(y+noise*double(scenario==2));
    input=fullfile(out,sprintf('input%d.csv',scenario));output=fullfile(out,sprintf('cpp%d.csv',scenario));
    fid=fopen(input,'w');assert(fid>=0);fprintf(fid,'time,input,measurement,dt\n');
    fprintf(fid,'%.17g,%.9g,%.9g,%.9g\n',[t,double(u),double(measurement),repmat(double(h),N,1)]');fclose(fid);
    [status,log]=system(sprintf('"%s" "%s" "%s" "%s"',build.eso_runner,config,input,output));assert(status==0,'ADRC:Runner','%s',log);
    C=readtable(output);assert(height(C)==N && isequal(C.time,t));
    z=single([0.1;0;0]);zd=double(z);before=zeros(N,3,'single');after=before;db=zeros(N,3);da=db;
    for k=1:N
        before(k,:)=z';db(k,:)=zd';
        z=adrc_nleso3_step(z,u(k),measurement(k),c,h);
        dz=ESO2_NL(t(k),zd,[double(measurement(k));double(u(k))],1,double(c(5)),double(c(6)),double(c(7)),double(c(2:4)),double(c(1)));
        zd=zd+double(h)*dz(:);after(k,:)=z';da(k,:)=zd';
    end
    cb=single(C{:,5:7});ca=single(C{:,8:10});
    single_error(scenario,:)=max([abs(double(cb)-double(before));abs(double(ca)-double(after))],[],1);
    source_error(scenario,:)=max([abs(double(cb)-db);abs(double(ca)-da)],[],1);
    assert(all(single_error(scenario,:)<[2e-6 2e-4 2e-3]),'ADRC:Float','single误差超出本实验范围。');
    assert(all(source_error(scenario,:)<[1e-4 2e-3 2e-2]),'ADRC:Source','原函数double误差超出范围。');
    assert(isequal(ca(1:end-1,:),cb(2:end,:)) && all(isfinite(C{:,:}),'all'));
    runs{scenario}=C;
end
assert(strcmp(snapshot,fileread(file)) && isequal(rng_before,rng));
metrics=table(["clean";"noisy"],single_error(:,1),single_error(:,2),single_error(:,3),source_error(:,1),source_error(:,2),source_error(:,3), ...
    'VariableNames',{'scenario','single_z1_max','single_z2_max','single_z3_max','source_double_z1_max','source_double_z2_max','source_double_z3_max'});
writetable(metrics,fullfile(out,'alignment_metrics.csv'));
result=struct('lesson',27,'teaching','第27讲：C++三阶非线性ESO，single回放和原函数自主double对照', ...
    'p',p,'actual_config',c,'actual_dt',h,'metrics',metrics,'runs',{runs},'build',build);
save(fullfile(out,'alignment_result.mat'),'result');
fprintf('第27讲通过：12002次更新，single最大差值%s。\n',mat2str(max(single_error,[],1),6));disp(metrics);
fig=figure('Name','第27讲：C++非线性ESO','Color','w','Position',[50 40 1200 760]);L=tiledlayout(fig,2,2);
ax=nexttile(L);plot(ax,t,y,'k:',t,runs{1}.z1);title(ax,'干净组：真实输出与C++估计');legend(ax,'真实输出','C++ z1');ylabel(ax,'输出（rad）');xlabel(ax,'时间（秒）');
ax=nexttile(L);plot(ax,t,f,'k:',t,runs{1}.z3);xlim(ax,[1.4 2.2]);title(ax,'原连续导数的欧拉移植');legend(ax,'真实扰动','C++ z3');ylabel(ax,'扰动（rad/s²）');xlabel(ax,'时间（秒）');
ax=nexttile(L);plot(ax,t,runs{2}.z3);xlim(ax,[4 4.4]);title(ax,'带噪组C++扰动估计');ylabel(ax,'扰动估计（rad/s²）');xlabel(ax,'时间（秒）');
ax=nexttile(L);bar(ax,[single_error;source_error]');title(ax,'各状态的数值差值：不是总估计误差');ylabel(ax,'最大数值差');legend(ax,'single干净','single带噪','原double干净','原double带噪');xticks(ax,1:3);xticklabels(ax,{'z1','z2','z3'});
for ax=findall(fig,'Type','axes')',grid(ax,'on');end
title(L,'第27讲｜独立NLESO3：参数、状态、时序与原函数核对');adrc_export_figure(fig,out,"cpp_nleso_alignment");
end
function write_matrix(file,values,format)
fid=fopen(file,'w');assert(fid>=0);fprintf(fid,format,values);fclose(fid);
end
