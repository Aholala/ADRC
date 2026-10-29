%% 应用方案4：带功率限制的四轮控制；仿真参数不是实车参数。
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
previous=path;cleanup=onCleanup(@()path(previous));
addpath(fullfile(repo,'matlab','内部函数'));
result=adrc_run_application(repo,4);
clear cleanup;
