%% 应用方案1：单电机转速；仿真参数不是实车参数。
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
previous=path;cleanup=onCleanup(@()path(previous));
addpath(fullfile(repo,'matlab','内部函数'));
result=adrc_run_application(repo,1);
clear cleanup;
