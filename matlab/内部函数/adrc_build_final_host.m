function build = adrc_build_final_host(repo)
% 仅教学/主机验证：构建完整静态库、11个测试和两个最终例程，恢复临时环境PATH。
compiler=getenv('ADRC_CXX');
if isempty(compiler)
    [s,v]=system('where g++');
    if s==0, lines=splitlines(strtrim(string(v)));compiler=char(lines(1));
    else, compiler='D:/msys64/ucrt64/bin/g++.exe'; end
end
assert(isfile(compiler),'ADRC:MissingCompiler','请设置ADRC_CXX为g++完整路径。');
[s,v]=system('where cmake'); assert(s==0,'ADRC:MissingCMake','需要CMake。');
lines=splitlines(strtrim(string(v)));cmake=char(lines(1));
[s,v]=system('where ninja'); assert(s==0,'ADRC:MissingNinja','需要Ninja。');
lines=splitlines(strtrim(string(v)));ninja=char(lines(1));
previous=getenv('PATH');cleanup=onCleanup(@()setenv('PATH',previous)); %#ok<NASGU>
setenv('PATH',[fileparts(compiler) pathsep fileparts(cmake) pathsep fileparts(ninja) pathsep previous]);
directory=fullfile(repo,'build','host');
q=@(v) ['"' char(v) '"'];
check([q(cmake) ' -S ' q(repo) ' -B ' q(directory) ' -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DADRC_BUILD_EXAMPLES=ON -DCMAKE_CXX_COMPILER=' q(compiler) ' -DCMAKE_MAKE_PROGRAM=' q(ninja)]);
check([q(cmake) ' --build ' q(directory) ' --parallel 2']);
ctest=fullfile(fileparts(cmake),'ctest.exe');
test_output=check([q(ctest) ' --test-dir ' q(directory) ' --output-on-failure']);
[~,version]=system([q(compiler) ' --version']);
build=struct('directory',directory,'compiler',compiler,'compiler_version',version, ...
    'test_output',test_output,'eso_runner',fullfile(directory,'nonlinear_eso_runner.exe'), ...
    'control_runner',fullfile(directory,'nonlinear_adrc_example.exe'));
fprintf('C++静态库、11项主机测试与最终例程：通过。\n');
end
function output=check(command)
[status,output]=system(command);
assert(status==0,'ADRC:BuildFailure','主机构建/测试失败：\n%s',output);
end
