function report=adrc_verify_suite(repo,include_teaching)
% 自动验证：检验程序计算，不代表个人学完课程或机器人已通过实测。
previous=path;previous_env=getenv('PATH');previous_rng=rng;previous_pwd=pwd;
visibility=get(groot,'defaultFigureVisible');cleanup=onCleanup(@()restore(previous,visibility)); %#ok<NASGU>
set(groot,'defaultFigureVisible','off');
build=adrc_build_final_host(repo);
lessons=[];
if include_teaching
    for id=1:25
        entries=dir(fullfile(repo,'matlab','教学',sprintf('第%02d讲_*',id)));
        assert(numel(entries)==1);run_entry(fullfile(entries.folder,entries.name,'run_lesson.m'));
        close all;lessons(end+1)=id; %#ok<AGROW>
        fprintf('TEACHING_ENTRY_PASS %02d\n',id);
    end
end
legacy=run_legacy(fullfile(repo,'matlab','验证','第30讲_库使用与最终验证','run_lesson.m'));
close all;
applications=cell(4,1);
for id=1:4
    applications{id}=adrc_run_application(repo,id,build);close all;
end
files={'main/ALG_ADRC.hpp','main/ALG_ADRC.cpp','examples/applications/application_runner.cpp', ...
    'matlab/内部函数/adrc_application_parameters.m','matlab/内部函数/adrc_application_reference.m'};
hashes=struct('path',files,'sha256',cell(size(files)));
for index=1:numel(files),hashes(index).sha256=sha256(fullfile(repo,files{index}));end
assert(strcmp(getenv('PATH'),previous_env) && isequal(rng,previous_rng) && strcmp(pwd,previous_pwd));
report=struct('purpose','自动验证','date',char(datetime('today','Format','yyyy-MM-dd')), ...
    'matlab',version,'passed',true,'cxx_standard',11,'host_test_count',build.test_count, ...
    'teaching_entries_checked',[lessons 26:29],'legacy_primary_updates',sum(legacy.primary_trajectory_updates), ...
    'application_native_updates',sum(cellfun(@(r)r.native_updates,applications)), ...
    'application_replay_max',{cellfun(@(r)r.replay_max,applications,'UniformOutput',false)}, ...
    'application_closed_max',{cellfun(@(r)r.closed_max,applications,'UniformOutput',false)}, ...
    'source_hashes',hashes,'firmware_linked',false,'hardware_tested',false, ...
    'environment_path_restored',true,'rng_unchanged',true,'working_directory_restored',true);
assert(isscalar(report));
out=fullfile(repo,'build','verification');if ~isfolder(out),mkdir(out);end
save(fullfile(out,'final_result.mat'),'report','applications','legacy','build');
fid=fopen(fullfile(out,'final_verification.json'),'w','n','UTF-8');assert(fid>=0);
fprintf(fid,'%s',jsonencode(report,'PrettyPrint',true));fclose(fid);
fprintf('FULL_VERIFICATION_PASS tests=%d teaching=%d applications=%d legacy_updates=%d application_updates=%d\n', ...
    report.host_test_count,numel(report.teaching_entries_checked),4,report.legacy_primary_updates,report.application_native_updates);
end
function run_entry(entry)
run(entry);
end
function final=run_legacy(entry)
run(entry);final=result;
end
function restore(previous,visibility)
path(previous);set(groot,'defaultFigureVisible',visibility);
end
function hash=sha256(file)
fid=fopen(file,'rb');assert(fid>=0);cleanup=onCleanup(@()fclose(fid)); %#ok<NASGU>
bytes=fread(fid,inf,'*uint8');digest=java.security.MessageDigest.getInstance('SHA-256');
digest.update(typecast(bytes,'int8'));raw=typecast(digest.digest(),'uint8');
hash=lower(reshape(dec2hex(raw,2)',1,[]));
end
