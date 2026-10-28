# 第24讲：非线性二阶TD的最小C++实现

前置：[第9讲：非线性TD与h0](09_非线性TD.md)、[第18讲：线性TD的C++实现](18_线性TD的C++实现.md)。

**教学归属：非线性TD工程实现。** 本讲将原 `TD2_NL.m` 的两状态更新和分段fhan整理成公共类 `adrc::TD2`。
独立观察TD，不接ESO、控制对象或反馈控制器；加入非线性TD也不意味着已经实现完整非线性ADRC。

## 1. 原代码和本讲文件对应哪里

原文件为：

`D:/ACE/ADRC/ADRC1/贺庆毕业论文ADRC全集包仅用于内部共享不要外传/MyLibrary/TD2_NL.m`

它的更新分支为：

```matlab
sys(1) = x(1) + h*x(2);
sys(2) = x(2) + h*fhan(x(1)-u,x(2),r,h0);
```

外层h是实际离散步长，传进局部fhan形参h的却是h0。
本讲保留两个状态均使用旧值，以及两个严格大于号分支：`abs(y)>d0`、`abs(a)>d`。
符号函数在零处返回0，没有把零误差当成正误差。

| 本讲文件 | 作用 |
| --- | --- |
| `include/adrc/adrc.hpp` | 公共接口、单位与时间约定 |
| `src/adrc.cpp` | Init、Reset、原fhan计算、前向欧拉更新 |
| `tests/td2_test.cpp` | 分支手算、解析阶段、边界与失败保持测试 |
| `examples/24_td_alignment/td_runner.cpp` | 主机逐拍输入输出，不加入固件控制环 |
| `matlab/第24讲_非线性TD的C++实现/run_lesson.m` | 自动编译、原函数对照与中文图 |

核心不依赖HAL、RTOS、动态内存或系统时钟。每个实例自己保存x1/x2，调用方串行访问。

## 2. 先分清四个量，别把Calc当成平滑输出

输入单位记为U：

| 量 | 含义 | 单位 |
| --- | --- | --- |
| x1 | 平滑后的输入值，GetOutput读取 | U |
| x2 | 平滑值的变化率状态，GetRate读取 | U/s |
| fh | 本拍内部加速度，Calc计算 | U/s² |
| r | fh的幅值尺度，不是x2的速度上限 | U/s² |

这与线性TD中的同名r不同：线性TD的r是频率参数，不能把两者数值当作同一种调参值。
如果输入是速度，x2的单位就是加速度，fh则是加速度的变化率；名称要结合输入单位理解。
fh只是TD内部信号动态，不是电机电流或执行器命令。

## 3. 最少接口怎样用

| 接口 | 含义 |
| --- | --- |
| `Init(r,h0,output0,rate0)` | 设置固定算法参数与两个初态 |
| `Reset(output0,rate0)` | 只重设状态，保留已有参数 |
| `GetOutput()`、`GetRate()` | 读取当前两个状态，不推进时间 |
| `Calc(input,acceleration)` | 用当前状态计算本拍fh；const，不推进状态 |
| `Update(input,dt)` | 用旧状态和本拍fh推进一次，成功后getter属于下一拍 |

例如：

```cpp
adrc::TD2 td;
bool ready = td.Init(20.0f, 0.01f, initial_output, initial_rate);
```

调用方检查ready后，每个区间保留本拍与下一拍的区分：

```cpp
const float current_target = td.GetOutput();
// 本拍控制器可以使用current_target。

float acceleration = 0.0f;
bool calculated = td.Calc(raw_target, acceleration); // 可选：观察本拍内部fh。
if (calculated)
{
    bool updated = td.Update(raw_target, dt);
    // 调用方检查updated；新的GetOutput/GetRate供下一拍使用。
}
```

不需要观察fh时，可以直接Update并检查返回值。
Calc不是取平滑输出，也不会代替Update；Update内部重新计算fh，但只积分一次。
不要在同一区间因为读了Calc又多调用一次Update。
实际接管初态应由消费工程提供，不能对每次目标变化都清零TD。

## 4. 原fhan的分支如何读

令error=x1-input，rate=x2，局部时间尺度为h0：

```text
d  = r*h0
d0 = h0*d
y  = error+h0*rate
a0 = sqrt(d*d+8*r*abs(y))

若abs(y)>d0：a=rate+(a0-d)*sign(y)/2
否则：       a=rate+y/h0

若abs(a)>d：fh=-r*sign(a)
否则：      fh=-r*a/d
```

这里提前看了当前变化率，所以还没到目标也可能开始制动。
原基准r=20、h0=0.01下，x1=0.95、x2=2、目标1时，fh=-20。
若实际dt=0.01，下一拍x1=0.97、x2=1.8：仍向目标接近，但变化率已减小。

程序先算出next_x1与next_x2，再一起提交。
不能先覆盖x1，再把新x1拿去计算本拍fh，那会改变原离散算法。
本讲没有换成其他常见的符号函数组合版fhan，也没有额外裁剪x2。

## 5. 为什么检查的是dt/h0，不是r*dt

h0固定在Init中，dt是每次Update实际传入的区间长度，两者都是秒。
r*dt具有变化率单位，不能照搬线性TD的无量纲频率步长条件。

在原函数两个内侧分支同时有效时：

$$
a=x_2+\frac{e+h_0x_2}{h_0}=\frac e{h_0}+2x_2
$$

$$
f_h=-\frac a{h_0}=-\frac e{h_0^2}-\frac{2x_2}{h_0}
$$

对固定目标，局部前向欧拉矩阵为：

$$
\begin{bmatrix}e[k+1]\\x_2[k+1]\end{bmatrix}
=\begin{bmatrix}1&dt\\-dt/h_0^2&1-2dt/h_0\end{bmatrix}
\begin{bmatrix}e[k]\\x_2[k]\end{bmatrix}
$$

两个特征值均为 `1-dt/h0`，因此局部渐近稳定要求 `0<dt/h0<2`。
本库用这个条件拒绝无效周期；h0、2h0分别对应局部极点0、-1，边界2h0拒绝执行。
**这只是局部内侧分支条件，不是任意幅值、切换、目标变化或周期抖动下的全局收敛证明。**

这是原代码之外明确增加的工程输入约束，不改变有效配置下的原分支和更新公式。
本讲比较的三个h0下，实际dt/h0分别为1、0.5、0.25。
改变h0时实际周期保持不变，不能用h0替代真实dt。

## 6. 数值与失败约定

Init要求r、h0为有限正数，初值有限，并要求d、d0、d²、8r等尺度可表示。
对会让d²下溢为0的极小配置也返回false。
因此本库接收域比原始未校验代码更严格，极端测试不是推荐工作参数。

Calc拒绝无效输入、偏差/预测量/根号内量/a或最终fh的非有限结果。
Update还检查周期及两个候选新状态。
保持原乘除次序意味着某些极端输入即使最终数学表达式可能有界，也会因中间量不可表示而拒绝；本讲没有重排公式掩盖这个差异。

- Init失败保留原配置与状态。
- Reset、Update失败保留两个状态；Reset本身只校验并赋状态，不能代替初始化。
- Calc失败保留调用方的输出引用，成功不改变状态。
- 原数学结构给出fh的幅值界，float下允许舍入量级的差异；没有额外加入输出钳位改变原结果。

对齐构建关闭浮点乘加收缩，未使用fast-math。消费工程若改动浮点优化选项，应重新核对数值与有限值检查。

## 7. MATLAB参数从哪里来

与第9讲一样，直接读取原库 `MyLibrary/ADRC.mdl` 的TD2_NL块：

| 项目 | 来源与设置 |
| --- | --- |
| r、标称h、基准h0 | 原库默认20、0.01秒、0.01秒 |
| 两个附加h0 | 明确增加的教学比较：0.02、0.04秒 |
| 恒值输入 | 原独立模型Constant块继承默认值1 |
| 正弦频率、观察时长 | 原独立模型记录pi/2 rad/s、20秒 |
| 原独立模型h0覆盖值 | 保存为0；本讲记录它，但采用原库正值h0基准 |

原零h0覆盖配置在零误差、零变化率处会出现0/0，脚本单独核对并记录该行为，不修改源文件。
这不等于本讲复现了那个独立模型的全部运行配置。

实际r、dt、h0和输入传给C++前转换为float。
time按实际float周期累计，正弦输入也按该时间生成，所以不把标称double周期与实际float周期混用。
六组各2001次更新，共12006次；末行after只是下一拍预测，不增加一个已采样状态。

## 8. 验证看哪些层次

主机测试先核对零点、正负远区、连续小误差、等号边界、提前制动与旧状态手算。
饱和加速阶段使用独立解析式：

$$
x_2[n]=nrh,\qquad x_1[n]=\frac{rh^2n(n-1)}2
$$

随后检查恒值跟踪、提前制动和变化率可超过r的示例，以及参数/数值失败保持、复位与独立实例。
这些是指定工况测试，不作为全局稳定性结论。

MATLAB入口再做两层核对：

- 相同float输入下，C++的更新前后两状态与fh，对照 `td_step` 的single更新。
- 用原 `TD2_NL.m` 自己推进double状态，比较完整轨迹。

原函数只输出两个状态，其本拍fh由第二状态增量除以实际dt反算，含微小的反算舍入。
它没有读取C++状态来修正自己的轨迹。
同时核对加速度界、单步变化率增量、恒值收敛和时间标签。

2026-10-28，使用MATLAB R2025b和GCC 16.1.0，从临时目录、默认搜索路径分别运行课程入口和根目录start.m，主机测试与仿真均通过。
六组的single差值均为0；相对原double函数，x1、x2、fh最大差值分别约1.33e-7、3.47e-6、5.39e-4。
对原函数差值不是输入跟踪误差：目标平滑会引入暂态和滞后，single对齐也不意味着消除了它们。
平滑x1也不等于fh始终连续，分段加速、制动和切换仍可在图中观察。

MATLAB搜索路径、环境PATH与全局rng均保持原状，中文图形已检查。
核心另使用STM32 GNU 13.3.1完成Cortex-M4/M7硬浮点对象编译，警告作为错误处理，均通过。
记录位于 `build/lesson24/entry_verification.json` 和 `build/lesson24/arm_object_verification.json`；后者含目标选项、对象文件与当前源文件哈希。
MATLAB入口只做主机编译，不依赖ARM工具链；本步没有固件链接、板上时序或电机验证。

## 9. 一键运行和输出

打开根目录 `start.m`，选择第24讲，按F5运行整个文件。
也可直接打开 `matlab/第24讲_非线性TD的C++实现/run_lesson.m`。
需要MATLAB、g++与本机ADRC1文件，不需要MEX或启动Simulink。
入口自动编译相关核心、主机测试和CSV驱动；编译器探测与PATH恢复沿用第22讲。

结果在 `build/lesson24/`：

- 六组 `*_input.csv`、`*_cpp.csv`，含本拍状态、内部加速度与下一拍状态。
- `alignment_metrics.csv`、`td_comparison.csv`：single与原double函数的分层对照。
- `alignment_result.mat`：变量result，保存教学归属、参数来源、编译器、测试输出与数据。
- `td_cpp_alignment.fig/.png/.svg`：中文教学图。

固件只需要头文件搜索目录include和 `src/adrc.cpp`，主机I/O与测试不带入控制任务。
对象编译、主机核对与板上执行是不同证据层次，不能由前两者推断实机已经验证。
下一步见[第25讲：fal与非线性ESO入门](25_fal与非线性ESO入门.md)，从原误差纠偏公式理解它和LESO的区别。
