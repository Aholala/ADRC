%% 补充原例：二阶LADRC与三阶LESO——直接点击“运行”
% 需要MATLAB和Simulink；按原tuning.m运行Motion_acc03的工作副本。
% 本脚本自动定位仓库，不需要手动切换当前文件夹。
repo = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
addpath(repo);
data = run_all;
