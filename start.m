%% ADRC入口：教学、应用方案、自动验证彼此独立
% 模式：'教学'、'应用'、'验证'、'原例'。默认从第1讲学习，不自动运行验收。
% 教学编号1~29；应用编号1~4；验证与原例不使用编号。
mode = '教学';
number = 1;

project_root=fileparts(mfilename('fullpath'));
switch mode
    case '教学'
        entries=dir(fullfile(project_root,'matlab','教学',sprintf('第%02d讲_*',number)));
        assert(numel(entries)==1,'ADRC:Lesson','教学编号请选择1~29。');
        entry=fullfile(entries.folder,entries.name,'run_lesson.m');
    case '应用'
        entries=dir(fullfile(project_root,'matlab','应用方案',sprintf('%02d_*',number)));
        assert(numel(entries)==1,'ADRC:Case','应用编号请选择1~4。');
        entry=fullfile(entries.folder,entries.name,'run_case.m');
    case '验证'
        entry=fullfile(project_root,'matlab','验证','run_checks.m');
    case '原例'
        entry=fullfile(project_root,'matlab','原例对照','补充原例_二阶LADRC与三阶LESO','run_lesson.m');
    otherwise
        error('ADRC:Mode','模式请选择：教学、应用、验证、原例。');
end
fprintf('当前用途：%s；入口：%s\n',mode,entry);
run(entry);
