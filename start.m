%% ADRC中文教学启动入口——直接点击“运行”
% 选择课次：7=LADRC与LESO初始化；8=线性TD；9=非线性TD。
% 10=一阶对象的二阶LESO：状态含义与采样时序。
% 11=二阶对象的三阶LESO：角度、角速度与总扰动。
% 12=三阶LESO的带宽与测量噪声。
% 13=二阶LADRC闭环：状态反馈与扰动补偿。
% 14=TD接入二阶LADRC：平滑目标与控制峰值。
% 15=二阶LADRC限幅与实际输入反馈。
% 16=三阶LESO的C++实现与MATLAB对齐（需要g++）。
% 17=二阶LADRC的C++组合与闭环对齐（需要g++）。
% 18=线性TD的C++实现与原函数对齐（需要g++）。
% 19=TD、二阶LADRC与实际输入反馈的完整C++示例（需要g++）。
% 20=一阶对象的降阶LESO：辅助状态、测量依赖与全阶对照。
% 21=降阶LESO接入一阶LADRC：当前测量、补偿与实际输入。
% 22=降阶LESO的最小C++实现与MATLAB逐拍对齐（需要g++）。
% 23=C++降阶一阶LADRC闭环接入与独立MATLAB对照（需要g++）。
% 24=非线性TD的C++实现、原TD2_NL与不同h0核对（需要g++）。
% 25=fal与非线性ESO入门：静态形状、单独纠偏与噪声。
% 26=完整三阶非线性ESO与局部匹配的LESO对照。
% 27=非线性ESO的C++实现（需要g++、CMake、Ninja）。
% 28=原NLSEF_2fal反馈与完整MATLAB闭环。
% 29=完整非线性ADRC的C++接入（需要g++、CMake、Ninja）。
% 30=第一版库最终验收：自动运行26～29讲与11项主机测试。
% 0=补充原例（二阶LADRC与三阶LESO，需要Simulink）。
% 修改下面这一行后，运行整个文件；无需手动设置搜索路径。
lesson_id = 30;

project_root = fileparts(mfilename('fullpath'));
switch lesson_id
    case 7
        lesson_folder = '第07讲_LADRC与LESO初始化';
    case 8
        lesson_folder = '第08讲_线性TD';
    case 9
        lesson_folder = '第09讲_非线性TD';
    case 10
        lesson_folder = '第10讲_二阶LESO状态与时序';
    case 11
        lesson_folder = '第11讲_三阶LESO';
    case 12
        lesson_folder = '第12讲_LESO带宽与噪声';
    case 13
        lesson_folder = '第13讲_二阶LADRC闭环';
    case 14
        lesson_folder = '第14讲_TD接入二阶LADRC';
    case 15
        lesson_folder = '第15讲_限幅与实际输入反馈';
    case 16
        lesson_folder = '第16讲_三阶LESO的C++对齐';
    case 17
        lesson_folder = '第17讲_二阶LADRC的C++组合';
    case 18
        lesson_folder = '第18讲_线性TD的C++实现';
    case 19
        lesson_folder = '第19讲_完整C++接入示例';
    case 20
        lesson_folder = '第20讲_一阶对象的降阶LESO';
    case 21
        lesson_folder = '第21讲_降阶LESO接入一阶LADRC';
    case 22
        lesson_folder = '第22讲_降阶LESO的C++实现';
    case 23
        lesson_folder = '第23讲_C++降阶一阶LADRC闭环接入';
    case 24
        lesson_folder = '第24讲_非线性TD的C++实现';
    case 25
        lesson_folder = '第25讲_fal与非线性ESO入门';
    case 26
        lesson_folder = '第26讲_完整非线性ESO与LESO对照';
    case 27
        lesson_folder = '第27讲_非线性ESO的C++实现';
    case 28
        lesson_folder = '第28讲_非线性误差反馈与闭环';
    case 29
        lesson_folder = '第29讲_完整非线性ADRC的C++接入';
    case 30
        lesson_folder = '第30讲_库使用与最终验证';
    case 0
        lesson_folder = '补充原例_二阶LADRC与三阶LESO';
    otherwise
        error('ADRC:UnknownLesson','课次请选择7～30或0。');
end
fprintf('即将运行：%s\n',lesson_folder);
run(fullfile(project_root,'matlab',lesson_folder,'run_lesson.m'));
