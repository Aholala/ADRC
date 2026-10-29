function result = adrc_basic_lesson(repo,lesson)
% 第1~6讲的简明教学实验。仅解释本讲概念，参数均为教学假设。
h=0.001;t=(0:5000)'*h;N=numel(t);b0=10;u=0.5*ones(N,1);f=-3*double(t>=1.5);
titles={'一阶对象与总扰动','二阶LESO的两个估计量','LESO纠偏快慢与测量抖动', ...
    '把连续变化换成逐次更新','一阶LADRC闭环','输出限制与ESO输入'};
out=fullfile(repo,'build',sprintf('lesson%02d',lesson));if ~isfolder(out),mkdir(out);end
fig=figure('Name',sprintf('第%d讲：%s',lesson,titles{lesson}),'Position',[80 80 1000 650]);
layout=tiledlayout(fig,2,1,'TileSpacing','compact');
switch lesson
    case 1
        fprintf('第1讲：对象是被控制的电机；模型是描述输入怎样改变输出的数学关系。\n');
        fprintf('本例u为电流，y为转速，b0是电流对加速度作用的估计系数，f合并其他作用。\n');
        f(:)=-3;y=(b0*u(1)+f(1))*t;
        assert(abs(y(2)-0.002)<1e-12);
        nexttile;plot(t,u,t,f,'LineWidth',1.2);legend('电流u（A）','总扰动f（rad/s²）');ylabel('不同单位，按图例读取');
        nexttile;plot(t,y,'LineWidth',1.2);ylabel('转速y（rad/s）');
        data=table(t,u,f,y);
    case {2,3,4}
        fprintf('观测器是利用已知输入和测量值，持续估计内部量的计算方法。\n');
        fprintf('本例LESO2保存两个状态：z1估计转速，z2估计总扰动；电机模型仍为一阶。\n');
        y=b0*u(1)*t-3*max(t-1.5,0);
        bands=20;if lesson==3,bands=[5 15 40];end
        measurement=y;
        if lesson==3,measurement=y+0.01*sin(2*pi*80*t);end
        states=zeros(N,2,numel(bands));
        for c=1:numel(bands)
            z=[0;0];w=bands(c);
            for k=1:N
                states(k,:,c)=z';
                z=adrc_eso_step(z,u(k),measurement(k),b0,2*w,w*w,h);
            end
        end
        if lesson==4
            fprintf('逐次更新：每隔h秒计算一次下一时刻估计；两行计算均使用更新前的状态。\n');
            one=adrc_eso_step([0;0],0,1,b0,20,100,h);
            assert(max(abs(one-[0.02;0.1]))<1e-12);
            fprintf('手算：旧z=[0,0]、测量1、输入0，下一次z=[0.02,0.1]。\n');
        end
        labels=compose('纠偏参数wo=%g',bands);
        nexttile;plot(t,y,'k--',t,squeeze(states(:,1,:)),'LineWidth',1.1);legend(["真实转速",labels]);ylabel('转速（rad/s）');
        nexttile;plot(t,f,'k--',t,squeeze(states(:,2,:)),'LineWidth',1.1);legend(["真实总扰动",labels]);ylabel('总扰动（rad/s²）');
        data=struct('time',t,'input',u,'measurement',measurement,'truth_y',y,'truth_f',f,'states',states,'wo',bands);
    case {5,6}
        fprintf('反馈控制是根据目标与当前值的差来计算作用量；LADRC再用ESO估计的扰动进行补偿。\n');
        fprintf('本例目标为转速1 rad/s，负载在1.5秒变化；不表示真实电机参数。\n');
        cases=1;if lesson==6,cases=2;fprintf('限幅是把命令约束在允许范围；ESO应使用最终处理后的模型输入。\n');end
        target=double(t>=0.5);records=zeros(N,5,cases);
        for c=1:cases
            y=0;z=[0;0];
            for k=1:N
                request=adrc_control(target(k),z,b0,8);
                applied=request;if lesson==6,applied=max(-0.35,min(0.35,request));end
                records(k,:,c)=[y,z(2),request,applied,target(k)];
                observer_input=applied;if c==2,observer_input=request;end
                z=adrc_eso_step(z,observer_input,y,b0,60,900,h);
                y=y+h*(b0*applied+f(k));
            end
        end
        assert(all(isfinite(records),'all'));
        nexttile;plot(t,target,'k--',t,squeeze(records(:,1,:)),'LineWidth',1.1);ylabel('转速（rad/s）');
        if cases==2,legend('目标','反馈最终输入','反馈限幅前请求（错误示范）');else,legend('目标','实际转速');end
        nexttile;plot(t,records(:,3,1),t,records(:,4,1),'LineWidth',1.1);legend('请求电流','最终电流');ylabel('电流（A）');
        data=struct('time',t,'records',records,'disturbance',f);
end
axes_list=findall(fig,'Type','axes');for ax=axes_list',xlabel(ax,'时间（s）');grid(ax,'on');end
title(layout,sprintf('教学｜第%02d讲：%s｜参数为教学假设',lesson,titles{lesson}));
adrc_export_figure(fig,out,'teaching');
result=struct('lesson',lesson,'purpose','教学','hardware_tested',false,'data',data);
save(fullfile(out,'teaching_result.mat'),'result');
fprintf('第%d讲教学实验完成。材料完成与个人学习进度分别记录。\n',lesson);
end
