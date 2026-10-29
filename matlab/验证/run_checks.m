%% 自动验证：检查实现和应用，不代表已经学完课程或通过实机测试。
% true会检查第1~25讲入口；第26~29讲及四类应用始终检查。
include_teaching = true;
repo=fileparts(fileparts(fileparts(mfilename('fullpath'))));
previous=path;cleanup=onCleanup(@()path(previous));
addpath(fullfile(repo,'matlab','内部函数'));
report=adrc_verify_suite(repo,include_teaching);
clear cleanup;
