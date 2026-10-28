# 学习路线与MATLAB教学索引

本项目优先参考 `D:/ACE/ADRC/ADRC1`，按递进路线讲解。
可以拆分或补充教学场景，但明确区分原实现、原参数、教学假设和本次改动。
后续实验统一用 MATLAB，入口说明、命令窗口提示和图注使用中文。

## 1. 先分清名称

| 名称 | 中文含义 | 在控制链中的角色 |
| --- | --- | --- |
| ADRC | 自抗扰控制 | 总体方法，包含线性与非线性等实现 |
| LADRC | 线性自抗扰控制 | 线性状态反馈、扰动补偿和线性观测器构成的控制方案 |
| ESO | 扩张状态观测器 | 状态与总扰动估计的观测器总称 |
| LESO | 线性扩张状态观测器 | ESO 的线性形式，前几讲的误差纠偏是线性的 |
| TD | 跟踪微分器 | 信号过渡与变化率估计，可作为目标处理环节 |

前面已经入门了“一阶LADRC + 二阶全阶LESO”，只是部分标题此前简写为 ESO。
后续仍会专门学习 LESO 和 LADRC，包括二阶对象、降阶结构和不同离散时序。
TD 的两个状态、LESO 的两个状态含义不同，不能按状态名称直接互换。

## 2. 当前可运行的中文MATLAB教学入口

最方便：打开项目根目录 `start.m`，点击“运行”（F5），默认第30讲第一版最终验收（自动运行26～29讲）。
顶部 `lesson_id` 可选7～30或0（补充原例）。也可直接打开中文课次目录里的 `run_lesson.m` 运行整个文件。
这些入口会自行定位仓库和加载内部函数，不依赖预先设置搜索路径。

如果使用下表的相对路径命令，先执行 `cd('D:/ACE/Infantry_dev/ADRC')`。

| 归属 | 教学主题 | 运行命令 | 输出目录 |
| --- | --- | --- | --- |
| 第7讲，一阶LADRC与二阶LESO | 非零状态初始化与接管 | `run('matlab/第07讲_LADRC与LESO初始化/run_lesson.m')` | `build/lesson07/` |
| 第8讲，线性二阶TD | 平滑目标与变化率，参数变化的影响 | `run('matlab/第08讲_线性TD/run_lesson.m')` | `build/lesson08/` |
| 第9讲，非线性二阶TD | 加速、制动及h与h0的区别 | `run('matlab/第09讲_非线性TD/run_lesson.m')` | `build/lesson09/` |
| 第10讲，一阶对象的二阶LESO | 状态含义、当前拍与下一拍预测 | `run('matlab/第10讲_二阶LESO状态与时序/run_lesson.m')` | `build/lesson10/` |
| 第11讲，二阶对象的三阶LESO | 角度、角速度与总扰动 | `run('matlab/第11讲_三阶LESO/run_lesson.m')` | `build/lesson11/` |
| 第12讲，LESO带宽与噪声 | 相同轨迹、相同噪声下比较快慢和抖动 | `run('matlab/第12讲_LESO带宽与噪声/run_lesson.m')` | `build/lesson12/` |
| 第13讲，二阶LADRC闭环 | 状态反馈与扰动补偿组合，比较负载响应 | `run('matlab/第13讲_二阶LADRC闭环/run_lesson.m')` | `build/lesson13/` |
| 第14讲，TD与二阶LADRC | 平滑目标、控制峰值与到位时间 | `run('matlab/第14讲_TD接入二阶LADRC/run_lesson.m')` | `build/lesson14/` |
| 第15讲，限幅与输入反馈 | 请求、实际作用量和LESO模型输入 | `run('matlab/第15讲_限幅与实际输入反馈/run_lesson.m')` | `build/lesson15/` |
| 第16讲，C++三阶LESO | 自动编译主机测试，与MATLAB single/double对齐；需g++ | `run('matlab/第16讲_三阶LESO的C++对齐/run_lesson.m')` | `build/lesson16/` |
| 第17讲，C++二阶LADRC组合 | C++自主闭环与分层对齐；需g++ | `run('matlab/第17讲_二阶LADRC的C++组合/run_lesson.m')` | `build/lesson17/` |
| 第18讲，C++线性TD | 独立状态更新，与原TD2_L和single核对；需g++ | `run('matlab/第18讲_线性TD的C++实现/run_lesson.m')` | `build/lesson18/` |
| 第19讲，完整C++接入示例 | TD、控制器、限幅和实际输入反馈；需g++ | `run('matlab/第19讲_完整C++接入示例/run_lesson.m')` | `build/lesson19/` |
| 第20讲，一阶对象的降阶LESO | 一个辅助状态、测量重构与全阶对照 | `run('matlab/第20讲_一阶对象的降阶LESO/run_lesson.m')` | `build/lesson20/` |
| 第21讲，降阶LESO的一阶LADRC闭环 | 当前测量、扰动补偿、实际输入与全阶闭环对照 | `run('matlab/第21讲_降阶LESO接入一阶LADRC/run_lesson.m')` | `build/lesson21/` |
| 第22讲，C++降阶LESO | 当前测量重构、内部状态更新与分层对齐；需g++ | `run('matlab/第22讲_降阶LESO的C++实现/run_lesson.m')` | `build/lesson22/` |
| 第23讲，C++降阶一阶LADRC接入 | 自主对象、限幅、同测量single与独立double闭环；需g++ | `run('matlab/第23讲_C++降阶一阶LADRC闭环接入/run_lesson.m')` | `build/lesson23/` |
| 第24讲，C++非线性TD | 原fhan分支、实际h与内部h0、single和原函数核对；需g++ | `run('matlab/第24讲_非线性TD的C++实现/run_lesson.m')` | `build/lesson24/` |
| 第25讲，fal与非线性ESO入门 | 静态映射、单独纠偏、噪声增益与原ESO导数 | `run('matlab/第25讲_fal与非线性ESO入门/run_lesson.m')` | `build/lesson25/` |
| 第26讲，完整非线性ESO | 与LESO匹配局部增益、同轨迹同噪声、原导数欧拉化 | `run('matlab/第26讲_完整非线性ESO与LESO对照/run_lesson.m')` | `build/lesson26/` |
| 第27讲，C++非线性ESO | 整库构建、11项测试、single/原double对照 | `run('matlab/第27讲_非线性ESO的C++实现/run_lesson.m')` | `build/lesson27/` |
| 第28讲，非线性误差反馈 | 原NLSEF_2fal及完整MATLAB闭环 | `run('matlab/第28讲_非线性误差反馈与闭环/run_lesson.m')` | `build/lesson28/` |
| 第29讲，完整非线性C++接入 | 自主对象、最终输入反馈、两层核对 | `run('matlab/第29讲_完整非线性ADRC的C++接入/run_lesson.m')` | `build/lesson29/` |
| 第30讲，第一版最终验收 | 自动运行26～29讲，汇总C++测试与MATLAB记录 | `run('matlab/第30讲_库使用与最终验证/run_lesson.m')` | `build/lesson30/` |
| 补充原例，二阶LADRC与三阶LESO | 原始 Motion_acc03 模型复现 | `run('matlab/补充原例_二阶LADRC与三阶LESO/run_lesson.m')` | `build/reference_adrc1/` |

第7～30讲包含分步教学与最终验收，最后一项是完整原例对照，不用它改变课程顺序。
第27、29、30讲需要g++、CMake、Ninja；第26、28讲仅需MATLAB与参考源文件。
第1～6讲的公式和推导继续有效，早期独立实验入口已经停用，不能把旧数据当成新 MATLAB 实验。

每个 `.mat` 结果保存教学归属信息，每张图写明课次和算法类别。
CSV 中的英文变量名用于数据处理，讲义提供中文解释；面向人的图和说明使用中文。

## 3. 阅读公共公式文件时，看清角色

以下文件集中放在 `matlab/内部函数/`，多数需要传入参数，不能直接点击“运行”来启动仿真。
运行入口在上表的中文课次目录中。中文`.m`文件名在本机R2025b直接运行失败，
因此使用中文目录与合法英文入口文件名。

| MATLAB 文件 | 教学归属 | 输入、输出与用途 |
| --- | --- | --- |
| `matlab/内部函数/adrc_eso_step.m` | LESO | 输入实际模型输入和测量，推进两个观测状态 |
| `matlab/内部函数/adrc_leso3_step.m` | 三阶LESO | 仅用输出测量和模型输入，推进三个观测状态 |
| `matlab/内部函数/leso_reduced_step.m` | 第20讲，降阶LESO | 重构当前扰动估计，推进一个辅助状态；直接使用当前输出测量 |
| `matlab/内部函数/adrc_control.m` | LADRC 控制律 | 用目标、输出估计、扰动估计计算控制量，不推进观测器 |
| `matlab/内部函数/adrc_control2.m` | 二阶LADRC控制律 | 位置反馈、速度反馈、扰动补偿；不推进观测器 |
| `matlab/内部函数/ltd_step.m` | 线性TD | 输入一个信号，推进平滑值和变化率两个状态 |
| `matlab/内部函数/td_step.m` | 非线性TD | 按原TD2_NL更新公式推进两个状态 |
| `matlab/内部函数/td_fhan.m` | 非线性TD内部算法 | 根据偏差与当前变化率，计算加速或制动量 |
| `matlab/内部函数/adrc_fal.m` | 第25讲，静态误差映射 | 原ESO使用的分段幂函数，无时间状态，不是独立ESO |
| `matlab/内部函数/adrc_eso3_step.m` | 第26讲，三阶非线性ESO | 原连续导数的明确前向欧拉离散化 |
| `matlab/内部函数/adrc_sef.m` | 第28讲，非线性反馈 | 两个误差的fal反馈与显式扰动补偿 |
| `matlab/内部函数/adrc_final_closed.m` | 第28/29讲完整教学循环 | 算法公式或原模块独立组合，不属于生产C++库 |
| `matlab/内部函数/adrc_final_parameters.m` | 第26～30讲教学配置 | 参数换算和来源，不能作为机器人默认值 |
| `matlab/内部函数/run_adrc1_reference.m` | 补充原例 | 运行原二阶LADRC/三阶LESO模型并导出日志 |

`adrc_control.m` 只是控制律函数；完整 LADRC 在上层将它与 LESO 按时序组合。
同一个符号 `x2` 或 `z2` 必须结合所属模块解释，图标题会明确这一点。

## 4. 后续按小节推进

| 顺序 | 专题 | 要解决的问题 |
| --- | --- | --- |
| 已完成第9讲 | [非线性TD](09_非线性TD.md) | 对照 `TD2_NL.m` 与 `fhan`，理解加速、制动和 h0 |
| 已完成第10讲 | [一阶对象的二阶LESO](10_LESO状态与时序.md) | 状态含义与采样时序 |
| 已完成第11讲 | [二阶对象的三阶LESO](11_三阶LESO.md) | 区分输出、变化率与总扰动三个估计状态 |
| 已完成第12讲 | [LESO带宽与测量噪声](12_LESO带宽与噪声.md) | 在相同对象轨迹和噪声下比较估计速度及噪声影响 |
| 已完成第13讲 | [二阶LADRC闭环](13_二阶LADRC闭环.md) | 位置和速度反馈、总扰动补偿与三阶LESO的组合 |
| 已完成第14讲 | [TD接入二阶LADRC](14_TD接入二阶LADRC.md) | 平滑目标对控制峰值和跟踪过程的影响 |
| 已完成第15讲 | [二阶LADRC限幅与输入反馈](15_限幅与实际输入反馈.md) | 区分原始控制量、实际输入和LESO所用输入 |
| 已完成第16讲 | [三阶LESO最小C++实现](16_三阶LESO的C++实现.md) | 将已学公式整理为可嵌入实现，与MATLAB逐拍对齐 |
| 已完成第17讲 | [二阶LADRC最小C++组合](17_二阶LADRC的C++组合.md) | 保持控制计算和观测更新分开，核对完整闭环 |
| 已完成第18讲 | [线性TD最小C++实现](18_线性TD的C++实现.md) | 独立验证状态与周期，再逐步组合目标处理 |
| 已完成第19讲 | [完整C++接入示例](19_完整C++接入示例.md) | TD、二阶LADRC、调用方限幅和实际输入反馈的组合 |
| 已完成第20讲 | [一阶对象的降阶LESO入门](20_一阶对象的降阶LESO.md) | 辅助状态变换、测量依赖与全阶形式的区别 |
| 已完成第21讲 | [降阶LESO接入一阶LADRC](21_降阶LESO接入一阶LADRC.md) | 当前测量、扰动补偿、控制请求和实际输入的闭环时序 |
| 已完成第22讲 | [一阶对象降阶LESO的最小C++实现](22_降阶LESO的C++实现.md) | 当前估计重构与实际输入驱动更新分开，再与MATLAB逐拍对齐 |
| 已完成第23讲 | [C++降阶一阶LADRC闭环接入](23_C++降阶一阶LADRC闭环接入.md) | 调用方反馈、限幅、实际输入与MATLAB闭环核对 |
| 已完成第24讲 | [非线性TD的最小C++实现](24_非线性TD的C++实现.md) | 对照ADRC1的TD2_NL与fhan，区分实际周期h和算法参数h0 |
| 已完成第25讲 | [fal与非线性ESO入门](25_fal与非线性ESO入门.md) | 从原误差纠偏公式理解分段幂函数，核对LESO退化与静态噪声增益 |
| 已完成第26讲 | [完整非线性ESO](26_完整非线性ESO与LESO对照.md) | 原连续导数的离散化、局部增益匹配与噪声对照 |
| 已完成第27讲 | [C++非线性ESO](27_非线性ESO的C++实现.md) | 原分支、三个状态与single/double核对 |
| 已完成第28讲 | [非线性误差反馈](28_非线性误差反馈与闭环.md) | 两项fal反馈与完整MATLAB闭环 |
| 已完成第29讲 | [完整C++非线性ADRC](29_完整非线性ADRC的C++接入.md) | 自主对象、限幅、实际输入与两层核对 |
| 已完成第30讲 | [第一版库使用与最终验证](30_第一版库使用与最终验证.md) | 统一构建、接口索引、证据与接入说明 |

第一版基础课程到第30讲完成。云台/底盘实机接入、更多阶次/降阶变体、其他离散方案属于可选后续专题，不计入本次基础库的剩余工作。
