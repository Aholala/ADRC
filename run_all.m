function data = run_all(reference_root)
%RUN_ALL 从 ADRC1 原始模型运行 MATLAB/Simulink 基线。
% 教学分类：补充原例，二阶LADRC与三阶LESO；不是顺序课程的“全部章节”。
% 默认参考位置来自用户指定路径；不使用早期自选参数的实验。
if nargin == 0
    reference_root = 'D:/ACE/ADRC/ADRC1';
end
repo = fileparts(mfilename('fullpath'));
addpath(fullfile(repo, 'matlab', '内部函数'));
inspect_adrc1_reference(reference_root);
data = run_adrc1_reference(reference_root);
fprintf('ADRC1补充原例复现完成。\n');
end
