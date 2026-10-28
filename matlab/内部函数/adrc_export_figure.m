function adrc_export_figure(fig, output_dir, name)
%ADRC_EXPORT_FIGURE 保存可重开的 MATLAB 图、PNG 和 SVG。
% 内部导出函数，由教学脚本调用，不是独立仿真入口。
% 统一中文字体与文本解释方式，避免中文显示成方框或下划线被当作下标。
available = string(listfonts);
candidates = ["Microsoft YaHei","微软雅黑","SimHei","黑体", ...
    "SimSun","宋体","Noto Sans CJK SC"];
font = "Microsoft YaHei";
for candidate = candidates
    if any(strcmpi(available,candidate))
        font = candidate;
        break;
    end
end
set(findall(fig,'-property','FontName'),'FontName',char(font));
set(findall(fig,'-property','Interpreter'),'Interpreter','none');
if ~isfolder(output_dir)
    mkdir(output_dir);
end
savefig(fig, fullfile(output_dir, name + ".fig"));
exportgraphics(fig, fullfile(output_dir, name + ".png"), 'Resolution', 160);
exportgraphics(fig, fullfile(output_dir, name + ".svg"), 'ContentType', 'vector');
fprintf('已保存教学图：%s\n', fullfile(output_dir, name + ".fig"));
end
