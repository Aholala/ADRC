%% 第25讲：fal与非线性ESO入门——静态形状、单独纠偏与噪声
% 教学归属：非线性ESO基础。本讲不运行完整ESO状态轨迹，不接TD/控制器。
% 直接点击“运行”（F5）。仅需MATLAB与本机ADRC1文件，无需g++或Simulink。
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
result = run_example(repo);

function result = run_example(repo)
old_path = path;
restore_path = onCleanup(@() path(old_path));
addpath(fullfile(repo,'matlab','内部函数'));
reference_dir = fullfile('D:/ACE/ADRC/ADRC1', ...
    '贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
names = {'ESO2_NL','ESO_2rd','ESO_2L'};
source_files = cellfun(@(name) fullfile(reference_dir,[name '.m']),names,'UniformOutput',false);
assert(all(cellfun(@isfile,source_files)),'ADRC:MissingReference','请检查原ESO2_NL、ESO_2rd和ESO_2L。');
snapshots = cellfun(@fileread,source_files,'UniformOutput',false);
addpath(reference_dir,'-begin');
for index = 1:numel(names)
    assert(strcmpi(which(names{index}),source_files{index}), ...
        'ADRC:WrongReference','当前参考函数不是指定ADRC1文件。');
end
output_dir = fullfile(repo,'build','lesson25');
if ~isfolder(output_dir), mkdir(output_dir); end
p = struct('alpha_values',[1 0.5 0.25],'delta',0.01,'delta_values',[0.005 0.01 0.04], ...
    'h',0.001,'end_time',2.5,'correction_gain',4,'initial_error',2, ...
    'noise_sigma',0.001,'seed',20261028,'probe_b0',10,'probe_beta',[3;3;1]);
teaching = struct('lesson','第25讲','algorithm','fal静态纠偏与非线性ESO公式入门', ...
    'topic','连续分段幂函数、小误差区斜率与静态噪声增益', ...
    'document','docs/25_fal与非线性ESO入门.md','reference_files',{source_files}, ...
    'parameter_source','alpha=0.5/0.25来自原ESO_2rd；其他数值为无量纲教学实验和导数探针', ...
    'scope','单独纠偏动态不是完整ESO，原ESO仅核对当前导数接口');
fprintf('\n【第25讲：fal与非线性ESO入门】\n');
fprintf('本讲只有静态映射、单独纠偏环节与原ESO导数探针，没有完整ESO闭环。\n');
fprintf('delta=%g；alpha=%s；误差在静态与纠偏实验中归一化为无量纲数。\n',p.delta,mat2str(p.alpha_values));

%% 一、静态形状与独立性质：alpha=1退化为线性，奇对称、阈值连续
error_grid = linspace(-2,2,4001)';
mapped = zeros(numel(error_grid),3);
inner_gain = p.delta.^(p.alpha_values-1);
property_max = 0;
for index = 1:3
    alpha = p.alpha_values(index);
    mapped(:,index) = adrc_fal(error_grid,alpha,p.delta);
    property_max = max(property_max,max(abs(adrc_fal(-error_grid,alpha,p.delta)+mapped(:,index))));
    boundary = adrc_fal([-p.delta;0;p.delta],alpha,p.delta);
    property_max = max(property_max,max(abs(boundary-[-p.delta^alpha;0;p.delta^alpha])));
    small = [-0.5;0;0.5]*p.delta;
    property_max = max(property_max,max(abs(adrc_fal(small,alpha,p.delta)-small*inner_gain(index))));
    near_boundary = p.delta*[1-1e-8;1+1e-8];
    assert(abs(diff(adrc_fal(near_boundary,alpha,p.delta))) < 1e-7, ...
        'ADRC:FalContinuity','阈值两侧未呈现连续性。');
end
assert(max(abs(mapped(:,1)-error_grid)) == 0 && property_max < 1e-12, ...
    'ADRC:FalProperties','线性退化、奇对称、阈值或小误差斜率核对失败。');
zoom_error = linspace(-0.05,0.05,2001)';
delta_curves = zeros(numel(zoom_error),3);
for index = 1:3
    delta_curves(:,index) = adrc_fal(zoom_error,0.5,p.delta_values(index));
end
shape = array2table([error_grid,mapped],'VariableNames',{'error','alpha_1','alpha_05','alpha_025'});
writetable(shape,fullfile(output_dir,'fal_shape.csv'));

%% 二、原三个状态的当前导数探针：保持输入顺序、线性首行和fal后两行
probe_errors = [-2;-0.04;-p.delta;-0.005;0;0.005;p.delta;0.04;2];
probe_values = zeros(numel(probe_errors),9);
source_derivative_max = 0; source_pair_max = 0; linear_reduction_max = 0;
for index = 1:numel(probe_errors)
    measurement = 0.1; applied = 0.03;
    z = [measurement+probe_errors(index);-0.4;0.2];
    e = z(1)-measurement;
    beta = p.probe_beta;
    expected = [z(2)-beta(1)*e; ...
        z(3)-beta(2)*adrc_fal(e,0.5,p.delta)+p.probe_b0*applied; ...
        -beta(3)*adrc_fal(e,0.25,p.delta)];
    original = ESO2_NL(0,z,[measurement;applied],1,0.5,0.25,p.delta,beta,p.probe_b0);
    original = original(:);
    other = ESO_2rd(0,z,[applied;measurement],1,p.delta,beta,p.probe_b0);
    source_derivative_max = max(source_derivative_max,max(abs(expected-original)));
    source_pair_max = max(source_pair_max,max(abs(original-other(:))));
    all_linear = ESO2_NL(0,z,[measurement;applied],1,1,1,p.delta,beta,p.probe_b0);
    linear_original = ESO_2L(0,z,[applied;measurement],1,beta,p.probe_b0);
    linear_reduction_max = max(linear_reduction_max,max(abs(all_linear(:)-linear_original(:))));
    probe_values(index,:) = [e,adrc_fal(e,0.5,p.delta),adrc_fal(e,0.25,p.delta), ...
        expected',original'];
end
assert(max([source_derivative_max source_pair_max linear_reduction_max]) < 1e-12, ...
    'ADRC:SourceMismatch','原ESO导数、输入顺序或alpha=1退化核对失败。');
probes = array2table(probe_values,'VariableNames',{'error','fal_05','fal_025', ...
    'extracted_dz1','extracted_dz2','extracted_dz3','original_dz1','original_dz2','original_dz3'});
writetable(probes,fullfile(output_dir,'original_eso_derivative_probes.csv'));

%% 三、单独纠偏e_dot=-k*fal(e)，仅为解释形状，不是完整非线性ESO
time = (0:round(p.end_time/p.h))'*p.h;
N = numel(time);
errors = zeros(N,3); errors(1,:) = p.initial_error;
next_errors = zeros(N,3); settling = zeros(3,1);
local_poles = 1-p.h*p.correction_gain*inner_gain;
assert(all(local_poles > 0 & local_poles < 1), ...
    'ADRC:InvalidStep','单独纠偏实验采用正的局部欧拉极点。');
for index = 1:3
    for k = 1:N
        phi = adrc_fal(errors(k,index),p.alpha_values(index),p.delta);
        next_errors(k,index) = errors(k,index)-p.h*p.correction_gain*phi;
        if k < N, errors(k+1,index) = next_errors(k,index); end
    end
    assert(all(errors(:,index) >= 0) && all(diff(errors(:,index)) <= 1e-14) ...
        && errors(end,index) < 0.001,'ADRC:CorrectionExample','当前单独纠偏工况未单调收敛。');
    last_outside = find(errors(:,index) > 0.02*p.initial_error,1,'last');
    assert(~isempty(last_outside) && last_outside < N,'ADRC:NotSettled','当前工况未进入2%误差带。');
    settling(index) = time(last_outside+1);
end
linear_expected = p.initial_error*(1-p.h*p.correction_gain).^(0:N-1)';
linear_analytic_max = max(abs(errors(:,1)-linear_expected));
assert(linear_analytic_max < 1e-12,'ADRC:AnalyticMismatch','线性纠偏与独立离散解析解不一致。');
assert(isequal(next_errors(1:end-1,:),errors(2:end,:)), ...
    'ADRC:StateTiming','纠偏环节当前状态与下一拍预测标签不一致。');
correction = array2table([time,errors,next_errors],'VariableNames', ...
    {'time','error_alpha_1','error_alpha_05','error_alpha_025', ...
    'next_alpha_1','next_alpha_05','next_alpha_025'});
writetable(correction,fullfile(output_dir,'isolated_correction.csv'));

%% 四、相同噪声直接通过静态fal，不把它误称为观测器噪声滤波
rng_before = rng;
stream = RandStream('mt19937ar','Seed',p.seed);
noise = p.noise_sigma*randn(stream,N,1);
assert(max(abs(noise)) < p.delta,'ADRC:NoiseRange','本教学噪声序列需要完全落在小误差区。');
noise_mapped = zeros(N,3);
for index = 1:3
    noise_mapped(:,index) = adrc_fal(noise,p.alpha_values(index),p.delta);
end
noise_rms = sqrt(mean(noise.^2));
mapped_rms = sqrt(mean(noise_mapped.^2,1));
noise_gain = mapped_rms/noise_rms;
assert(max(abs(noise_gain-inner_gain)) < 1e-10, ...
    'ADRC:NoiseGain','小误差区静态噪声增益与斜率不一致。');
assert(isequal(rng_before,rng),'ADRC:RngChanged','全局随机数状态被改变。');
noise_table = array2table([time,noise,noise_mapped],'VariableNames', ...
    {'time','noise','mapped_alpha_1','mapped_alpha_05','mapped_alpha_025'});
writetable(noise_table,fullfile(output_dir,'static_noise_response.csv'));
for index = 1:numel(source_files)
    assert(strcmp(snapshots{index},fileread(source_files{index})), ...
        'ADRC:ReferenceChanged','原参考文件发生变化。');
end
metrics = table(p.alpha_values(:),repmat(p.delta,3,1),inner_gain(:),local_poles(:), ...
    settling,noise_gain(:),mapped_rms(:), ...
    'VariableNames',{'alpha','delta','inner_slope','isolated_local_pole', ...
    'isolated_settling_2pct_s','static_noise_rms_gain','static_mapped_noise_rms'});
writetable(metrics,fullfile(output_dir,'fal_metrics.csv'));
result = struct('teaching',teaching,'parameters',p,'shape',shape,'probes',probes, ...
    'correction',correction,'noise',noise_table,'metrics',metrics, ...
    'property_max',property_max,'source_derivative_max',source_derivative_max, ...
    'source_pair_max',source_pair_max,'linear_reduction_max',linear_reduction_max, ...
    'linear_analytic_max',linear_analytic_max,'rng_unchanged',isequal(rng_before,rng));
save(fullfile(output_dir,'fal_intro_result.mat'),'result');
fprintf('验证通过：原ESO导数最大差值%.3g，两个原非线性ESO导数差值%.3g，alpha=1退化差值%.3g。\n', ...
    source_derivative_max,source_pair_max,linear_reduction_max);
fprintf('三组小误差斜率=%s；本次静态噪声RMS增益=%s。\n',mat2str(inner_gain,6),mat2str(noise_gain,6));
disp(metrics);

%% 中文图：三类实验明确分开，避免把静态映射当作完整ESO
fig = figure('Name','第25讲：fal与非线性ESO入门','Color','w','Position',[70 40 1300 840]);
layout = tiledlayout(fig,2,2,'TileSpacing','compact');
labels = {'alpha=1（线性）','alpha=0.5','alpha=0.25'};
ax1 = nexttile(layout);
plot(ax1,error_grid,mapped,'LineWidth',1.3);
title(ax1,'静态形状：误差量级不同，增益也不同'); ylabel(ax1,'fal(e)（无量纲）');
xlabel(ax1,'误差e（无量纲）'); legend(ax1,labels,'Location','best'); grid(ax1,'on');
ax2 = nexttile(layout);
plot(ax2,zoom_error,delta_curves,'LineWidth',1.3);
title(ax2,'固定alpha=0.5：delta改变线性区宽度与斜率'); ylabel(ax2,'fal(e)（无量纲）');
xlabel(ax2,'误差e（无量纲）');
legend(ax2,'delta=0.005','delta=0.01','delta=0.04','Location','best'); grid(ax2,'on');
ax3 = nexttile(layout);
plot(ax3,time,errors,'LineWidth',1.3); xlim(ax3,[0 1.5]);
title(ax3,'单独纠偏e_dot=-4*fal(e)，不是完整ESO'); ylabel(ax3,'纠偏误差e（无量纲）');
xlabel(ax3,'时间（秒）'); legend(ax3,labels,'Location','best'); grid(ax3,'on');
ax4 = nexttile(layout);
plot(ax4,time,noise_mapped,'LineWidth',1.0); xlim(ax4,[0.5 0.6]);
title(ax4,'同一份小噪声：静态fal可能放大幅值'); ylabel(ax4,'静态映射输出（无量纲）');
xlabel(ax4,'时间（秒）'); legend(ax4,labels,'Location','best'); grid(ax4,'on');
title(layout,'第25讲｜fal与非线性ESO入门：原公式、单独纠偏与静态噪声增益');
adrc_export_figure(fig,output_dir,"fal_intro");
end
