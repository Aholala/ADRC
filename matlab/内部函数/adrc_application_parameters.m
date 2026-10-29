function p=adrc_application_parameters(id)
% 应用仿真假设，与examples/applications/application_runner.cpp中的参数对应。
% 电流A、角度rad、速度rad/s；时间s。不是实车辨识或裁判系统功率模型。
names={'单电机转速','舵向角度','视觉云台跟随','带功率限制的四轮控制'};
p=struct('id',id,'name',names{id},'h',0.001,'duration',8,'seed',20261029, ...
    'vision_hz',30,'vision_delay',0.04,'power_limit',12,'current_delay_samples',2);
switch id
    case 1
        p.b0=12;p.wc=12;p.wo=45;p.limit=2;p.damping=3;p.tau=0.01;p.initial=0;p.wheels=1;
        p.labels=["示例PI","LADRC1","ESO2与NLESF1"];
    case 2
        p.b0=30;p.wc=8;p.wo=30;p.limit=1.5;p.damping=0.8;p.tau=0.008;p.initial=179*pi/180;p.wheels=1;
        p.labels=["示例PID","LADRC2（目标速度为零）","LADRC2（接入TD速度）"];
    case 3
        p.b0=30;p.wc=8;p.wo=30;p.limit=1.5;p.damping=0.8;p.tau=0.008;p.initial=0;p.wheels=1;
        p.labels=["示例PID","LADRC2（仅目标位置）","LADRC2（含目标速度，2秒接管）","ESO3与NLESF2"];
    case 4
        p.b0=10;p.wc=12;p.wo=40;p.limit=3;p.damping=1.6;p.tau=0.01;p.initial=0;p.wheels=4;
        p.labels=["示例PI","LADRC1（反馈最终输入）","LADRC1（反馈请求，错误对照）"];
end
% 舵向比较两种速度基准；云台另包含手动/自动接管的独立过程。
p.variants=numel(p.labels);
b=single(p.b0);w=single(p.wo);delta=single(0.02);wc=single(p.wc);
p.eso2=[b,single(2)*w,w*w*delta^single(0.5),single(0.5),delta];
p.nlesf1=[b,wc*single(0.1)^single(0.25),single(0.75),single(0.1)];
p.eso3=[b,single(3)*w,single(3)*w*w*delta^single(0.5),w*w*w*delta^single(0.75),single(0.5),single(0.25),delta];
p.nlesf2=[b,wc*wc*single(0.1)^single(0.25),single(0.75),single(2)*wc*single(0.1)^single(-0.25),single(1.25),single(0.1)];
p.firmware_linked=false;p.hardware_tested=false;
end
