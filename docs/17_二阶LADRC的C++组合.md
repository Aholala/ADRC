# 第17讲：二阶LADRC的最小C++组合与闭环核对

前置：[第16讲：三阶LESO的C++实现](16_三阶LESO的C++实现.md)。

**教学归属：工程实现。** 本讲新增 `adrc::LADRC2`，内部组合已有 `LESO3`，
将第13讲控制公式落实为可单独调用的C++控制计算和观测更新。

## 1. 类内部保存什么

- b0、kp、kd是控制计算使用的参数，`kp=wc²、kd=2wc`。
- 一个LESO3保存三个估计状态，不再额外复制一份z1、z2、z3。
- 不保存电机类型、模式、目标轨迹、输入上限或任务时钟。

本讲组件在合并库中的位置：

- `include/adrc/adrc.hpp`：接口与组合关系。
- `src/adrc.cpp`：参数初始化、静态控制律与观测更新转发。

已有LADRC1仍对应一阶对象，未被替换。

## 2. Calc只做控制计算

控制公式与MATLAB `adrc_control2.m` 一致：

$$
u_{req}=\frac{k_p(r-z_1)-k_dz_2-z_3}{b_0}
$$

本例是角度给定与零速度基准反馈，没有目标速度或加速度前馈。
可以将TD的v1作为位置给定，但目标过渡中的动态跟踪误差仍需按第14讲理解。

```cpp
bool Calc(float target, float& command) const;
```

`const`表明Calc不会修改控制器状态。反复对同一状态和目标调用，结果相同，观测器不会被推进。
未初始化、目标非有限或计算溢出时返回false，并保留调用方原来的command。
Calc输出的是请求值，没有在核心里偷偷限制电流。

## 3. UpdateObserver单独推进一次

```cpp
bool UpdateObserver(float applied_input, float measurement, float dt);
```

它调用内部LESO3，用当前拍测量和本区间实际输入，将z[k]推进为z[k+1]。
同一区间只更新一次，沿用第16讲的输入、周期与失败保持约定。

对已正确初始化的实例，一拍的调用形式如下，参数由消费工程提供：

```cpp
float requested = 0.0f;
bool calculated = controller.Calc(target, requested);

if (calculated)
{
    // 调用方做最终输出处理；此处仅以限幅为例。
    float applied = std::min(std::max(requested, input_min), input_max);
    bool updated = controller.UpdateObserver(applied, measurement, dt);
    // 调用方继续检查updated，处理失败分支。
}
```

以上展示接口顺序，实际工程需要检查每个bool返回值；失败时的业务处理由调用方决定。
限幅和发送给执行器的过程属于消费工程，模块没有加入这些业务动作。
标准C++使用 `std::min/std::max` 时包含 `<algorithm>`；实际作用量还要符合所建模型和时间区间。

核心顺序是：**读当前估计计算请求 → 调用方输出处理 → 实际输入更新LESO**。
没有先把LESO推进到下一拍，再混作当前拍状态使用。

## 4. 初始化、复位与读取

```cpp
adrc::LADRC2 controller;
bool configured = controller.Init(b0, omega_c, omega_o,
                                  initial_output, initial_rate, initial_disturbance);
```

后面三个初始估计可省略，默认0；实际接管是否适合零初值，由调用方判断。
Init先检查反馈增益，再初始化临时观测器；全部成功后才替换现有配置与状态。
失败不会把一个正在运行的控制器改成半初始化状态。

`Reset(output,rate,disturbance)`只复位估计状态，保留参数，不代替Init。

| 读取函数 | 含义 | 角度模型单位 |
| --- | --- | --- |
| `GetEstimatedOutput()` | z1，输出估计 | rad |
| `GetEstimatedRate()` | z2，输出变化率估计 | rad/s |
| `GetEstimatedDisturbance()` | z3，总扰动估计 | rad/s² |

读取不会推进状态；UpdateObserver成功后，读到的均是下一拍预测。
同一实例由调用方串行使用。

## 5. 为什么分开两个函数

第15讲已经说明，请求值不一定能被执行。
如果将“计算请求并更新观测器”封成一个没有实际输入回传位置的函数，
调用方限幅后就容易让观测器继续使用未执行的请求。

本接口给调用方留出输出处理的位置，但不替调用方决定电流限制、功率限制或异常策略。
本讲主机驱动的±0.2 A仅为教学条件，公共类内没有这个数值。
业务输入增益允许为负，反馈方向随b0处理；不会硬编码机器人坐标方向。

## 6. 主机接口测试

`tests/ladrc2_test.cpp` 检查：

- 位置、速度、扰动各项的符号和整体除以b0的单位约定。
- const Calc与重复Calc不改变状态；Calc失败不改变输出参数。
- 非零扰动下，零位置误差仍有维持输入。
- 匹配欧拉对象的二重极点阶跃解析解。
- 外部限幅后的可达负载闭环，以及调用方将实际输入覆盖为0的情况。
- 无效初始化、增益溢出/下溢、复位与观测更新失败后的保持行为。
- 负输入增益与独立实例。

Init检查正且可表示的控制增益，观测器沿用原周期检查。
它不要求wc与wo具有某个固定比例，也不根据这些局部检查自动保证实际对象闭环稳定。
本讲参数wc=8、wo=40只是已经核对的教学配置。

## 7. 这次C++自主运行整个闭环

主机驱动在 `examples/17_ladrc2_alignment/ladrc2_runner.cpp`。
MATLAB只提供目标、外部负载与周期；C++自行完成：

1. 将自身对象当前输出量化为float测量。
2. 用LADRC2计算请求。
3. 由驱动程序选择无限幅或±0.2 A外部限幅。
4. 用实际输入更新控制器内部LESO。
5. 用实际输入与负载推进自己的double精度对象。

因此不是由MATLAB提前算好控制输入，再让C++仅打印结果。
对象本身使用区间恒定输入的精确ZOH更新，观测器仍为前向欧拉。

参数沿用第13～15讲：h=0.001秒、b0=40、wo=40来自原tuning，教学wc=8；
0.5秒目标变为1 rad，2.5秒负载变为-2 rad/s²，运行6秒。
原wc=150单独记录，未冒充原完整模型复现。

参考仍为用户提供的ADRC1中LADRC Demo目录下的tuning.m和Motion_acc03.mdl，原文件保持只读。

## 8. 两层MATLAB核对分别证明什么

### 同输入的单精度核对

回放C++实际使用的测量和float参数，MATLAB single按同样顺序计算控制请求、调用方限幅和LESO更新。
核对请求、实际输入、更新前后状态及相邻拍关系。
这层检查控制与观测组合是否忠实实现了已学公式和时序。

### 独立double闭环核对

另一组MATLAB模型读取原控制增益表达式和原LESO矩阵，使用自己的对象状态和测量独立闭环。
没有回放C++测量，也没有用C++控制输入驱动它。
两条自主闭环的精度和测量量化不同，不要求逐位相同，比较位置轨迹和实际输入的差异。

周期、输入限幅等参数先转为float，再将同一数值交给参考模型，避免参数本身不一致。
无噪声、无TD、匹配输入增益，均是本讲明确的教学条件。

## 9. 本次结果

两组各6001拍，控制请求、实际输入和三个观测状态在相同输入下与MATLAB single逐拍完全一致。
两组自主闭环合计，相对独立double模型的位置最大差值约 `2.76e-6 rad`，
实际输入最大差值约 `2.66e-5 A`。

| 场景 | 实际输入绝对峰值/A | 5～6秒平均位置误差/rad | 5～6秒平均输入/A |
| --- | --- | --- | --- |
| 无限幅 | 1.6 | 约7.29e-7 | 约0.05 |
| 外部±0.2 A限幅 | 0.2 | 约7.81e-7 | 约0.05 |

float值本身也有表示误差，限幅断言按相同的float边界比较。
这些差值描述本次模型与构建结果，不是实机定位精度指标或任意参数的误差上限。
限幅系统的有限轨迹核对也不等于对所有饱和工况的稳定性证明。

## 10. MATLAB一键运行

根目录 `start.m` 默认选择 `lesson_id=17`，按F5运行整个文件。
也可打开 `matlab/第17讲_二阶LADRC的C++组合/run_lesson.m`。
需要MATLAB、g++和本机ADRC1参考文件，不需要MEX。
编译器探测、ADRC_CXX和PATH临时设置方式沿用第16讲。

入口自动编译并运行主机测试、两个自主C++闭环，再做MATLAB核对和中文绘图。
输出位于 `build/lesson17/`：

- `ladrc2_test.exe`、`ladrc2_runner.exe`：主机工具。
- `closed_loop_input.csv`：目标、外部负载和周期。
- `unlimited_cpp.csv`、`limited_cpp.csv`：C++对象、控制请求、实际输入及观测状态。
- `closed_loop_comparison.csv`、`alignment_metrics.csv`：独立闭环对照和分层误差指标。
- `alignment_result.mat`：教学归属、参数、编译器与主机测试输出。
- `ladrc2_cpp_alignment.fig/.png/.svg`：中文教学图。

CSV中before是本拍估计，after是UpdateObserver后的下一拍预测；最后一行after不是新的当前拍采样。

## 11. 加入嵌入式工程

消费工程添加 `src/adrc.cpp`，头文件搜索路径添加 `include/`。
不需要加入主机驱动、文件流、MATLAB或测试代码。核心使用C++11，未引入HAL、RTOS或动态内存。

目前两个核心组件统一位于src/adrc.cpp，整库Cortex-M4/M7对象编译见第30讲；
尚未完成实际固件链接、任务时序、板上数据或电机验证。
MATLAB入口重跑主机验证，不自动调用ARM工具链。

本机R2025b已从临时目录、默认搜索路径分别运行课程入口和根目录 `start.m`，均通过。
两组共12002次自主闭环更新，同输入回放的控制请求与更新前后状态和MATLAB single完全一致。
已检查PATH恢复和中文图形，记录在 `build/lesson17/entry_verification.json`。

继续：[第18讲：线性TD的C++实现](18_线性TD的C++实现.md)。
