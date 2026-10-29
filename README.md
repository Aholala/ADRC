# ADRC电机控制基础库

把摩擦、外力和模型误差等合并对输出产生的影响称为总扰动。ADRC是估计这些影响，再在控制计算中补偿的控制方法。

本仓库分别管理三件事：**可复用C++库、个人学习、应用仿真与验证**。材料完成不代表已经学完，仿真通过不代表实机通过。

## 运行库：两个文件

- [ALG_ADRC.hpp](main/ALG_ADRC.hpp)：公共接口，按基础函数、TD、ESO、反馈和控制器分区。
- [ALG_ADRC.cpp](main/ALG_ADRC.cpp)：全部实现。

将 `main/` 加入头文件搜索路径，编译 `main/ALG_ADRC.cpp`。命名空间为 `ADRC`，使用C++11，无HAL、RTOS、时钟读取或动态内存依赖。

```cpp
#include "ALG_ADRC.hpp"
ADRC::LADRC1 speed_controller;
ADRC::LADRC2 angle_controller;
```

| 类或函数 | 含义 |
| --- | --- |
| LTD2、TD2 | 跟踪输入并提供变化率，分别使用线性、非线性规律 |
| LESO2、LESO3 | 一阶、二阶对象的线性状态与总扰动估计 |
| ESO2、ESO3 | 一阶、二阶对象的非线性状态与总扰动估计 |
| RLESO1 | 一阶对象的降阶线性观测器 |
| LADRC1、LADRC2 | 内含对应LESO的线性控制器；LADRC2支持目标位置与目标速度 |
| NLESF1、NLESF2 | 一项、两项误差的非线性反馈与显式扰动补偿 |
| Fal | 静态分段幂函数，用于误差变换 |

TD和观测器数字表示内部动态状态数；LADRC数字表示对应对象阶次；NLESF1和NLESF2数字表示误差项数。
NLESF2表示非线性误差状态反馈，是原SEF2的当前名称。
参数、目标、测量、时间、单位、方向和最终输出限制由使用项目提供。
详见[库接入与应用说明](docs/库接入与应用说明.md)。

全部有参数的组件支持SetParameters在线改参；LTD2/TD2均可限制速度与加速度，五类独立观测器及LADRC1/2均可限制扰动估计。LESO2/3和LADRC1/2支持带宽与手动增益两种配置。
默认关闭新增限制，保留原调用方式。接口规则与一致性表见[在线改参与状态限制](docs/在线改参与状态限制.md)。
运行时只保留基本数值检查，不再逐次判定理论稳定区间；参数与周期的匹配由调用方离线验证。

## MATLAB：先选择用途

打开根目录 [start.m](start.m)，F5运行。默认 `mode='教学'`、`number=1`，从第1讲开始。
目录和说明使用中文，运行文件使用本机MATLAB支持的英文名称。

| 用途 | 启动设置或入口 | 内容 |
| --- | --- | --- |
| 教学 | mode='教学'，number=1~29 | 每次理解一个概念，按定义、例子、必要公式和代码推进 |
| 应用 | mode='应用'，number=1~4 | 单电机转速、舵向角度、视觉云台跟随、带功率限制的四轮控制 |
| 验证 | mode='验证'，或 [run_checks.m](matlab/验证/run_checks.m) | 自动检查课程入口、C++计算与应用结果 |
| 原例 | mode='原例'，或run_all | 原ADRC1 Simulink模型的工作副本，不修改参考仓库 |

也可以直接打开中文目录中的 `run_lesson.m` 或 `run_case.m`运行整个文件。
`matlab/内部函数/`需要参数，由入口调用，不作为独立仿真入口。
第30讲原验收保留在“验证”目录；它是程序检查，不作为个人学习完成记录。

运行结果位于忽略目录 `build/`：教学为lessonXX，应用为applications/caseXX，最终验证为verification。
应用输出包含中文FIG/PNG/SVG、参数、指标、输入和真正C++产生的轨迹；全部参数标为仿真假设。

## 依赖和检查

基础教学与公式使用MATLAB；涉及C++的课程、四类应用和验证还需g++、CMake、Ninja。
原例额外使用Simulink。应用不要求MEX，当前不要求PMS_Generator的电路模型工具箱。
原参考路径为用户指定的 `D:/ACE/ADRC/ADRC1`；Adavance_PID仅提供补充思路。

```powershell
& .\tests\verify_host.ps1
& .\tests\verify_arm.ps1
```

主机检查运行现有14项测试；ARM脚本分别生成M4/M7静态库并记录当前源文件哈希。
CMake提供 `adrc`目标，固件使用时关闭 `BUILD_TESTING`和 `ADRC_BUILD_EXAMPLES`。
主机CSV、对象模型、PID对照与功率示例只在examples，不加入生产库。
尚未完成实际固件链接、板上周期、电机或整车验证。

## 文档入口

- [学习路线与MATLAB索引](docs/学习路线与MATLAB索引.md)
- [MATLAB运行说明](docs/MATLAB运行说明.md)
- [库接入与应用说明](docs/库接入与应用说明.md)
- [ACE_ECF27电机库接入指南](docs/ACE_ECF27电机库接入指南.md)
- [参考核查与来源说明](docs/参考核查与来源说明.md)
- [改造计划与执行记录](docs/后续改造计划与记录.md)
- [原第30讲说明与历史验收](docs/30_第一版库使用与最终验证.md)
