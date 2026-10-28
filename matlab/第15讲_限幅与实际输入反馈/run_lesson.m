%% 第15讲：二阶LADRC限幅与实际输入反馈
% 教学归属：LADRC工程边界；对象始终接收限幅后的输入，只比较LESO输入。
% 直接点击“运行”（F5），运行整个文件。对应docs/15_限幅与实际输入反馈.md。
% 本讲用直接阶跃触发饱和；无TD、无噪声，理想执行器精确跟随限幅后指令。
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1','LADRC实现参考', ...
    'MATLABDemo-LADRC 实现(高志强老师)','LADRC Demo');
model_file = fullfile(reference_dir,'Motion_acc03.mdl');
tuning_file = fullfile(reference_dir,'tuning.m');
assert(isfile(model_file) && isfile(tuning_file),'ADRC:MissingReference', ...
    '请检查ADRC1原模型和tuning.m的位置。');
model_snapshot = fileread(model_file); tuning_snapshot = fileread(tuning_file);
output_dir = fullfile(repo,'build','lesson15');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 沿用原LESO结构及第13讲教学控制带宽；限幅属于调用方的实验执行器
original = struct;
for name = {'h','b0','wo','wc'}
    token = regexp(tuning_snapshot,['(?m)^\s*' name{1} '\s*=\s*([^;]+);'],'tokens','once');
    assert(~isempty(token),'ADRC:MissingParameter','原参数未找到。');
    original.(name{1}) = str2double(strtrim(token{1}));
end
h = original.h; b0 = original.b0; wo = original.wo;
p = struct('h',h,'b0',b0,'omega_o',wo,'omega_c',8,'end_time',6, ...
    'target_time',0.5,'target_value',1,'load_time',2.5,'load_value',-2, ...
    'input_min',-0.2,'input_max',0.2,'steady_start',5);
wc = p.omega_c;
assert(all(isfinite([h b0 wo wc p.input_min p.input_max])) && h>0 && b0~=0 ...
    && wo>0 && wc>0 && wo*h<2 && p.input_min<p.input_max, ...
    'ADRC:InvalidParameter','检查步长、增益和教学执行器限幅。');
assert(-p.load_value/b0>p.input_min && -p.load_value/b0<p.input_max, ...
    'ADRC:InfeasibleHold','本讲负载需要的维持输入已超出能力，不能作为可达目标对比。');
blocks = regexp(model_snapshot, ...
    'BlockType\s+StateSpace\s+Name\s+"LESO[^"]*"(.*?)\n\s*}', 'tokens');
assert(numel(blocks)==1,'ADRC:ReferenceChanged','原LESO块结构变化。');
token_a = regexp(blocks{1}{1},'(?m)^\s*A\s+"([^"]+)"','tokens','once');
token_b = regexp(blocks{1}{1},'(?m)^\s*B\s+"([^"]+)"','tokens','once');
kp_token = regexp(tuning_snapshot,'(?m)(?:^|;)\s*kp\s*=\s*([^;]+);','tokens','once');
kd_token = regexp(tuning_snapshot,'(?m)(?:^|;)\s*kd\s*=\s*([^;]+);','tokens','once');
assert(~isempty(token_a) && ~isempty(token_b) && ~isempty(kp_token) && ~isempty(kd_token), ...
    'ADRC:MissingExpression','原矩阵或控制增益未找到。');
A_reference = eval(token_a{1}); B_reference = eval(token_b{1});
kp = eval(kp_token{1}); kd_signed = eval(kd_token{1});
assert(kp==wc^2 && kd_signed==-2*wc,'ADRC:ControlChanged','原控制增益表达式变化。');
teaching = struct('lesson','第15讲','algorithm','二阶LADRC、三阶LESO与输出限幅', ...
    'topic','请求值、实际输入和观测器输入的区别', ...
    'reference_model',model_file,'reference_parameters',tuning_file, ...
    'document','docs/15_限幅与实际输入反馈.md', ...
    'changes','限幅为新增调用方教学设置；wc=8；理想限幅后输入等于实际作用量');
fprintf('\n【第15讲：二阶LADRC限幅与实际输入反馈】\n');
fprintf('请求值u_req由控制器计算；实际输入u_act限制在[%g,%g] A。\n',p.input_min,p.input_max);
fprintf('对象始终使用u_act；两组LESO分别使用u_act（正确）和u_req（反例）。\n');

%% 参考离散更新分别设置对象输入通道和观测器输入通道，保留限幅非线性
plant_transition = [1 h;0 1];
open_transition = [plant_transition zeros(2,3); ...
    h*B_reference(:,2)*[1 0] eye(3)+h*A_reference];
plant_input = [0.5*h^2*b0;h*b0;zeros(3,1)];
observer_input_channel = [zeros(2,1);h*B_reference(:,1)];
disturbance_channel = [0.5*h^2;h;zeros(3,1)];
feedback_row = [0 0 -kp/b0 kd_signed/b0 -1/b0];
scenario_names = ["applied_input","requested_input"];
labels = ["LESO使用实际输入","LESO误用请求值（反例）"];
time = (0:round(p.end_time/h))'*h;
records = zeros(numel(time)*2,16);
reference_errors = zeros(2,1);
row = 0;
for scenario = 1:2
    y = 0; velocity = 0; z = zeros(3,1); reference_state = zeros(5,1);
    for k = 1:numel(time)
        target = p.target_value*double(k-1>=round(p.target_time/h));
        true_f = p.load_value*double(k-1>=round(p.load_time/h));
        requested_input = adrc_control2(target,z,b0,wc);
        applied_input = min(max(requested_input,p.input_min),p.input_max);
        if scenario==1, observer_input=applied_input; else, observer_input=requested_input; end
        saturated = abs(requested_input-applied_input)>1e-12;
        % 相对于观测器收到的输入重写对象方程；这是该输入约定下的有效残差。
        input_relative_disturbance = true_f+b0*(applied_input-observer_input);
        row = row+1;
        records(row,:) = [scenario,time(k),target,y,velocity,true_f,z.', ...
            requested_input,applied_input,observer_input,double(saturated), ...
            input_relative_disturbance,target-y,z(3)-true_f];
        z_next = adrc_leso3_step(z,observer_input,y,b0,3*wo,3*wo^2,wo^3,h);
        acceleration = b0*applied_input+true_f;
        y_next = y+h*velocity+0.5*h^2*acceleration;
        velocity_next = velocity+h*acceleration;

        reference_request = feedback_row*reference_state+(kp/b0)*target;
        reference_applied = min(max(reference_request,p.input_min),p.input_max);
        if scenario==1, reference_observer=reference_applied; else, reference_observer=reference_request; end
        reference_next = open_transition*reference_state+plant_input*reference_applied ...
            + observer_input_channel*reference_observer+disturbance_channel*true_f;
        reference_errors(scenario) = max(reference_errors(scenario), ...
            max(abs([y_next;velocity_next;z_next]-reference_next)));
        if k<numel(time)
            y=y_next; velocity=velocity_next; z=z_next; reference_state=reference_next;
        end
    end
end
T = array2table(records(:,2:end),'VariableNames',{'time','target','true_y','true_velocity', ...
    'true_f','z1','z2','z3','requested_input','applied_input','observer_input','saturated', ...
    'input_relative_disturbance','tracking_error','physical_disturbance_error'});
scenario_column = scenario_names(records(:,1));
T = addvars(T,scenario_column(:),'Before',1,'NewVariableNames','scenario');

%% 核对实际限幅、输入通道、首拍手算、可达负载的正确组末态
peak_requests=zeros(2,1); peak_applied=zeros(2,1); saturation_times=zeros(2,1);
preload_disturbance_errors=zeros(2,1); steady_errors=zeros(2,1); steady_inputs=zeros(2,1);
for scenario=1:2
    rows=T(T.scenario==scenario_names(scenario),:);
    peak_requests(scenario)=max(abs(rows.requested_input));
    peak_applied(scenario)=max(abs(rows.applied_input));
    saturation_times(scenario)=sum(rows.saturated(1:end-1))*h; % 最后一行不是额外时间区间。
    preload=rows.time>=p.target_time & rows.time<p.load_time;
    preload_disturbance_errors(scenario)=max(abs(rows.physical_disturbance_error(preload)));
    steady=rows.time>=p.steady_start;
    steady_errors(scenario)=mean(rows.tracking_error(steady));
    steady_inputs(scenario)=mean(rows.applied_input(steady));
    assert(all(rows.applied_input>=p.input_min & rows.applied_input<=p.input_max), ...
        'ADRC:InputLimit','对象输入超出调用方设置的限幅。');
    if scenario==1
        assert(isequal(rows.observer_input,rows.applied_input),'ADRC:InputRoute','正确组输入通道不一致。');
    else
        assert(isequal(rows.observer_input,rows.requested_input),'ADRC:InputRoute','反例组输入通道不一致。');
    end
    first=round(p.target_time/h)+1;
    assert(abs(rows.requested_input(first)-1.6)<1e-12 && abs(rows.applied_input(first)-0.2)<1e-12, ...
        'ADRC:FirstStep','首拍教学手算条件发生变化。');
    if scenario==2
        assert(abs(rows.input_relative_disturbance(first)+56)<1e-10, ...
            'ADRC:ResidualMeaning','首拍限幅差额折算结果应为-56 rad/s²。');
    end
end
assert(abs(steady_errors(1))<1e-7 && abs(steady_inputs(1)+p.load_value/b0)<1e-7, ...
    'ADRC:SteadyMismatch','当前可达负载下正确输入组末态异常。');
assert(max(reference_errors)<1e-7,'ADRC:ReferenceMismatch','含限幅的完整更新与参考矩阵不一致。');
assert(all(isfinite(records),'all'),'ADRC:NonfiniteResult','出现非有限结果。');
assert(strcmp(model_snapshot,fileread(model_file)) && strcmp(tuning_snapshot,fileread(tuning_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');
metrics=table(scenario_names(:),peak_requests,peak_applied,saturation_times, ...
    preload_disturbance_errors,steady_errors,steady_inputs,'VariableNames', ...
    {'scenario','peak_request','peak_applied','saturation_time_s','preload_peak_z3_error', ...
    'mean_final_tracking_error','mean_final_applied_input'});
metrics_cn=table(labels(:),peak_requests,peak_applied,saturation_times,preload_disturbance_errors, ...
    'VariableNames',{'观测器输入选择','请求绝对峰值_A','实际绝对峰值_A','饱和累计时间_s','负载前扰动估计最大误差_rad_s2'});
disp(metrics_cn);
fprintf('验证通过：限幅、输入通道、首拍残差和正确组末态；参考更新最大差值%.3g。\n', ...
    max(reference_errors));
writetable(T,fullfile(output_dir,'saturation_sim.csv'));
writetable(metrics,fullfile(output_dir,'saturation_metrics.csv'));
save(fullfile(output_dir,'saturation_result.mat'),'T','metrics','p','original','teaching','reference_errors');

%% 中文图：请求、实际作用量、运动和扰动估计分别展示
fig=figure('Name','第15讲：限幅与实际输入反馈','Color','w','Position',[70 40 1270 820]);
layout=tiledlayout(fig,2,2,'TileSpacing','compact');
ax1=nexttile(layout); hold(ax1,'on'); ax2=nexttile(layout); hold(ax2,'on');
ax3=nexttile(layout); hold(ax3,'on'); ax4=nexttile(layout); hold(ax4,'on');
base=T(T.scenario==scenario_names(1),:);
plot(ax1,time,base.target,'k--','DisplayName','角度目标');
colors=[0.22 0.43 0.70;0.79 0.38 0.18];
for scenario=1:2
    rows=T(T.scenario==scenario_names(scenario),:);
    plot(ax1,time,rows.true_y,'Color',colors(scenario,:),'LineWidth',1.3,'DisplayName',labels(scenario));
    plot(ax2,time,rows.applied_input,'Color',colors(scenario,:),'LineWidth',1.2,'DisplayName',labels(scenario));
    plot(ax3,time,rows.requested_input,'Color',colors(scenario,:),'LineWidth',1.2,'DisplayName',labels(scenario));
    plot(ax4,time,rows.physical_disturbance_error,'Color',colors(scenario,:),'LineWidth',1.2,'DisplayName',labels(scenario));
end
for ax=[ax2 ax3]
    yline(ax,p.input_max,'k:','HandleVisibility','off'); yline(ax,p.input_min,'k:','HandleVisibility','off');
end
title(ax1,'同一目标下的真实角度'); ylabel(ax1,'角度（rad）');
title(ax2,'对象实际输入始终受限'); ylabel(ax2,'限幅后电流（A）'); ylim(ax2,[-0.25 0.25]);
title(ax3,'控制请求可以超过执行器能力'); ylabel(ax3,'请求电流（A）');
title(ax4,'相对物理总扰动的估计误差'); ylabel(ax4,'z3－真实f（rad/s²）');
for ax=[ax1 ax2 ax3 ax4]
    xlabel(ax,'时间（秒）'); grid(ax,'on'); legend(ax,'show','Location','best','FontSize',8);
end
title(layout,'第15讲｜二阶LADRC限幅：对象都用实际输入，比较LESO使用哪个输入');
adrc_export_figure(fig,output_dir,"saturation_response");
