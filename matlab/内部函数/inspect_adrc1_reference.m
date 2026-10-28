function report = inspect_adrc1_reference(reference_root)
%INSPECT_ADRC1_REFERENCE 记录原一阶示例的参数来源和待核实依赖，不猜测 K。
if nargin == 0, reference_root = 'D:/ACE/ADRC/ADRC1'; end
repo = fileparts(fileparts(fileparts(mfilename('fullpath'))));
base = fullfile(reference_root,'贺庆毕业论文ADRC全集包仅用于内部共享不要外传','MyLibrary');
model_path = fullfile(base,'ADRC_1RD_TEST.mdl');
text = fileread(model_path);
report.model_source = model_path;
report.controller_source = fullfile(base,'ADRC_1.m');
block_names = {'ADRC_1rd1','ADRC_1rd2'};
parameter_names = {'r0','h','B01','B02','r'};
for index = 1:numel(block_names)
    token = regexp(text, ['Name\s+"' block_names{index} '"(.*?)\n\s*}'], 'tokens', 'once');
    assert(~isempty(token), 'ADRC:MissingSourceBlock', '原例控制器块已变化。');
    parameters = struct;
    for field = 1:numel(parameter_names)
        value = regexp(token{1}, ['(?m)^\s*' parameter_names{field} '\s+"([^"]+)"'], ...
            'tokens','once');
        assert(~isempty(value), 'ADRC:MissingParameter', '原例参数未找到。');
        parameters.(parameter_names{field}) = str2double(value{1});
    end
    report.controllers.(block_names{index}) = parameters;
end
report.external_gain_expression = 'K';
report.status = ['ADRC_1RD_TEST使用外部K，本独立工作区尚未恢复其配套初始化。' ...
    '同名矩阵只记录为候选，不自动选择；不据此否定原仓库已有验证。'];
report.candidates = struct('file',{},'matrix',{});
files = dir(fullfile(base,'*.mat'));
for index = 1:numel(files)
    file = fullfile(files(index).folder,files(index).name);
    variables = whos('-file',file);
    if any(strcmp({variables.name},'K'))
        value = load(file,'K');
        assert(isnumeric(value.K), 'ADRC:UnexpectedK', 'K 的类型需要人工核对。');
        report.candidates(end+1) = struct('file',file,'matrix',value.K); %#ok<AGROW>
    end
end
work = fullfile(repo,'build','reference_adrc1');
if ~isfolder(work), mkdir(work); end
file = fopen(fullfile(work,'first_order_audit.json'),'w','n','UTF-8');
assert(file >= 0, 'ADRC:WriteFailure', '无法创建审计结果。');
cleanup = onCleanup(@() fclose(file)); %#ok<NASGU>
fprintf(file,'%s\n',jsonencode(report,'PrettyPrint',true));
fprintf('一阶原例依赖检查：h=%g，B01=%g，B02=%g，r=%g；K待恢复（%d个候选）。\n', ...
    report.controllers.ADRC_1rd1.h,report.controllers.ADRC_1rd1.B01, ...
    report.controllers.ADRC_1rd1.B02,report.controllers.ADRC_1rd1.r,numel(report.candidates));
end
