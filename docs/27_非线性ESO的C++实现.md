# 第27讲：三阶非线性ESO的C++实现

前置：[第26讲](26_完整非线性ESO与LESO对照.md)。教学归属：**独立观测器工程实现**。

## 1. 拿哪些文件

共享分段幂函数在 `include/adrc/fal.hpp` 和 `src/fal.cpp`。
三阶非线性ESO在 `include/adrc/nleso3.hpp` 和 `src/nleso3.cpp`。
核心只保存配置和三个状态，没有HAL、RTOS、动态内存或系统时钟依赖。

```cpp
adrc::NLESO3Config config = {
    40.0f, 60.0f, 120.0f, 252.98221f, 0.5f, 0.25f, 0.01f
}; // 仅第26讲教学配置，不是机器人默认参数。
adrc::NLESO3 observer;
bool ready = observer.Init(config, initial_output, initial_rate, initial_disturbance);
```

配置字段顺序是b0、beta1、beta2、beta3、alpha1、alpha2、delta。
beta是实际原式系数，不要把未经换算的LESO带宽值直接填进来。

## 2. 调用和失败约定

当前getter供本拍控制使用；调用方完成最终输出处理后，执行：

```cpp
bool updated = observer.Update(applied_input, measurement, dt);
```

成功后getter是下一拍预测。每个区间只推进一次，三个新状态一起提交。
Init失败保留配置与状态；Reset和Update失败保持状态。
Reset只设状态，不能代替Init。消费工程检查所有bool返回值。

有效域为有限非零b0、正beta、0<alpha1/alpha2<=1、正delta和有限初态，局部有效增益须可表示。
Update要求有限实际输入、测量和正周期，并检查中间量及新状态是否溢出。
对于任意给定非线性系数，库没有假设一个通用稳定步长；第26讲用局部矩阵核对当前配置，实际消费工程也需验证周期与参数。
共享Fal支持alpha>1，但ESO接口单独限定指数范围，和后面的反馈用途区分。

## 3. 原实现对应和数值核对

C++严格保留原ESO2_NL的线性首行、后两行fal、负纠偏方向和旧状态欧拉更新。
主机测试含独立手算、失败保持、alpha=1退化为LESO、负b0平衡与独立实例。

MATLAB两组各6001次更新：

- 同一float输入、测量、初值和周期，对比MATLAB single更新前后状态。
- 原ESO2_NL用自己的double状态计算导数，再作相同欧拉离散化。

在本次GCC16.1/R2025b构建中，single状态最大差值均为0。
相对原double更新，z1/z2/z3最大差值约5.09e-6、1.90e-4、1.01e-3。
后者是精度差，不是对象总估计误差；也不保证不同平台pow实现总能逐位相同。

## 4. 运行

第27讲入口自动用CMake构建静态库、运行11项主机测试，再运行C++观测器CSV驱动。
需要MATLAB、g++、CMake、Ninja与ADRC1，不需要MEX/Simulink。
入口为 `matlab/第27讲_非线性ESO的C++实现/run_lesson.m`；输出在 `build/lesson27/`，MAT变量为result。
图、CSV、指标和编译测试输出均保存，原参考文件不变。
这是主机验证，固件链接和板上时序留给消费工程。
