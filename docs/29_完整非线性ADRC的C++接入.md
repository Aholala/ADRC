# 第29讲：完整非线性ADRC的C++接入

前置：[第28讲](28_非线性误差反馈与闭环.md)。教学归属：**完整组件组合和原模块对照**。

## 1. 库中哪些组件参与

- TD2：原TD2_NL/fhan，输入原始目标，输出平滑值与变化率。
- ESO3：原ESO2_NL，使用输出测量与实际输入估计三状态。
- NLESF2：原NLSEF_2fal两误差反馈，再按本讲单位约定补偿z3并除b0。

不新增把三者强绑在一起的公共类。调用方决定是否使用TD、目标速度、最终输出处理和初始化。
完整可编译示例在 `examples/29_adrc/control_example.cpp`，读取配置与输入CSV只是主机工具。

## 2. 生产控制任务保留的顺序

```cpp
// 三个实例已由消费工程配置和初始化，跨周期保存。
float requested = 0.0f;
bool calculated = feedback.Calc(td.GetOutput(), td.GetRate(),
    observer.GetOutput(), observer.GetRate(), observer.GetDisturbance(), requested);
if (calculated)
{
    // 调用方完成最终输出处理并提交，得到applied。
    bool observed = observer.Update(applied, measurement, dt);
    if (observed)
    {
        bool tracked = td.Update(raw_target, dt);
        // 调用方处理tracked和各失败分支。
    }
}
```

读到的是当前状态，新状态供下一拍使用。状态模块每区间只推进一次。
真实负载和真实速度没有直接送入控制器；C++只使用自己的对象输出形成测量。
观测器收到的applied和对象实际使用的输入是同一个变量。
若后级还有功率限制、执行器动态或输入单位变化，必须继续核对模型作用量。

每个bool接口失败保留对应模块状态/输出引用，但不承诺整条“提交输出→更新多个模块”自动回滚。
例程遇错退出属于主机行为，实机失败策略由消费工程负责。

## 3. 数值和验证方法

C++自己维护double精确ZOH对象、float测量与算法状态，输入文件没有预先提供测量轨迹。
三个工况为干净无限幅、干净有限幅、带噪有限幅，沿用第28讲场景。
配置、周期和限幅按实际float表示记录，时间按实际周期累计。

核对分两层：

1. 使用C++当前测量做MATLAB single回放，检查TD/ESO更新前后、请求与限幅。
2. 原TD2_NL、ESO2_NL、NLSEF_2fal直接组成独立double闭环，不读取C++测量、对象或状态。

三组共24003次自主C++更新通过。
本机single回放最大请求差约7.15e-7 A，状态最大差约2.98e-8；幂函数库与运算舍入存在微小差异，**不是逐位完全相同**。
相对独立原模块double闭环，最大输出差约8.25e-6 rad，实际输入差约1.50e-5 A。
所有差值均保存在CSV，测试容差只针对本教学配置，不是任意参数下的误差保证。

有限幅干净组请求峰值约4.42 A，实际边界为float的0.3 A，累计限幅约1.254秒。
末段平均目标误差约-1.55e-6 rad；带噪组约-3.07e-5 rad。浮点稳态与噪声残差保留在结果中。
目标下调时仍有明显超调，图中保留这一执行约束下的暂态；当前参数仅用于公式和接入验证，不作为实机性能承诺。
最后一行next字段只是预测，不额外计入采样区间或限幅时间。

## 4. 运行与证据

入口 `matlab/教学/第29讲_完整非线性ADRC的C++接入/run_lesson.m`。
需要MATLAB、g++、CMake、Ninja、ADRC1，不需MEX/Simulink。
它构建整库与当前主机测试（现为14项），运行三条自主闭环，再生成两层对照和中文图。
结果在 `build/lesson29/`：三组配置/输入/C++日志、integration_metrics.csv、integration_result.mat及完整闭环图。

这验证了主机组合和理想对象，未替代固件链接、板上采样时序或电机实验。
下一讲给出第一版库的最终接口索引、接入清单和统一验证入口。
