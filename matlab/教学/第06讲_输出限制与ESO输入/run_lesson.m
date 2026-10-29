%% 教学：第6 讲，先理解本讲概念，再看曲线。
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
previous=path;cleanup=onCleanup(@()path(previous));
addpath(fullfile(repo,'matlab','内部函数'));
result=adrc_basic_lesson(repo,6);
clear cleanup;
