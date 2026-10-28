%% 第8讲：线性二阶跟踪微分器（TD）——平滑目标与变化率
% 教学分类：TD目标信号预处理；本讲不连接LESO或LADRC闭环。
% 对应讲义：docs/08_线性TD.md；先看输入与x1，再看x2和解析导数的差别。
% 这是可直接点击“运行”的教学脚本，无需填写函数参数。
% 优先参考：ADRC1/MyLibrary/TD2_L.m和MODEL/TD2_L_MODEL.mdl。
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
reference_file = fullfile(reference_dir,'TD2_L.m');
model_file = fullfile(reference_dir,'MODEL','TD2_L_MODEL.mdl');
assert(isfile(reference_file) && isfile(model_file),'ADRC:MissingTDReference', ...
    '请检查ADRC1参考文件的位置。');
output_dir = fullfile(repo,'build','lesson08');
if ~isfolder(output_dir), mkdir(output_dir); end

%% 从参考模型读取基准参数、输入幅值和正弦频率，不改原文件
model_text = fileread(model_file);
mask = regexp(model_text,'MaskValueString\s+"([^|"]+)\|([^"]+)"','tokens','once');
constant = regexp(model_text,'Name\s+"Constant".*?Value\s+"([^"]+)"','tokens','once');
sine = regexp(model_text,'Name\s+"Sine Wave".*?Frequency\s+"([^"]+)"','tokens','once');
stop = regexp(model_text,'StopTime\s+"([^"]+)"','tokens','once');
assert(~isempty(mask) && ~isempty(constant) && ~isempty(sine) && ~isempty(stop), ...
    'ADRC:TDReferenceChanged','参考模型的参数结构变化，需要重新检查。');
r_base = str2double(mask{1});
h = str2double(mask{2});
amplitude = str2double(constant{1});
end_time = str2double(stop{1});
sine_frequency = eval(sine{1}); % 原模型表达式pi/2，仅在此处求原参数的值。
r_values = [r_base/2,r_base,2*r_base]; % 参数对比属于本讲教学扩展。
assert(all(isfinite([r_values h amplitude end_time sine_frequency])) ...
    && h > 0 && all(r_values > 0) && all(r_values*h < 2), ...
    'ADRC:InvalidTDReference','线性TD参数不在当前教学的有效范围内。');

teaching = struct('lesson','第8讲','algorithm','线性二阶跟踪微分器TD', ...
    'topic','目标过渡与变化率','reference_code',reference_file, ...
    'reference_model',model_file,'document','docs/08_线性TD.md');
fprintf('\n【第8讲：线性二阶跟踪微分器（TD）——平滑目标与变化率】\n');
fprintf('学习目标：分清原输入、平滑值x1、平滑信号变化率x2。\n');
fprintf('原模型基准：r=%g，h=%g秒，恒值输入=%g，正弦角频率=%g弧度/秒。\n', ...
    r_base,h,amplitude,sine_frequency);

%% 临时引用原函数，只调用其离散更新分支flag=2，对照后恢复搜索路径
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(reference_dir,'-begin');
assert(strcmpi(which('TD2_L'),reference_file),'ADRC:WrongTDFunction', ...
    '当前TD2_L并非指定参考文件。');
time = (0:round(end_time/h))'*h;
case_names = ["constant","sine"];
records = zeros(numel(case_names)*numel(r_values)*numel(time),8);
reference_errors = zeros(numel(case_names),numel(r_values));
row = 0;
for scenario = 1:numel(case_names)
    for index = 1:numel(r_values)
        r = r_values(index);
        x = [0;0]; reference_x = [0;0];
        for k = 1:numel(time)
            if scenario == 1
                input = amplitude;
                input_derivative = NaN; % 初始跳变没有有限的普通导数参考线。
            else
                input = sin(sine_frequency*time(k));
                input_derivative = sine_frequency*cos(sine_frequency*time(k));
            end
            row = row+1;
            records(row,:) = [scenario,r,time(k),input,x(1),x(2),input_derivative,h];
            reference_errors(scenario,index) = max(reference_errors(scenario,index), ...
                max(abs(x-reference_x)));
            if k < numel(time)
                x = ltd_step(x,input,r,h);
                reference_x = TD2_L(time(k),reference_x,input,2,r,h);
                reference_x = reference_x(:);
                if scenario == 1
                    n = k; pole = 1-r*h;
                    analytic = [amplitude*(1-pole^n-n*r*h*pole^(n-1)); ...
                                amplitude*n*r*(r*h)*pole^(n-1)];
                    assert(max(abs(x-analytic)) < 1e-9,'ADRC:TDAnalyticMismatch', ...
                        '恒值输入响应与离散解析解不一致。');
                end
            end
        end
    end
end
clear restore_path; % 只恢复临时加入的原参考路径，公共教学函数路径仍可使用。
assert(max(reference_errors,[],'all') < 1e-10,'ADRC:TDReferenceMismatch', ...
    '教学提取公式与原始TD2_L.m不一致。');

T = array2table(records(:,2:end),'VariableNames', ...
    {'r','time','input','x1','x2','input_derivative','h'});
scenario_column = case_names(records(:,1));
T = addvars(T,scenario_column(:),'Before',1,'NewVariableNames','scenario');
writetable(T,fullfile(output_dir,'ltd_sim.csv'));
save(fullfile(output_dir,'ltd_result.mat'),'T','teaching','reference_errors', ...
    'r_base','r_values','h','amplitude','sine_frequency');
fprintf('验证通过：与原TD2_L.m逐拍对照，最大差值%.3g；恒值响应与离散解析解一致。\n', ...
    max(reference_errors,[],'all'));

%% 中文教学图：左列看平滑，右列看变化率
fig = figure('Name','第8讲：线性二阶TD——目标平滑与变化率','Color','w', ...
    'Position',[80 50 1160 800]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
colors = [0.26 0.39 0.67; 0.16 0.52 0.36; 0.82 0.42 0.15];
for scenario = 1:numel(case_names)
    ax1 = nexttile(layout); hold(ax1,'on');
    ax2 = nexttile(layout); hold(ax2,'on');
    base_rows = T(T.scenario==case_names(scenario) & T.r==r_base,:);
    plot(ax1,base_rows.time,base_rows.input,'k--','LineWidth',1.2,'DisplayName','原始输入');
    if scenario == 2
        plot(ax2,base_rows.time,base_rows.input_derivative,'k--','LineWidth',1.2, ...
            'DisplayName','原输入的解析导数');
    end
    for index = 1:numel(r_values)
        rows = T(T.scenario==case_names(scenario) & T.r==r_values(index),:);
        label = sprintf('r=%g',r_values(index));
        if index == 2, label = [label '（原模型基准）']; end
        plot(ax1,rows.time,rows.x1,'Color',colors(index,:),'LineWidth',1.4,'DisplayName',label);
        plot(ax2,rows.time,rows.x2,'Color',colors(index,:),'LineWidth',1.4,'DisplayName',label);
    end
    if scenario == 1
        title(ax1,'恒值输入：x1怎样逐渐跟上目标');
        title(ax2,'恒值输入：x2怎样先增大再回零');
    else
        title(ax1,'正弦输入：平滑与跟踪滞后');
        title(ax2,'正弦输入：x2与原输入导数有差别');
    end
    ylabel(ax1,'输入与平滑值（输入单位）');
    ylabel(ax2,'变化率（输入单位/秒）');
    for ax = [ax1 ax2]
        xlabel(ax,'时间（秒）'); grid(ax,'on'); legend(ax,'show','FontSize',8,'Interpreter','none');
    end
end
title(layout,'第8讲｜线性二阶跟踪微分器TD：参考ADRC1的TD2_L.m，基准h=0.01秒');
adrc_export_figure(fig,output_dir,"ltd_response");
