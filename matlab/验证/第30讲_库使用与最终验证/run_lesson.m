%% 第30讲：第一版ADRC库收尾——统一主机构建和第26～29讲验收
% F5运行整个文件。需要MATLAB、g++、CMake、Ninja与本机ADRC1参考文件。
% 不运行ARM工具链或实机；ARM验证另见tests/verify_arm.ps1。
repo=fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));
result=run_final(repo);
function report=run_final(repo)
previous=path;cleanup=onCleanup(@()path(previous));
previous_env=getenv('PATH');previous_rng=rng;previous_pwd=pwd;
folders={'第26讲_完整非线性ESO与LESO对照','第27讲_非线性ESO的C++实现', ...
    '第28讲_非线性误差反馈与闭环','第29讲_完整非线性ADRC的C++接入'};
results=cell(4,1);
for index=1:4
    fprintf('\n===== 第%d讲最终验证 =====\n',index+25);
    run(fullfile(repo,'matlab','教学',folders{index},'run_lesson.m'));
    results{index}=result;
end
assert(isequal(cellfun(@(r)r.lesson,results),(26:29)'));
assert(strcmp(getenv('PATH'),previous_env) && isequal(rng,previous_rng) && strcmp(pwd,previous_pwd));
clear cleanup;
assert(strcmp(path,previous));
updates=[sum(cellfun(@height,results{1}.records),'all');sum(cellfun(@height,results{2}.runs)); ...
    sum(cellfun(@height,results{3}.runs),'all');sum(cellfun(@height,results{4}.runs))];
report=struct('lesson',30,'teaching','第一版ADRC基础库的主机和MATLAB最终验收', ...
    'date',char(datetime('today','Format','yyyy-MM-dd')),'matlab',version,'passed',true,'validated_lessons',[26 27 28 29], ...
    'primary_trajectory_updates',updates','host_test_count',results{4}.build.test_count, ...
    'path_restored',true,'environment_path_restored',true,'rng_unchanged',true, ...
    'working_directory_restored',true,'firmware_linked',false,'hardware_tested',false, ...
    'lesson26_source_max',max(results{1}.reference_max,[],'all'), ...
    'lesson27_metrics',table2struct(results{2}.metrics), ...
    'lesson28_source_max',max(results{3}.reference_max,[],'all'), ...
    'lesson29_metrics',table2struct(results{4}.metrics), ...
    'host_test_output',results{4}.build.test_output);
out=fullfile(repo,'build','lesson30');if ~isfolder(out),mkdir(out);end
save(fullfile(out,'final_result.mat'),'report','results');
fid=fopen(fullfile(out,'final_verification.json'),'w','n','UTF-8');assert(fid>=0);
fprintf(fid,'%s',jsonencode(report,'PrettyPrint',true));fclose(fid);
fprintf('\n原课程公式验收通过：%d项C++测试，第26～29讲主要轨迹共%d次更新。\n',report.host_test_count,sum(updates));
fprintf('第一版库完成。当前证据为主机与MATLAB验证，实机接入和参数验证由消费工程完成。\n');
end
