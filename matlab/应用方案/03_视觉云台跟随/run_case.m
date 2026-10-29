%% 应用方案3：视觉云台跟随；仿真参数不是实车参数。
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
previous=path;cleanup=onCleanup(@()path(previous));
addpath(fullfile(repo,'matlab','内部函数'));
result=adrc_run_application(repo,3);
clear cleanup;
