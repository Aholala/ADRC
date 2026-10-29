# 共用算法的四类应用实验

application_runner.cpp是主机仿真例程，不属于生产库。四类场景共用main/ALG_ADRC.hpp/.cpp中的同一套算法。
场景ID选择输入、模型参数和对比方案，不根据电机型号复制控制算法。

MATLAB入口位于matlab/应用方案/01~04_主题/run_case.m，自动构建CMake的application_runner目标。
命令行为：application_runner case_id fixture.csv native.csv。
输入为time、四路target、四路rate、四路noise、四路load、dt，共18列；每个量的单位和模型见docs/库接入与应用说明.md。
输出记录本次计算前的TD/ESO状态、请求、最终命令、下一次估计和独立对象状态。

Calc读取当前估计，最终命令经过限制后才送UpdateObserver/Update；每个区间只推进一次。
场景03在2秒将实际测量和之前最终命令用于初始化LADRC2，以检查切换的第一条命令衔接。
场景04保留“错误地反馈请求”的对照，目的是展示错误接法，不能用于固件控制。

模型中的PID、两拍命令延迟、电流响应、重力与教学功率限制都属于本例。
参数不是实车推荐值；PI/PID没有复制原机器人增益，不声称实车算法性能排序。
