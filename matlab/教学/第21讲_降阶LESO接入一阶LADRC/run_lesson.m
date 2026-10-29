%% 第21讲：降阶LESO接入一阶LADRC——当前测量、补偿与实际输入
% 教学归属：LADRC闭环；全阶/降阶各自驱动对象，不接TD，不新增C++核心。
% 点击“运行”（F5）执行整个文件，对应docs/21_降阶LESO接入一阶LADRC.md。
repo = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
result = run_example(repo);

function result = run_example(repo)
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file = fullfile(reference_dir,'ADRC_1.m');
assert(isfile(reference_file),'ADRC:MissingReference','请检查ADRC1的ADRC_1.m。');
source_snapshot = fileread(reference_file);
addpath(reference_dir,'-begin');
assert(strcmpi(which('ADRC_1'),reference_file),'ADRC:WrongReference', ...
    '当前ADRC_1不是指定参考文件。');
output_dir = fullfile(repo,'build','lesson21');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 教学设置：第20讲对象与观测带宽，控制带宽沿用已有一阶教学值
p = struct('b0',10,'omega_c',8,'omega_o',20,'h',0.001,'end_time',5, ...
    'initial_y',3,'initial_f_hat',0,'first_target_time',0.5, ...
    'first_target',4,'second_target_time',2.5,'second_target',2, ...
    'load_time',1.5,'load_value',-2,'input_limit',0.3, ...
    'noise_sigma',0.001,'seed',20261028,'tail_time',4.5);
p.beta1 = 2*p.omega_o; p.beta2 = p.omega_o^2;
assert(p.omega_c*p.h > 0 && p.omega_c*p.h < 2 ...
    && p.omega_o*p.h > 0 && p.omega_o*p.h < 2, ...
    'ADRC:InvalidStep','理想离散极点配置不满足要求。');
time = (0:round(p.end_time/p.h))'*p.h;
N = numel(time);
target = p.initial_y*ones(N,1);
target(time >= p.first_target_time) = p.first_target;
target(time >= p.second_target_time) = p.second_target;
true_f = p.load_value*double(time >= p.load_time);
rng_before = rng;
stream = RandStream('mt19937ar','Seed',p.seed);
noise = p.noise_sigma*randn(stream,N,1);
teaching = struct('lesson','第21讲','algorithm','降阶LESO的一阶LADRC闭环', ...
    'topic','当前扰动重构、静态控制计算、限幅与实际输入驱动状态预测', ...
    'document','docs/21_降阶LESO接入一阶LADRC.md','reference_code',reference_file, ...
    'parameter_source','第7/20讲教学参数与补充闭环场景，非原完整模型或实机配置');
fprintf('\n【第21讲：降阶LESO接入一阶LADRC】\n');
fprintf('顺序：测量 -> 当前扰动重构 -> 控制请求 -> 外部限幅 -> 用实际输入推进观测器。\n');
fprintf('每种控制器驱动自己的对象；噪声组仅共用同一噪声序列。\n');

%% 六条独立闭环：三种工况，各比较全阶与降阶
conditions = ["clean_unlimited","clean_limited","noisy_limited"];
observers = ["reduced","full"];
runs = cell(3,2);
metrics_rows = zeros(6,7);
metric_condition = strings(6,1); metric_observer = strings(6,1);
source_control_max = 0; source_observer_max = 0;
recurrence_max = zeros(3,1); analytic_max = zeros(2,1);
independent_closed_loop_max = 0;
for condition_id = 1:3
    limited = condition_id ~= 1;
    measurement_noise = noise*double(condition_id == 3);
    for observer_id = 1:2
        reduced = observer_id == 1;
        y = p.initial_y;
        initial_measurement = y+measurement_noise(1);
        q = p.initial_f_hat-p.omega_o*initial_measurement;
        z = [initial_measurement;p.initial_f_hat];
        reference_x = [0;0;z];
        common = zeros(N,15);
        if reduced, state_records = zeros(N,2); else, state_records = zeros(N,4); end
        for k = 1:N
            % 1. 本拍采样。这里的真实y只在对象侧，控制器接收measurement。
            measurement = y+measurement_noise(k);
            % 2. 先重构当前扰动；本拍f_hat与尚未决定的u[k]无关。
            if reduced
                f_hat = q+p.omega_o*measurement;
                feedback_output = measurement;
            else
                f_hat = z(2);
                feedback_output = z(1);
            end
            % 3. 静态控制计算；复用既有一阶控制律，不推进观测状态。
            requested = adrc_control(target(k),[feedback_output;f_hat],p.b0,p.omega_c);
            reference_command = ADRC_1(time(k),[target(k);0;feedback_output;f_hat], ...
                zeros(3,1),3,1,p.h,p.beta1,p.beta2,p.omega_c);
            source_control_max = max(source_control_max,abs(requested-reference_command(1)/p.b0));
            % 4. 调用方完成最终限幅并向对象提交；观测器使用同一区间实际输入。
            applied = requested;
            if limited, applied = max(-p.input_limit,min(p.input_limit,requested)); end
            observer_input = applied;
            % 5. 每个观测器只推进一次。降阶输出仍标为本拍，q_next属于下一拍。
            if reduced
                [q_next,reconstructed] = leso_reduced_step(q,observer_input,measurement, ...
                    p.b0,p.omega_o,p.h);
                assert(reconstructed == f_hat,'ADRC:Reconstruction','本拍扰动重构不一致。');
                state_records(k,:) = [q,q_next];
            else
                z_next = adrc_eso_step(z,observer_input,measurement, ...
                    p.b0,p.beta1,p.beta2,p.h);
                reference_next = ADRC_1(time(k),reference_x, ...
                    [0;measurement;p.b0*observer_input],2,1,p.h,p.beta1,p.beta2,p.omega_c);
                reference_next = reference_next(:);
                source_observer_max = max(source_observer_max,max(abs(z_next-reference_next(3:4))));
                state_records(k,:) = [z',z_next'];
            end
            % 6. 仿真对象精确推进：区间内输入与扰动恒定。真实f不送给算法。
            true_rate = p.b0*applied+true_f(k);
            y_next = y+p.h*true_rate;
            common(k,:) = [time(k),target(k),y,measurement,measurement_noise(k), ...
                true_f(k),feedback_output,f_hat,requested,applied,observer_input, ...
                p.h,y_next,true_rate,double(applied ~= requested)];
            if k < N
                y = y_next;
                if reduced
                    q = q_next;
                else
                    z = z_next; reference_x = reference_next;
                end
            end
        end
        T = array2table(common,'VariableNames',{'time','target','true_y','measurement', ...
            'noise','true_f','feedback_output','f_hat','requested_input','applied_input', ...
            'observer_input','dt','y_next','true_rate','saturated'});
        if reduced
            states = array2table(state_records,'VariableNames',{'q','q_next'});
            assert(isequal(states.q_next(1:end-1),states.q(2:end)), ...
                'ADRC:StateTiming','q的当前与下一拍状态不连续。');
            expected_f = (1-p.omega_o*p.h)*T.f_hat(1:end-1) ...
                +p.omega_o*p.h*T.true_f(1:end-1)+p.omega_o*diff(T.noise);
            recurrence_max(condition_id) = max(abs(T.f_hat(2:end)-expected_f));
            assert(recurrence_max(condition_id) < 1e-10, ...
                'ADRC:RecurrenceMismatch','限幅闭环中的降阶独立递推不一致。');
            if condition_id <= 2
                n = max(round((time-p.load_time)/p.h),0);
                analytic_f = p.load_value*(1-(1-p.omega_o*p.h).^n);
                analytic_max(condition_id) = max(abs(T.f_hat-analytic_f));
                assert(analytic_max(condition_id) < 1e-10, ...
                    'ADRC:AnalyticMismatch','已知实际输入未被正确抵消。');
            end
            if condition_id == 1
                % 独立闭环递推仅保留[y;f_hat]，不使用q或主循环测量。
                independent = zeros(N,2); independent(1,:) = [p.initial_y,p.initial_f_hat];
                M = [1-p.h*p.omega_c,-p.h;0,1-p.h*p.omega_o];
                for j = 1:N-1
                    independent(j+1,:) = (M*independent(j,:)' ...
                        +p.h*[p.omega_c*target(j)+true_f(j);p.omega_o*true_f(j)])';
                end
                independent_closed_loop_max = max(abs(independent-[T.true_y,T.f_hat]),[],'all');
                assert(independent_closed_loop_max < 1e-10, ...
                    'ADRC:ClosedLoopMismatch','降阶完整闭环与独立递推不一致。');
            end
        else
            states = array2table(state_records,'VariableNames',{'z1','z2','z1_next','z2_next'});
            assert(isequal(states.z1_next(1:end-1),states.z1(2:end)) ...
                && isequal(states.z2_next(1:end-1),states.z2(2:end)), ...
                'ADRC:StateTiming','全阶状态时间标签不一致。');
        end
        assert(all(isfinite(common),'all') && all(isfinite(state_records),'all') ...
            && isequal(T.observer_input,T.applied_input) ...
            && isequal(T.y_next(1:end-1),T.true_y(2:end)), ...
            'ADRC:IntegrationMismatch','实际输入、对象时序或有限数值检查失败。');
        if limited
            assert(max(abs(T.applied_input)) <= p.input_limit && any(T.saturated), ...
                'ADRC:LimitMismatch','限幅边界或限幅场景未满足。');
        end
        if condition_id <= 2
            tail = time >= p.tail_time;
            assert(abs(mean(T.true_y(tail)-T.target(tail))) < 5e-5 ...
                && abs(mean(T.applied_input(tail))+p.load_value/p.b0) < 5e-5, ...
                'ADRC:EquilibriumMismatch','干净组末段目标或负载维持输入异常。');
        end
        T = [T,states];
        file_base = conditions(condition_id)+"_"+observers(observer_id);
        writetable(T,fullfile(output_dir,char(file_base+".csv")));
        runs{condition_id,observer_id} = T;
        row = (condition_id-1)*2+observer_id;
        tail = time >= p.tail_time;
        load_window = time >= p.load_time & time < p.second_target_time;
        metrics_rows(row,:) = [max(abs(T.requested_input)),max(abs(T.applied_input)), ...
            sum(T.saturated(1:end-1).*T.dt(1:end-1)), ...
            max(abs(T.true_y(load_window)-T.target(load_window))), ...
            mean(T.true_y(tail)-T.target(tail)),mean(T.applied_input(tail)), ...
            std(T.requested_input(tail),1)];
        metric_condition(row) = conditions(condition_id); metric_observer(row) = observers(observer_id);
    end
end
assert(source_control_max < 1e-10 && source_observer_max < 1e-10, ...
    'ADRC:SourceMismatch','与原ADRC_1控制律或全阶状态更新不一致。');
assert(strcmp(source_snapshot,fileread(reference_file)), ...
    'ADRC:ReferenceChanged','原参考文件发生变化。');
assert(isequal(rng_before,rng),'ADRC:RngChanged','全局随机数状态被改变。');
metrics = array2table(metrics_rows,'VariableNames',{'peak_request','peak_applied', ...
    'saturation_time_s','load_window_peak_error','mean_tail_error','mean_tail_input', ...
    'tail_request_std'});
metrics = addvars(metrics,metric_condition,metric_observer,'Before',1, ...
    'NewVariableNames',{'condition','observer'});
load_row = round(p.load_time/p.h)+1;
timing = runs{2,1}(load_row+(0:3),{'time','target','true_y','measurement','true_f', ...
    'q','f_hat','requested_input','applied_input','q_next'});
writetable(metrics,fullfile(output_dir,'closed_loop_metrics.csv'));
writetable(timing,fullfile(output_dir,'load_step_timing.csv'));
result = struct('teaching',teaching,'parameters',p,'conditions',conditions,'observers',observers, ...
    'runs',{runs},'metrics',metrics,'timing',timing,'source_control_max',source_control_max, ...
    'source_observer_max',source_observer_max,'recurrence_max',recurrence_max, ...
    'analytic_max',analytic_max,'independent_closed_loop_max',independent_closed_loop_max, ...
    'rng_unchanged',isequal(rng_before,rng));
save(fullfile(output_dir,'reduced_ladrc_result.mat'),'result');
fprintf('验证通过：6条闭环共%d行；原控制律差值%.3g，原全阶更新差值%.3g。\n', ...
    6*N,source_control_max,source_observer_max);
fprintf('降阶闭环独立递推差值%.3g；所有观测更新均使用实际作用量。\n',independent_closed_loop_max);
disp(metrics); disp(timing);

%% 中文图：跟踪、负载与噪声要分开读
fig = figure('Name','第21讲：降阶LESO接入一阶LADRC', ...
    'Color','w','Position',[70 40 1300 840]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
ax1 = nexttile(layout);
plot(ax1,time,target,'k--',time,runs{2,1}.true_y,time,runs{2,2}.true_y,'LineWidth',1.3);
title(ax1,'干净有限幅：各自对象的速度跟踪'); ylabel(ax1,'角速度（rad/s）');
legend(ax1,'目标','降阶LADRC','全阶LADRC','Location','best');
ax2 = nexttile(layout);
plot(ax2,time,runs{2,1}.requested_input,'--',time,runs{2,1}.applied_input,'LineWidth',1.2);
yline(ax2,p.input_limit,'k:','HandleVisibility','off');
yline(ax2,-p.input_limit,'k:','HandleVisibility','off');
title(ax2,'降阶有限幅：请求与实际输入（反馈后者）'); ylabel(ax2,'输入（A）');
legend(ax2,'控制请求','实际输入','Location','best');
ax3 = nexttile(layout);
plot(ax3,time,true_f,'k--',time,runs{2,1}.f_hat,time,runs{2,2}.f_hat,'LineWidth',1.3);
xlim(ax3,[1.45 2.05]);
title(ax3,'干净有限幅：负载发生后的扰动估计'); ylabel(ax3,'总扰动（rad/s²）');
legend(ax3,'真实f','降阶估计','全阶估计','Location','best');
ax4 = nexttile(layout);
plot(ax4,time,runs{3,1}.requested_input,time,runs{3,2}.requested_input,'LineWidth',1.0);
xlim(ax4,[4.5 4.75]);
title(ax4,'带噪有限幅：闭环末段控制请求波动'); ylabel(ax4,'请求输入（A）');
legend(ax4,'降阶LADRC','全阶LADRC','Location','best');
for ax = [ax1 ax2 ax3 ax4]
    grid(ax,'on'); xlabel(ax,'时间（秒）');
end
title(layout,'第21讲｜降阶LESO的一阶LADRC：当前测量补偿、外部限幅与实际输入反馈');
adrc_export_figure(fig,output_dir,"reduced_ladrc_comparison");
end
