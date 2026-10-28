# ADRC

按“一阶对象 → ESO → 控制与补偿 → 工程边界 → TD/非线性扩展”的顺序学习。
`D:/ACE/ADRC/ADRC1` 是优先参考；需要的教学拆分或补充场景明确说明来源和假设。
仿真统一用 MATLAB/Simulink，学懂后再对齐可嵌入式运行的 C++ 实现。

**第一版ADRC基础库与第1～30讲已完成。** 接入与验收从 [第30讲：库使用与最终验证](docs/30_第一版库使用与最终验证.md) 开始。
全部课程分类、入口和后续 LADRC/LESO 专题见 [学习路线与 MATLAB 索引](docs/学习路线与MATLAB索引.md)。

**最直接的运行方法：在MATLAB编辑器打开根目录 `start.m`，点击“运行”（F5）。**
默认运行第30讲最终验收；修改顶部的 `lesson_id` 可选第7～29讲或补充原例（0）。
也可以打开对应中文课程文件夹里的 `run_lesson.m`，直接运行整个文件。
无需手动添加搜索路径；初次运行不要只运行中间某一节。

课程目录使用中文；`.m`入口统一叫 `run_lesson.m`。
本机MATLAB R2025b实测无法直接运行中文文件名的`.m`文件，因此不把它们改成无效入口。
`matlab/内部函数/` 保存需要参数的公式函数，用于阅读或由脚本调用，不能当独立仿真脚本运行。
详细操作见 [MATLAB运行说明](docs/MATLAB运行说明.md)。

- **ADRC**：自抗扰控制的总体方法。
- **LADRC**：线性自抗扰控制器，前几讲使用的一阶控制器属于这一类。
- **LESO**：线性扩张状态观测器，作为 LADRC 内部的观测部分。
- **TD**：跟踪微分器，用于信号过渡和变化率估计，可与控制器组合。

库中的类名统一使用算法缩写：

| 类名 | 含义 |
| --- | --- |
| `LTD2` / `TD2` | 线性 / 非线性二阶TD |
| `LESO2` / `LESO3` | 二阶 / 三阶线性ESO |
| `RLESO1` | 一个内部状态的降阶线性ESO |
| `ESO3` / `ESO3Config` | 三阶非线性ESO及其配置 |
| `LADRC1` / `LADRC2` | 一阶 / 二阶LADRC |
| `SEF2` | 两项非线性状态误差反馈 |

所有类位于 `adrc` 命名空间，头文件位于 `include/adrc/`。
非线性部分直接用 `TD`、`ESO`、`SEF` 命名；线性部分保留 `LTD`、`LESO`、`LADRC` 前缀。
LESO的数字表示观测器阶次；LADRC的数字表示控制器对应的对象阶次。
统一头文件仍为 `#include "adrc/adrc.hpp"`。

**运行库只需两个文件：** [adrc.hpp](include/adrc/adrc.hpp) 和 [adrc.cpp](src/adrc.cpp)。
将 `include/` 加入头文件搜索路径，编译 `src/adrc.cpp` 即可使用全部组件。
文件内部按“基础函数 → TD → ESO → 控制器与反馈”分区，各类仍可独立使用。
`docs/`、`matlab/`、`examples/` 与 `tests/` 分别保存讲义、仿真、接入示例与验证程序。

在 MATLAB 中将当前文件夹切到本仓库，运行最终验收：

```matlab
run('matlab/第30讲_库使用与最终验证/run_lesson.m')
```

第30讲自动运行第26～29讲，完成完整非线性ESO、反馈与C++组合核对，并构建静态库和运行11项主机测试。
需要MATLAB、g++、CMake、Ninja与本机ADRC1参考文件，无需MEX或Simulink。最终记录在 `build/lesson30/`。
生产接口集中在 `include/adrc/adrc.hpp`，全部实现在 `src/adrc.cpp`。
CMake提供adrc目标；单独验证C++可运行 `tests/verify_host.ps1`，M4/M7整库编译见 `tests/verify_arm.ps1`。
这些是主机/MATLAB与目标静态库验证，尚未消费固件ELF链接、板上时序或电机验证。

第25讲fal入门入口为 `run('matlab/第25讲_fal与非线性ESO入门/run_lesson.m')`，输出位于 `build/lesson25/`，无需g++。

第24讲C++非线性TD入口为 `run('matlab/第24讲_非线性TD的C++实现/run_lesson.m')`，输出位于 `build/lesson24/`，需要g++。
该讲保留原fhan与旧状态更新，恒值/正弦各比较三个h0。

第23讲降阶一阶LADRC入口为 `run('matlab/第23讲_C++降阶一阶LADRC闭环接入/run_lesson.m')`。
三条自主C++闭环核对同测量single与独立double，输出位于 `build/lesson23/`；接入示例在 `examples/23_reduced_ladrc_integration/control_example.cpp`。

第22讲独立C++降阶LESO入口为 `run('matlab/第22讲_降阶LESO的C++实现/run_lesson.m')`，输出位于 `build/lesson22/`，需要g++。

第21讲降阶LESO的一阶LADRC闭环入口为 `run('matlab/第21讲_降阶LESO接入一阶LADRC/run_lesson.m')`。
三种工况各比较全阶和降阶两条独立MATLAB闭环，输出位于 `build/lesson21/`，无需g++。

第20讲降阶LESO入门入口为 `run('matlab/第20讲_一阶对象的降阶LESO/run_lesson.m')`，输出位于 `build/lesson20/`。
该讲用固定同轨迹实验解释一个辅助状态的推导、测量依赖与噪声影响；原 `ESO_1rd.m` 是空模板。

第19讲完整C++接入入口为 `run('matlab/第19讲_完整C++接入示例/run_lesson.m')`，输出位于 `build/lesson19/`。
该讲需g++，自动编译三个核心测试与完整示例；调用顺序在 `examples/19_complete_integration/control_example.cpp` 的编号1～6中。

第18讲线性TD入口为 `run('matlab/第18讲_线性TD的C++实现/run_lesson.m')`，输出位于 `build/lesson18/`。

第17讲二阶LADRC入口为 `run('matlab/第17讲_二阶LADRC的C++组合/run_lesson.m')`，输出位于 `build/lesson17/`。

第16讲三阶LESO入口为 `run('matlab/第16讲_三阶LESO的C++对齐/run_lesson.m')`，输出位于 `build/lesson16/`。

第15讲限幅入口为 `run('matlab/第15讲_限幅与实际输入反馈/run_lesson.m')`，输出位于 `build/lesson15/`。

第14讲TD接入入口为 `run('matlab/第14讲_TD接入二阶LADRC/run_lesson.m')`，输出位于 `build/lesson14/`。

第13讲二阶LADRC入口为 `run('matlab/第13讲_二阶LADRC闭环/run_lesson.m')`，输出位于 `build/lesson13/`。

第12讲带宽与噪声入口为 `run('matlab/第12讲_LESO带宽与噪声/run_lesson.m')`，输出位于 `build/lesson12/`。

第11讲三阶LESO入口为 `run('matlab/第11讲_三阶LESO/run_lesson.m')`，输出位于 `build/lesson11/`。

第10讲二阶LESO入口为 `run('matlab/第10讲_二阶LESO状态与时序/run_lesson.m')`，输出位于 `build/lesson10/`。

第9讲非线性TD入口为 `run('matlab/第09讲_非线性TD/run_lesson.m')`，输出位于 `build/lesson09/`。

第8讲线性TD入口为 `run('matlab/第08讲_线性TD/run_lesson.m')`，输出位于 `build/lesson08/`。

第7讲是“一阶LADRC + 二阶LESO”的工程初始化应用。
在 MATLAB 中将当前文件夹切到本仓库，运行本节实验：

```matlab
run('matlab/第07讲_LADRC与LESO初始化/run_lesson.m')
```

输出位于 `build/lesson07/`，包含 `.mat/.csv/.fig/.png/.svg`。
本节从 `ADRC_1.m` 的状态初始化、ESO 和反馈结构出发，补充非零稳态的教学场景。
核心公式在 `matlab/内部函数/adrc_eso_step.m` 和 `matlab/内部函数/adrc_control.m` 中，可逐行查看。

[ADRC1 原例、参数来源与复现状态](docs/00_ADRC1原例与复现基线.md) 是补充参考。
如需运行已经复现的原始二阶模型，使用：

```matlab
data = run_all;
```

该补充原例入口依赖 MATLAB 和 Simulink，已在 R2025b 验证。无需 Python 或 C++ 编译器；第16～19、22～24讲另需g++，第27、29、30讲另需g++、CMake、Ninja。
脚本从原 `tuning.m` 读取参数，运行原 `Motion_acc03.mdl` 的工作副本，保留
原求解器、对象、轨迹、噪声与阶跃输入；只增加日志和结果导出。
源仓库保持只读，工作副本和 `.mat/.csv/.fig/.png/.svg` 输出位于 `build/reference_adrc1/`。

原例是二阶 LADRC、三阶 LESO，不是前面的一阶教学控制器。
原一阶 `ADRC_1RD_TEST.mdl` 的 `K` 依赖尚未确认，程序只审计、不代填或冒充已复现。
C++现已有线性/非线性TD、全阶/降阶LESO、三阶非线性ESO、LADRC与非线性反馈核心，并有完整线性/非线性主机接入例程。各自完成教学模型核对，尚未据此声称完整原例或硬件验证。

第 1～6 步中的教学对象、参数和实验数值与原例配置分开标注，知识路线继续有效。
早期独立实验入口已停用，后续各节提供 MATLAB 入口。

- [第 1 步：从电机速度理解一阶对象与总扰动](docs/01_一阶对象与总扰动.md)
- [第 2 步：二阶线性 ESO 怎样估计总扰动](docs/02_二阶线性ESO.md)
- [第 3 步：ESO 增益、带宽与采样周期](docs/03_ESO增益与采样周期.md)
- [第 4 步：最小离散 ESO 与开环仿真](docs/04_离散ESO与仿真.md)
- [第 5 步：状态反馈、扰动补偿与一阶 LADRC](docs/05_一阶LADRC闭环.md)
- [第 6 步：输出限幅与实际输入反馈](docs/06_输出限幅与输入反馈.md)
- [第 7 步：非零状态下的初始化与无扰接管](docs/07_初始化与无扰接管.md)
- [第8讲：线性二阶跟踪微分器（TD）](docs/08_线性TD.md)
- [第9讲：非线性二阶跟踪微分器（TD）](docs/09_非线性TD.md)
- [第10讲：一阶对象的二阶LESO——状态含义与采样时序](docs/10_LESO状态与时序.md)
- [第11讲：二阶对象的三阶LESO——角度、角速度与总扰动](docs/11_三阶LESO.md)
- [第12讲：LESO带宽与测量噪声](docs/12_LESO带宽与噪声.md)
- [第13讲：二阶LADRC闭环](docs/13_二阶LADRC闭环.md)
- [第14讲：TD接入二阶LADRC](docs/14_TD接入二阶LADRC.md)
- [第15讲：限幅与实际输入反馈](docs/15_限幅与实际输入反馈.md)
- [第16讲：三阶LESO的C++实现与MATLAB对齐](docs/16_三阶LESO的C++实现.md)
- [第17讲：二阶LADRC的C++组合](docs/17_二阶LADRC的C++组合.md)
- [第18讲：线性TD的C++实现](docs/18_线性TD的C++实现.md)
- [第19讲：完整C++接入示例](docs/19_完整C++接入示例.md)
- [第20讲：一阶对象的降阶LESO](docs/20_一阶对象的降阶LESO.md)
- [第21讲：降阶LESO接入一阶LADRC](docs/21_降阶LESO接入一阶LADRC.md)
- [第22讲：降阶LESO的C++实现](docs/22_降阶LESO的C++实现.md)
- [第23讲：C++降阶一阶LADRC闭环接入](docs/23_C++降阶一阶LADRC闭环接入.md)
- [第24讲：非线性TD的C++实现](docs/24_非线性TD的C++实现.md)
- [第25讲：fal与非线性ESO入门](docs/25_fal与非线性ESO入门.md)
- [第26讲：完整非线性ESO与LESO对照](docs/26_完整非线性ESO与LESO对照.md)
- [第27讲：非线性ESO的C++实现](docs/27_非线性ESO的C++实现.md)
- [第28讲：非线性误差反馈与闭环](docs/28_非线性误差反馈与闭环.md)
- [第29讲：完整非线性ADRC的C++接入](docs/29_完整非线性ADRC的C++接入.md)
- [第30讲：第一版库使用与最终验证](docs/30_第一版库使用与最终验证.md)

此前 C++ 核心、数值测试和实验源文件保留，尚未据此声称实际硬件或完整原例验证通过。
