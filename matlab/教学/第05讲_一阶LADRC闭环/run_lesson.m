%% 教学：第5 讲，先理解本讲概念，再看曲线。
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
previous=path;cleanup=onCleanup(@()path(previous));
addpath(fullfile(repo,'matlab','内部函数'));
result=adrc_basic_lesson(repo,5);
clear cleanup;
