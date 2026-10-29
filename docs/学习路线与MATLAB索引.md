# 学习路线与MATLAB索引

本页管理学习顺序，不用于证明库已经学完或机器人已经验证。

从定义、直观例子开始，再看当前问题需要的公式和代码。一次推进当前课次，不因库已完成而跳过前置知识。

## 三种用途分别进入

- 教学：打开start.m，保持mode='教学'，number选择1~29；默认从第1讲开始。
- 应用：mode='应用'，number选择1~4；直接运行完整轮速、舵向、视觉跟随或四轮功率限制仿真。
- 验证：mode='验证'；自动检查程序，不记录为个人学习进度。
- 原例：mode='原例'；运行ADRC1模型工作副本，额外需要Simulink。

目录中文，运行文件为run_lesson.m、run_case.m、run_checks.m；内部函数需要参数，不直接F5运行。

## 教学目录

| 课次 | 讲义 | MATLAB入口 |
| --- | --- | --- |
| 1 | [第 1 步：一阶对象与总扰动](01_一阶对象与总扰动.md) | [run_lesson.m](../matlab/教学/第01讲_一阶对象与总扰动/run_lesson.m) |
| 2 | [第2讲：一阶对象的二阶线性扩张状态观测器（LESO）](02_二阶线性ESO.md) | [run_lesson.m](../matlab/教学/第02讲_二阶LESO的两个估计量/run_lesson.m) |
| 3 | [第3讲：二阶LESO的增益、带宽与采样周期](03_ESO增益与采样周期.md) | [run_lesson.m](../matlab/教学/第03讲_LESO纠偏快慢与测量抖动/run_lesson.m) |
| 4 | [第4讲：二阶LESO的离散实现与开环仿真](04_离散ESO与仿真.md) | [run_lesson.m](../matlab/教学/第04讲_逐次更新与采样周期/run_lesson.m) |
| 5 | [第5讲：一阶LADRC的状态反馈、扰动补偿与闭环](05_一阶LADRC闭环.md) | [run_lesson.m](../matlab/教学/第05讲_一阶LADRC闭环/run_lesson.m) |
| 6 | [第6讲：一阶LADRC的输出限幅与LESO输入反馈](06_输出限幅与输入反馈.md) | [run_lesson.m](../matlab/教学/第06讲_输出限制与ESO输入/run_lesson.m) |
| 7 | [第7讲：一阶LADRC与二阶LESO的初始化与无扰接管](07_初始化与无扰接管.md) | [run_lesson.m](../matlab/教学/第07讲_LADRC与LESO初始化/run_lesson.m) |
| 8 | [第8讲：线性二阶跟踪微分器（TD）——平滑目标与变化率](08_线性TD.md) | [run_lesson.m](../matlab/教学/第08讲_线性TD/run_lesson.m) |
| 9 | [第9讲：非线性二阶跟踪微分器（TD）——加速、制动与h0](09_非线性TD.md) | [run_lesson.m](../matlab/教学/第09讲_非线性TD/run_lesson.m) |
| 10 | [第10讲：一阶对象的二阶LESO——状态含义与采样时序](10_LESO状态与时序.md) | [run_lesson.m](../matlab/教学/第10讲_二阶LESO状态与时序/run_lesson.m) |
| 11 | [第11讲：二阶对象的三阶LESO——角度、角速度与总扰动](11_三阶LESO.md) | [run_lesson.m](../matlab/教学/第11讲_三阶LESO/run_lesson.m) |
| 12 | [第12讲：LESO带宽与测量噪声——快慢和抖动的取舍](12_LESO带宽与噪声.md) | [run_lesson.m](../matlab/教学/第12讲_LESO带宽与噪声/run_lesson.m) |
| 13 | [第13讲：二阶LADRC闭环——位置反馈、速度反馈与扰动补偿](13_二阶LADRC闭环.md) | [run_lesson.m](../matlab/教学/第13讲_二阶LADRC闭环/run_lesson.m) |
| 14 | [第14讲：TD接入二阶LADRC——平滑目标、控制峰值与到位时间](14_TD接入二阶LADRC.md) | [run_lesson.m](../matlab/教学/第14讲_TD接入二阶LADRC/run_lesson.m) |
| 15 | [第15讲：二阶LADRC限幅与实际输入反馈](15_限幅与实际输入反馈.md) | [run_lesson.m](../matlab/教学/第15讲_限幅与实际输入反馈/run_lesson.m) |
| 16 | [第16讲：三阶LESO的最小C++实现与MATLAB逐拍对齐](16_三阶LESO的C++实现.md) | [run_lesson.m](../matlab/教学/第16讲_三阶LESO的C++对齐/run_lesson.m) |
| 17 | [第17讲：二阶LADRC的最小C++组合与闭环核对](17_二阶LADRC的C++组合.md) | [run_lesson.m](../matlab/教学/第17讲_二阶LADRC的C++组合/run_lesson.m) |
| 18 | [第18讲：线性TD的最小C++实现与逐拍核对](18_线性TD的C++实现.md) | [run_lesson.m](../matlab/教学/第18讲_线性TD的C++实现/run_lesson.m) |
| 19 | [第19讲：TD、二阶LADRC与实际输入反馈的完整C++示例](19_完整C++接入示例.md) | [run_lesson.m](../matlab/教学/第19讲_完整C++接入示例/run_lesson.m) |
| 20 | [第20讲：一阶对象的降阶LESO](20_一阶对象的降阶LESO.md) | [run_lesson.m](../matlab/教学/第20讲_一阶对象的降阶LESO/run_lesson.m) |
| 21 | [第21讲：降阶LESO接入一阶LADRC](21_降阶LESO接入一阶LADRC.md) | [run_lesson.m](../matlab/教学/第21讲_降阶LESO接入一阶LADRC/run_lesson.m) |
| 22 | [第22讲：降阶LESO的最小C++实现](22_降阶LESO的C++实现.md) | [run_lesson.m](../matlab/教学/第22讲_降阶LESO的C++实现/run_lesson.m) |
| 23 | [第23讲：C++降阶一阶LADRC闭环接入](23_C++降阶一阶LADRC闭环接入.md) | [run_lesson.m](../matlab/教学/第23讲_C++降阶一阶LADRC闭环接入/run_lesson.m) |
| 24 | [第24讲：非线性二阶TD的最小C++实现](24_非线性TD的C++实现.md) | [run_lesson.m](../matlab/教学/第24讲_非线性TD的C++实现/run_lesson.m) |
| 25 | [第25讲：fal与非线性ESO入门](25_fal与非线性ESO入门.md) | [run_lesson.m](../matlab/教学/第25讲_fal与非线性ESO入门/run_lesson.m) |
| 26 | [第26讲：完整三阶非线性ESO与LESO对照](26_完整非线性ESO与LESO对照.md) | [run_lesson.m](../matlab/教学/第26讲_完整非线性ESO与LESO对照/run_lesson.m) |
| 27 | [第27讲：三阶非线性ESO的C++实现](27_非线性ESO的C++实现.md) | [run_lesson.m](../matlab/教学/第27讲_非线性ESO的C++实现/run_lesson.m) |
| 28 | [第28讲：非线性误差反馈与完整MATLAB闭环](28_非线性误差反馈与闭环.md) | [run_lesson.m](../matlab/教学/第28讲_非线性误差反馈与闭环/run_lesson.m) |
| 29 | [第29讲：完整非线性ADRC的C++接入](29_完整非线性ADRC的C++接入.md) | [run_lesson.m](../matlab/教学/第29讲_完整非线性ADRC的C++接入/run_lesson.m) |

第1~6讲新增简明MATLAB实验；第7~29讲保留并迁移原课程入口。
前几讲的基础实验不依赖C++；第16~19、22~24讲需要g++，第27、29讲需要g++、CMake、Ninja。
各讲使用ADRC1参考的部分仍需要本机原文件；示例参数始终与实车参数分开。

## 应用与验证

独立方案与指标定义见[库接入与应用说明](库接入与应用说明.md)。
原第30讲移入[验证目录](../matlab/验证/第30讲_库使用与最终验证/run_lesson.m)，保留原四讲公式核对；它不再作为默认学习入口。
新[自动验证入口](../matlab/验证/run_checks.m)还覆盖新增组件与四套应用。
原模型入口位于[原例对照](../matlab/原例对照/补充原例_二阶LADRC与三阶LESO/run_lesson.m)。

## 来源与学习边界

ADRC1继续作为原公式和实现参考；Adavance_PID补充应用仿真思路。已知实现问题与参数换算见[参考核查说明](参考核查与来源说明.md)。
课程脚本和库接口只说明如何计算；模型假设、参数选取、实机周期和电机性能需要单独确认。
