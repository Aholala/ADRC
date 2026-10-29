%% 第9讲：非线性二阶跟踪微分器（TD）——加速、制动与h0
% 教学分类：非线性TD目标信号预处理；本讲独立观察TD。
% 对应讲义：docs/09_非线性TD.md；先看恒值输入的加速/制动，再比较h0。
% 这是可直接点击“运行”的教学脚本，无需填写函数参数。
% 参考：ADRC1/MyLibrary/TD2_NL.m和ADRC.mdl中的TD2_NL库块默认参数。
repo = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file = fullfile(reference_dir,'TD2_NL.m');
library_file = fullfile(reference_dir,'ADRC.mdl');
model_file = fullfile(reference_dir,'MODEL','TD2_NL_MODEL.mdl');
assert(isfile(reference_file) && isfile(library_file) && isfile(model_file), ...
    'ADRC:MissingReference','请检查第9讲所需的ADRC1参考文件。');
output_dir = fullfile(repo,'build','lesson09');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 读取库块的正值默认参数，并记录独立模型的覆盖值，不修改原文件
library_text = fileread(library_file);
block = regexp(library_text,'FunctionName\s+"TD2_NL"(.*?)MaskValueString\s+"([^"]+)"', ...
    'tokens','once');
assert(~isempty(block),'ADRC:MissingTDMask','未找到原库TD2_NL参数。');
values = str2double(split(string(block{2}),'|'));
assert(numel(values)==3 && all(isfinite(values)) && all(values>0), ...
    'ADRC:InvalidReference','原库默认参数需要重新核对。');
r = values(1); h = values(2); h0_base = values(3);
model_text = fileread(model_file);
override = regexp(model_text,'(?m)^\s*h0\s+"([^"]+)"','tokens','once');
stop = regexp(model_text,'StopTime\s+"([^"]+)"','tokens','once');
sine = regexp(model_text,'Name\s+"Sine Wave".*?Frequency\s+"([^"]+)"','tokens','once');
assert(~isempty(override) && ~isempty(stop) && ~isempty(sine), ...
    'ADRC:ReferenceChanged','原独立模型结构变化，需要重新核对。');
model_h0_override = str2double(override{1});
end_time = str2double(stop{1});
sine_frequency = eval(sine{1}); % 原模型表达式pi/2。
constant_amplitude = 1; % 原模型Constant块未覆盖Value，继承该模型默认值1。
h0_values = h0_base*[1 2 4]; % 正值库基准与两组明确标注的教学参数对比。
teaching = struct('lesson','第9讲','algorithm','非线性二阶跟踪微分器TD', ...
    'topic','加速、制动及实际步长h与内部尺度h0','reference_code',reference_file, ...
    'reference_library',library_file,'reference_model',model_file, ...
    'document','docs/09_非线性TD.md');
fprintf('\n【第9讲：非线性二阶跟踪微分器（TD）——加速、制动与h0】\n');
fprintf('学习目标：解释为什么还没到目标就要减速，区分h、h0和r的含义。\n');
fprintf('库块基准：r=%g，h=%g秒，h0=%g秒；独立模型保存的h0覆盖值=%g。\n', ...
    r,h,h0_base,model_h0_override);

%% 对照原函数的更新分支；原参考文件保持只读
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(reference_dir,'-begin');
assert(strcmpi(which('TD2_NL'),reference_file),'ADRC:WrongReference', ...
    '当前TD2_NL不是指定参考文件。');
if model_h0_override == 0
    singular_result = TD2_NL(0,[0;0],0,2,r,h,0);
    assert(any(~isfinite(singular_result)), 'ADRC:SingularityChanged', ...
        '零h0的原函数行为发生变化，需要重新核对。');
    fprintf('来源说明：零h0在零误差平衡点会产生非有限结果；本讲使用原库正值默认参数。\n');
end

time = (0:round(end_time/h))'*h;
case_names = ["constant","sine"];
records = zeros(numel(case_names)*numel(h0_values)*numel(time),10);
reference_errors = zeros(numel(case_names),numel(h0_values));
peak_accelerations = zeros(size(reference_errors));
row = 0;
for scenario = 1:numel(case_names)
    for index = 1:numel(h0_values)
        h0 = h0_values(index);
        x = [0;0]; reference_x = [0;0];
        for k = 1:numel(time)
            if scenario == 1
                input = constant_amplitude;
                derivative = NaN;
            else
                input = sin(sine_frequency*time(k));
                derivative = sine_frequency*cos(sine_frequency*time(k));
            end
            [x_next,fh] = td_step(x,input,r,h,h0);
            row = row+1;
            records(row,:) = [scenario,h0,time(k),input,x(1),x(2),fh,derivative,r,h];
            reference_errors(scenario,index) = max(reference_errors(scenario,index), ...
                max(abs(x-reference_x)));
            peak_accelerations(scenario,index) = max(peak_accelerations(scenario,index),abs(fh));
            assert(abs(fh) <= r+1e-10*max(1,r),'ADRC:AccelerationBound', ...
                '当前实现违反原fhan的内部变化率变化界限。');
            assert(abs(x_next(2)-x(2)) <= h*r+1e-10*max(1,h*r), ...
                'ADRC:RateIncrementBound','单步变化率增量超出h*r。');
            if k < numel(time)
                reference_x = TD2_NL(time(k),reference_x,input,2,r,h,h0);
                reference_x = reference_x(:);
                x = x_next;
            end
        end
        if scenario == 1
            assert(abs(x(1)-constant_amplitude) < 1e-9 && abs(x(2)) < 1e-8, ...
                'ADRC:ConstantTracking','当前恒值测试没有收敛到目标和平衡变化率。');
        end
    end
end

%% 一个“还没到目标就先减速”的可手算状态
[braking_next,braking_acceleration] = td_step([0.95;2],1,r,h,h0_base);
reference_braking = TD2_NL(0,[0.95;2],1,2,r,h,h0_base);
assert(braking_acceleration < 0 && braking_next(1) < 1, ...
    'ADRC:BrakingDirection','教学状态应在到达目标前开始减速。');
assert(max(abs(braking_next-reference_braking(:))) < 1e-12, ...
    'ADRC:BrakingReference','制动状态与原函数不一致。');
assert(max(reference_errors,[],'all') < 1e-10,'ADRC:ReferenceMismatch', ...
    '非线性TD提取公式与原函数不一致。');
clear restore_path;

T = array2table(records(:,2:end),'VariableNames', ...
    {'h0','time','input','x1','x2','fh','input_derivative','r','h'});
scenario_column = case_names(records(:,1));
T = addvars(T,scenario_column(:),'Before',1,'NewVariableNames','scenario');
writetable(T,fullfile(output_dir,'td_sim.csv'));
save(fullfile(output_dir,'td_result.mat'),'T','teaching','reference_errors', ...
    'peak_accelerations','r','h','h0_base','h0_values','model_h0_override', ...
    'braking_next','braking_acceleration');
fprintf('验证通过：与原TD2_NL.m逐拍对照，最大差值%.3g；恒值收敛、fh界限和提前制动均通过。\n', ...
    max(reference_errors,[],'all'));
fprintf('提前制动例：x1=0.95、x2=2、目标1时，fh=%g，下一拍x1=%g、x2=%g。\n', ...
    braking_acceleration,braking_next(1),braking_next(2));

%% 中文教学图：本讲独立观察非线性TD，不接控制对象或LESO
fig = figure('Name','第9讲：非线性二阶TD——加速、制动与h0','Color','w', ...
    'Position',[60 40 1320 800]);
layout = tiledlayout(fig,2,3,'TileSpacing','compact');
colors = [0.16 0.52 0.36; 0.26 0.39 0.67; 0.82 0.42 0.15];
for scenario = 1:numel(case_names)
    ax1 = nexttile(layout); hold(ax1,'on');
    ax2 = nexttile(layout); hold(ax2,'on');
    ax3 = nexttile(layout); hold(ax3,'on');
    base_rows = T(T.scenario==case_names(scenario) & T.h0==h0_base,:);
    plot(ax1,base_rows.time,base_rows.input,'k--','DisplayName','原始输入');
    if scenario == 2
        plot(ax2,base_rows.time,base_rows.input_derivative,'k--','DisplayName','原输入解析导数');
    end
    yline(ax3,r,'k:','HandleVisibility','off');
    yline(ax3,-r,'k:','HandleVisibility','off');
    for index = 1:numel(h0_values)
        rows = T(T.scenario==case_names(scenario) & T.h0==h0_values(index),:);
        label = sprintf('h0=%g秒',h0_values(index));
        if index == 1, label = [label '（库基准）']; end
        plot(ax1,rows.time,rows.x1,'Color',colors(index,:),'LineWidth',1.3,'DisplayName',label);
        plot(ax2,rows.time,rows.x2,'Color',colors(index,:),'LineWidth',1.3,'DisplayName',label);
        plot(ax3,rows.time,rows.fh,'Color',colors(index,:),'LineWidth',1.3,'DisplayName',label);
    end
    if scenario == 1
        title(ax1,'恒值输入：平滑值 x1'); title(ax2,'恒值输入：变化率 x2');
        title(ax3,'恒值输入：fh先加速再制动'); window = [0 1.5];
    else
        title(ax1,'正弦输入：平滑与滞后'); title(ax2,'正弦输入：变化率估计');
        title(ax3,'正弦输入：内部变化率的变化'); window = [0 8];
    end
    ylabel(ax1,'输入单位'); ylabel(ax2,'输入单位/秒'); ylabel(ax3,'输入单位/秒²');
    for ax = [ax1 ax2 ax3]
        xlim(ax,window); xlabel(ax,'时间（秒）'); grid(ax,'on');
        legend(ax,'show','FontSize',8,'Interpreter','none');
    end
end
title(layout,'第9讲｜非线性二阶TD：加速、制动与h0（固定r=20、实际步长h=0.01秒）');
adrc_export_figure(fig,output_dir,"td_response");
