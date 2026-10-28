function data = run_adrc1_reference(reference_root)
%RUN_ADRC1_REFERENCE 原样运行 ADRC1 的 Motion_acc03 模型与 tuning 参数。
% 教学分类：补充原例——二阶LADRC控制器 + 三阶LESO观测器。
% 对应讲义：docs/00_ADRC1原例与复现基线.md。
% 只在 build/ 中创建工作副本；对模型的内存修改仅为信号记录配置。
% 不修改原模型、调参、改求解器、改参考轨迹或新增被控对象。
arguments
    reference_root (1,:) char = 'D:/ACE/ADRC/ADRC1'
end
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
fprintf('\n【补充原例：ADRC1二阶LADRC与三阶LESO】\n');
fprintf('教学用途：查看原始模型的轨迹、反馈、扰动补偿和三个观测状态。\n');
source_dir = fullfile(reference_root, 'LADRC实现参考', ...
    'MATLABDemo-LADRC 实现(高志强老师)', 'LADRC Demo');
model_source = fullfile(source_dir, 'Motion_acc03.mdl');
tuning_source = fullfile(source_dir, 'tuning.m');
assert(isfile(model_source) && isfile(tuning_source), ...
    'ADRC:MissingReference', 'ADRC1 原始模型或 tuning.m 不存在。');
model = 'Motion_acc03';
assert(~bdIsLoaded(model), 'ADRC:ModelAlreadyLoaded', ...
    '请先关闭已打开的 Motion_acc03，避免覆盖现有模型的内存修改。');
work = fullfile(repo, 'build', 'reference_adrc1');
if ~isfolder(work), mkdir(work); end
model_copy = fullfile(work, 'Motion_acc03.mdl');
tuning_copy = fullfile(work, 'tuning.m');
copyfile(model_source, model_copy);
copyfile(tuning_source, tuning_copy);
source_bytes = read_bytes(model_source);
tuning_bytes = read_bytes(tuning_source);
assert(isequal(source_bytes, read_bytes(model_copy)) && ...
       isequal(tuning_bytes, read_bytes(tuning_copy)), 'ADRC:CopyMismatch', ...
       '工作副本必须与原始文件字节一致。');

% 执行原 tuning.m 的参数赋值；只将原 sim(...) 调用替换为带日志的 sim。
% 原参数表达式本身不改写，不手工填写另一组参数。
initialization = fileread(tuning_copy);
pattern = "(?m)^\s*sim\s*\(\s*'Motion_acc03'\s*\)\s*;\s*$";
assert(numel(regexp(initialization, pattern)) == 1, 'ADRC:UnexpectedTuning', ...
    'tuning.m 结构变化，需要重新核对原始初始化流程。');
initialization = regexprep(initialization, pattern, '');
eval(initialization);
parameters = struct('h', h, 'b0', b0, 'wo', wo, 'wc', wc, 'kp', kp, 'kd', kd);

load_system(model_copy);
model_cleanup = onCleanup(@() close_system(model, 0)); %#ok<NASGU>
% 仿真缓存放在本项目内；函数退出后恢复原来的生成目录设置。
old_generation = Simulink.fileGenControl('getConfig');
generation_cleanup = onCleanup(@() Simulink.fileGenControl('setConfig', ...
    'config', old_generation)); %#ok<NASGU>
Simulink.fileGenControl('set', 'CacheFolder', fullfile(work, 'cache'), ...
    'CodeGenFolder', fullfile(work, 'codegen'), 'createDir', true);

leso_blocks = find_system(model, 'SearchDepth', 1, 'BlockType', 'StateSpace');
assert(numel(leso_blocks) == 1, 'ADRC:UnexpectedModel', '需要重新核对 LESO 模块。');
leso = leso_blocks{1};
log_output([model '/trap profile'], 1, 'reference');
log_output([model '/plant'], 1, 'output');
log_output([model '/plant'], 2, 'velocity');
log_output([model '/plant'], 3, 'total_disturbance');
log_output([model '/b0'], 1, 'control');
log_output([model '/Sum7'], 1, 'measurement');
log_output(leso, 1, 'leso');

simulation = Simulink.SimulationInput(model);
names = fieldnames(parameters);
for index = 1:numel(names)
    simulation = simulation.setVariable(names{index}, parameters.(names{index}));
end
simulation = simulation.setModelParameter('SignalLogging', 'on', ...
    'SignalLoggingName', 'logsout', 'ReturnWorkspaceOutputs', 'on', ...
    'LimitDataPoints', 'off');
output = sim(simulation);
logs = output.logsout;
signal_names = {'reference','output','velocity','total_disturbance', ...
    'control','measurement','leso'};
data = struct;
data.teaching = struct('lesson','补充原例','algorithm','二阶LADRC + 三阶LESO', ...
    'topic','ADRC1的Motion_acc03原模型复现','document','docs/00_ADRC1原例与复现基线.md');
for index = 1:numel(signal_names)
    signal = logs.getElement(signal_names{index}).Values;
    if index == 1
        data.time = signal.Time;
    else
        assert(isequal(signal.Time, data.time), 'ADRC:TimeMismatch', ...
            '日志时间标签不一致；不自动插值或更改仿真。');
    end
    data.(signal_names{index}) = reshape(signal.Data, numel(data.time), []);
    assert(all(isfinite(data.(signal_names{index})), 'all'), ...
        'ADRC:NonfiniteResult', '原始模型产生了非有限日志。');
end
data.parameters = parameters;
data.model_source = model_source;
data.tuning_source = tuning_source;
data.solver = get_param(model, 'Solver');
data.fixed_step_expression = get_param(model, 'FixedStep');
data.stop_time_expression = get_param(model, 'StopTime');

% 检查原模型的实际控制接线：整体除以 b0，kd 的负号来自原 tuning.m。
expected_control = (kp*(data.reference-data.leso(:,1)) ...
                    + kd*data.leso(:,2)-data.leso(:,3))/b0;
data.control_residual = max(abs(data.control-expected_control));
assert(data.control_residual < 1e-9*max(1,max(abs(data.control))), ...
    'ADRC:ControlMismatch', '原始模型控制接线与记录的信号不一致。');

% 当前原例用 ode1。按其原始矩阵核对实际日志，不换成自己的观测器。
A = eval(get_param(leso, 'A'));
B = eval(get_param(leso, 'B'));
assert(strcmp(data.solver, 'ode1'), 'ADRC:SolverChanged', ...
    '求解器已变化，需要重新核对离散日志验证方法。');
derivatives = data.leso(1:end-1,:)*A.' ...
    + [data.control(1:end-1),data.measurement(1:end-1)]*B.';
residual = diff(data.leso)-diff(data.time).*derivatives;
data.observer_residual = max(abs(residual), [], 'all');
assert(data.observer_residual < 1e-7, 'ADRC:ObserverMismatch', ...
    '原模型 LESO 日志与 ode1 更新不一致。');

assert(isequal(source_bytes, read_bytes(model_source)) && ...
       isequal(tuning_bytes, read_bytes(tuning_source)), 'ADRC:ReferenceChanged', ...
       '参考文件发生变化。');
save(fullfile(work,'reference_result.mat'), 'data');
table_data = table(data.time,data.reference,data.output,data.velocity, ...
    data.total_disturbance,data.control,data.measurement, ...
    data.leso(:,1),data.leso(:,2),data.leso(:,3), ...
    'VariableNames', {'time','reference','output','velocity','total_disturbance', ...
    'control','measurement','z1','z2','z3'});
writetable(table_data, fullfile(work,'reference_result.csv'));

fig = figure('Name','补充原例：ADRC1二阶LADRC与三阶LESO','Color','w', ...
    'Position',[70 60 1250 780]);
layout = tiledlayout(fig, 2, 3, 'TileSpacing','compact');
ax = nexttile(layout); plot(ax,data.time,[data.reference,data.output]);
title(ax,'原始目标轨迹与实际输出'); legend(ax,'目标轨迹','实际输出');
ax = nexttile(layout); plot(ax,data.time,[data.output,data.leso(:,1)]);
title(ax,'LESO的输出估计'); legend(ax,'真实输出','输出估计 z1');
ax = nexttile(layout); plot(ax,data.time,[data.velocity,data.leso(:,2)]);
title(ax,'LESO的一阶导数估计'); legend(ax,'真实一阶导数','导数估计 z2');
ax = nexttile(layout); plot(ax,data.time,[data.total_disturbance,data.leso(:,3)]);
title(ax,'LESO的总扰动估计'); legend(ax,'模型总扰动','总扰动估计 z3');
ax = nexttile(layout); plot(ax,data.time,data.control);
title(ax,'LADRC原始控制输出 u');
ax = nexttile(layout); plot(ax,data.time,data.reference-data.output);
title(ax,'目标跟踪误差');
all_axes = findall(fig,'Type','axes');
for index = 1:numel(all_axes)
    grid(all_axes(index),'on'); xlabel(all_axes(index),'时间（s）');
    ylabel(all_axes(index),'原模型单位');
end
title(layout,'补充原例｜ADRC1二阶LADRC与三阶LESO：保留原始模型和参数');
adrc_export_figure(fig, work, "reference_response");
fprintf('验证通过：原模型仿真到 %.9g 秒，共 %d 个采样点。\n', ...
    data.time(end), numel(data.time));
fprintf('h=%g b0=%g wo=%g wc=%g kp=%g kd=%g\n',h,b0,wo,wc,kp,kd);
fprintf('控制律残差：%.9g；LESO前向欧拉更新残差：%.9g。\n', ...
    data.control_residual,data.observer_residual);
end

function log_output(block, port_number, name)
ports = get_param(block,'PortHandles');
set_param(ports.Outport(port_number), 'DataLogging','on', ...
    'DataLoggingNameMode','Custom','DataLoggingName',name, ...
    'DataLoggingLimitDataPoints','off','DataLoggingDecimateData','off');
end

function bytes = read_bytes(path)
file = fopen(path,'rb');
assert(file >= 0, 'ADRC:ReadFailure', '无法读取 %s', path);
cleanup = onCleanup(@() fclose(file)); %#ok<NASGU>
bytes = fread(file, Inf, '*uint8');
end
