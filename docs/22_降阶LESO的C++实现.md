# 第22讲：降阶LESO的最小C++实现

前置：[第20讲：降阶LESO推导](20_一阶对象的降阶LESO.md)、[第21讲：降阶LESO的一阶LADRC闭环](21_降阶LESO接入一阶LADRC.md)。

**教学归属：可嵌入式C++实现。** 本讲只把已经学过的独立降阶观测器加入公共库，
暂不增加控制器组合类、TD或限幅策略。类名为 `adrc::RLESO1`。

## 1. 和参考仓库的关系

优先参考仍为 `D:/ACE/ADRC/ADRC1`。
其中 `MyLibrary/ADRC_1.m` 的第3、4状态是前面核对过的全阶LESO；
`MyLibrary/ESO_1rd.m` 是空模板，不能声称本讲复制了其中的降阶实现。
本讲实际移植的是第20、21讲已经推导、独立核对的公式：

$$
\hat f[k]=q[k]+\omega_o y_{meas}[k]
$$

$$
q[k+1]=q[k]-h\omega_o(\hat f[k]+b_0u_{act}[k])
$$

对应MATLAB公式文件为 `matlab/内部函数/leso_reduced_step.m`，本讲保持该文件不变。
对象仍是一阶速度对象，不是二阶角度对象；唯一动态状态q也不是扰动估计本身。

## 2. 核心文件和最少接口

| 文件 | 用途 |
| --- | --- |
| `include/adrc/adrc.hpp` | 公共接口与时间标签约定 |
| `src/adrc.cpp` | 初始化、重构、前向欧拉更新 |
| `tests/rleso1_test.cpp` | 主机解析响应、接口与数值边界核对 |
| `examples/22_reduced_leso_alignment/reduced_leso_runner.cpp` | CSV同输入回放工具，属于主机例程 |

| 接口 | 含义 |
| --- | --- |
| `Init(b0,omega_o,measurement0,disturbance0)` | 设置参数，并用初始测量对齐q |
| `Reset(measurement0,disturbance0)` | 保留配置，只重设q；必须已经初始化 |
| `Calc(measurement,disturbance)` | 用当前测量重构扰动估计；const，不推进q |
| `Update(applied_input,measurement,dt)` | 用本区间实际输入推进一次q |
| `GetAuxiliaryState()` | 读取q用于诊断，不能把它当作扰动估计 |

内部只保存b0、ωo和q，不缓存上一拍测量或扰动估计。
多个实例互不共享状态；同一实例由调用方串行访问。
核心不读取时钟，不包含HAL、RTOS、动态内存或主机文件操作。

## 3. 为什么Calc必须带当前测量

全阶观测器的z2本身就是一个动态状态，可以直接读取。
降阶扰动估计却同时依赖q与当前测量：只读q不够。
所以这里没有无参数的 `GetDisturbance()`，避免把上一拍缓存值误当作当前估计。

```cpp
adrc::RLESO1 observer;
bool initialized = observer.Init(b0, omega_o, first_measurement, initial_f_hat);
```

消费工程检查initialized后，每拍执行成功路径：

```cpp
float f_hat = 0.0f;
bool reconstructed = observer.Calc(measurement, f_hat);
if (reconstructed)
{
    // 当前估计供本拍控制使用：
    // requested = (omega_c*(target-measurement)-f_hat)/b0;
    // 调用方完成最终输出处理，得到并提交applied_input。
    bool updated = observer.Update(applied_input, measurement, dt);
    // 调用方处理updated及各失败分支。
}
```

这段展示接口顺序，控制计算与最终输入来自消费工程，不是观测器内部功能。
Update内部重新计算本拍估计，但只推进一次q，没有第二个积分状态。
更新后拿同一个旧measurement再Calc，得到的是“新q加旧测量”，不能标成下一拍完整扰动估计。
应等下一拍采到新测量再Calc。

如果后级还有功率限制、执行器动态或不同的输入单位，需要按模型核对最终作用量；
不能把未限幅请求冒充观测器所需的实际输入。

## 4. 初值、返回值和数值约定

初始化与复位均使用：

$$
q_0=\hat f_0-\omega_o y_{meas,0}
$$

本讲初始速度为3 rad/s、ωo为20、期望初始扰动估计为0，干净组q0=-60。
初始扰动估计是调用方选择，不是自动读取真实扰动。
Reset需要已有ωo才能执行状态变换，因此初始化前Reset会返回false。

- b0必须有限非零，允许负增益；ωo必须有限且为正。
- 测量、输入、初值和周期必须有效，要求 `0<omega_o*dt<2`。
- Init失败保留旧配置和q；Reset、Update失败保留q。
- Calc失败保留调用方传入的输出引用；成功不改变q。
- 初始q或初始重构结果不可表示、当前重构溢出、状态更新溢出时返回false。

该步长范围只描述理想离散误差极点，不代表任意闭环或周期抖动下的性能保证。
本实现不需要预先存ωo²，因此不会因为某个从未使用的平方增益无法表示而拒绝配置；
每次实际运算仍要通过有限值与步长检查。极端数值测试不是推荐工作参数。

float重构包含两个较大量的相加抵消。
有限精度下，即使初态按公式设置，第一次重构也不保证与期望扰动逐位相等；
输出量程、带宽与精度需求应一起核对。
本讲对齐构建关闭浮点乘加收缩，未使用fast-math；消费工程改变浮点优化选项后需要重新验证结果和有限值检查。

## 5. 主机测试具体核对什么

主机测试包含：

1. 未初始化的Calc、Update、Reset拒绝执行，Calc失败保持输出引用。
2. 非零初态、测量直通增益、const重构不推进q以及手算更新。
3. 二进制精确周期与对象增量下，与独立离散阶跃解析解比较。
4. 正负输入增益的适用约定、负增益负载平衡、按测量复位与独立实例。
5. NaN、Inf、零/负周期、欧拉边界、乘积下溢、初值/重构/更新溢出。
6. 有限q但初始重构仍会溢出的极端舍入情况，以及失败后配置、q保持。

其中解析响应不是把实现公式重新抄一遍当期望值。
它使用 `hat_f[n]=F*(1-(1-omega_o*h)^n)`，独立核对当前采样时刻的估计。

## 6. MATLAB怎样核对真实C++

打开根目录 `start.m`，选择22，按F5运行整个文件。
也可打开 `matlab/第22讲_降阶LESO的C++实现/run_lesson.m`。
它自动编译核心、运行主机测试，再运行CSV回放程序，需要g++，不需要MEX或Simulink。
编译器探测与临时PATH恢复沿用第16讲，可通过ADRC_CXX指定完整路径。

教学参数沿用第20讲：b0=10、ωo=20、标称h=0.001秒、初始速度3，
0.5秒附近增加0.3 A实际输入，1.5秒附近施加-2 rad/s²负载。
两组分别无附加噪声、加标准差0.001 rad/s的固定种子噪声。
这些仍是教学假设，原参考文件保持只读。

这里明确区分三层核对：

| 层次 | 数据与目的 |
| --- | --- |
| C++对MATLAB single | 完全相同的float输入、测量、周期与初值，比较q_before、当前hat_f、q_after |
| C++对MATLAB double | 使用同一份float输入转换后的double，观察状态积分与重构的精度差 |
| 独立离散关系 | 消去q后的递推包含测量噪声和float测量量化，核对double参考的数学关系 |

对象在MATLAB以double推进，区间内输入与扰动恒定，因此对象更新精确。
CSV中time按传给C++的float周期累积，阶跃按固定采样编号发生；
标称0.001与它的float表示存在微小差异，不混用两套实际时间。
测量送入C++前转为float，所以clean表示没有额外随机噪声，仍有测量量化。
消去q的独立递推将“实际传入测量减真实输出”作为等效测量误差，避免把量化当作算法错误。

CSV回放只将实际输入、测量、周期传入核心，不将真实扰动或真实输出当作额外算法输入。
它属于开环观测器核对，没有声称C++在此运行了第21讲的完整控制闭环。

## 7. 本机验证结果

2026-10-28，使用MATLAB R2025b、GCC 16.1.0运行主机测试与回放。
从临时目录、默认搜索路径运行课程入口和根目录start.m均通过；路径、环境PATH与全局rng已核对保持原状，中文图已检查。

| 指标 | 无附加噪声组 | 带噪组 |
| --- | --- | --- |
| 更新次数 | 3001 | 3001 |
| 对single的q_before、当前hat_f、q_after最大差值 | 均为0 | 均为0 |
| 对double的q最大差值 | 1.984e-4 rad/s² | 6.803e-5 rad/s² |
| 对double的扰动估计最大差值 | 2.003e-4 rad/s² | 7.558e-5 rad/s² |
| double独立递推残差 | 1.887e-14 | 1.865e-14 |

数值差值不是总物理估计误差；后者还包含观测暂态、噪声和测量量化。
干净组也会积累float状态舍入误差，不能因single对齐差值为0就宣称等同于double精度。
本次扰动估计对double的最大差值约0.00020 rad/s²，量程或带宽改变后需要重新评估。

最终核心另使用STM32 GNU 13.3.1完成Cortex-M4、Cortex-M7硬浮点对象编译，均启用警告检查并作为错误处理。
记录分别位于 `build/lesson22/entry_verification.json` 和 `build/lesson22/arm_object_verification.json`。
ARM记录包含目标参数、对象文件和当前源文件哈希；MATLAB入口只执行主机编译，不依赖ARM工具链。
以上没有覆盖固件链接、板上执行或实际电机。

## 8. 输出与后续接入

结果在 `build/lesson22/`：

- 主机测试和逐拍驱动可执行程序。
- `clean/noisy_input.csv` 与 `clean/noisy_cpp.csv`：float输入与真实C++更新前后状态。
- `reduced_leso_comparison.csv`、`alignment_metrics.csv`：double参考及分层差值。
- `alignment_result.mat`：变量result，含教学归属、参数、编译器、主机测试输出与数据。
- `reduced_leso_cpp_alignment.fig/.png/.svg`：中文教学图。

移入消费工程时，只需增加头文件搜索目录include并编译 `src/adrc.cpp`。
不要把测试程序、CSV解析或主机日志代码带进固件控制环。
核心对象编译通过只能说明相关目标编译器接受它，仍需要消费工程链接、板上时序和实际对象验证。

下一步见[第23讲：C++降阶一阶LADRC闭环接入](23_C++降阶一阶LADRC闭环接入.md)，复核请求、限幅与实际输入反馈，再与第21讲的MATLAB模型对齐。
