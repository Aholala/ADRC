# 第16讲：三阶LESO的最小C++实现与MATLAB逐拍对齐

前置：[第15讲：限幅与实际输入反馈](15_限幅与实际输入反馈.md)。

**教学归属：工程实现。** 本讲只将已经学过的三阶LESO整理成C++模块，
并验证状态更新和接口行为；二阶LADRC的C++组合留到下一步。

## 1. 先确认这次实现的内容

对象模型仍为 `ddot(y)=b0*u+f`：

| 状态 | 含义 | 本例单位 | C++读取函数 |
| --- | --- | --- | --- |
| z1 | 输出角度估计 | rad | `GetOutput()` |
| z2 | 输出角速度估计 | rad/s | `GetRate()` |
| z3 | 总扰动估计 | rad/s² | `GetDisturbance()` |

新增的模块为 `ADRC::LESO3`：

- `main/ALG_ADRC.hpp`：接口与状态。
- `main/ALG_ADRC.cpp`：初始化、复位和一拍更新。

已有 `LESO2` 仍服务于一阶对象，没有被覆盖或改成三阶。
公共模块不依赖HAL、RTOS、动态内存、系统时钟或本机文件读写。
测试程序中的文件流与日志只用于主机验证，不需要加进嵌入式工程。

## 2. 公式怎样落到代码

带宽参数化沿用原 `Motion_acc03.mdl` 的三阶LESO：

$$
\beta_1=3\omega_o,\quad \beta_2=3\omega_o^2,\quad \beta_3=\omega_o^3
$$

`Update()` 内的核心为：

```cpp
const float error = z1_ - measurement;
const float next_z1 = z1_ + dt * (z2_ - beta1_ * error);
const float next_z2 = z2_ + dt * (z3_ + b0_ * input - beta2_ * error);
const float next_z3 = z3_ - dt * beta3_ * error;
```

三个右侧都使用当前拍旧状态。确认三个新值都有限后，再统一保存。
不要先更新z1，再用新z1计算另一个状态的误差。

本模块使用float，采用C++11；没有加入限幅、控制目标、模式或电机默认配置。
输入增益b0、带宽、初始状态和实际周期都由调用方决定。

## 3. 接口与时间标签

以下是调用形式，变量由使用者的工程提供：

```cpp
#include "ALG_ADRC.hpp"

ADRC::LESO3 observer;
bool configured = observer.Init(b0, omega_o,
                                initial_output, initial_rate, initial_disturbance);

// 每个区间只推进一次：使用u[k]与y_meas[k]，得到z[k+1]。
bool updated = observer.Update(applied_input_k, measurement_k, dt);
if (updated)
{
    float predicted_output_next = observer.GetOutput();
    float predicted_rate_next = observer.GetRate();
    float predicted_disturbance_next = observer.GetDisturbance();
}
```

`Init(b0,omega_o)` 的三个初始估计默认均为0；是否适合实际接管场景由调用方决定。
`Reset(output,rate,disturbance)` 只改状态，不更改增益，也不能替代初始化。
读取函数不会推进观测器：Update成功前是z[k]，成功后是z[k+1]。

输入必须对应所建模型、单位和该预测区间。第15讲说明的输出限幅或后级输出处理，仍由调用方完成。
同一实例由调用方串行访问；模块不增加锁或任务调度。

## 4. 无效调用怎样处理

以下情况返回false，并保留原状态；失败的Init同时保留原配置：

- 尚未初始化就Update。
- b0为0或非有限值；带宽非正或非有限。
- 增益计算溢出或下溢为0，尤其要检查新增的三次方增益。
- 初始状态、输入、测量或周期不是有限数值。
- dt非正或不是有限数值。
- 更新运算产生非有限结果。

负的非零b0允许使用，以适应调用方的坐标与输入方向约定。
失败后的业务动作由使用者决定，模块不自动改周期、清零或选择其他控制模式；扰动估计只按调用方显式设置的上限限制，默认不限制。
这些检查依赖正常IEEE浮点语义，不应加上 `-ffast-math` 后仍假定有限性检查有效。
固定周期的理想误差系统稳定范围，不代表任意采样抖动或整个实际闭环的稳定保证。
当前库不在Update中检查理论稳定区间，调用方应在运行前验证参数与周期。

## 5. 为什么这次要同时比较single和double

MATLAB通常用double，C++模块使用float；不能要求二者逐位相同。
本讲先把输入、测量、周期和初始化参数量化为float，再交给以下三种计算：

| 计算 | 运算精度 | 用途 |
| --- | --- | --- |
| C++ LESO3 | float | 实际被测代码 |
| MATLAB原公式 | single | 检查相同精度与运算顺序 |
| MATLAB原公式 | double | 对同一份已量化输入进行高精度重算，观察舍入差异 |

double参考不是另一份未经量化的输入。两种参考都接收C++实际读取的同一数值。
CSV按9位有效数字保存float，保证本机回读恢复相同float值；时间列用17位有效数字作为采样标签。

对齐程序同时保存Update前、后的三个状态，并核对“下一行before”等于“上一行after”。
最后一行after是最后一次预测，不当成最后一行当前状态。

## 6. 输入场景与本次结果

原参数读取自用户指定的ADRC1：

`D:/ACE/ADRC/ADRC1/LADRC实现参考/MATLABDemo-LADRC 实现(高志强老师)/LADRC Demo/tuning.m`

沿用h=0.001秒、b0=40、wo=40 rad/s；新增教学轨迹沿用第11讲：
0.5秒输入变为0.075 A，1.5秒总扰动变为-2 rad/s²，观察3秒。
另按第12讲的教学方式添加标准差0.001 rad、种子20261028的离散测量噪声。
两组都从零估计状态开始，原参考文件保持只读。

两组各3001拍。在本机GCC构建选项下，更新前后的C++状态与MATLAB single逐拍完全一致。
相对同输入double参考，两个场景合计的各状态最大差值约为：

| 状态 | 最大差值 | 单位 |
| --- | --- | --- |
| z1 | 1.12e-6 | rad |
| z2 | 7.14e-5 | rad/s |
| z3 | 7.66e-4 | rad/s² |

这些是数值实现对照差异，不是状态相对真实对象的估计误差上限。
换编译器、运算融合选项、输入量级或带宽后，要重新验证，不能据此承诺所有平台逐位相同。

## 7. 主机测试还检查了什么

`tests/leso3_test.cpp` 包含：

- 匹配欧拉对象下三重极点误差系统的解析响应，包括已知输入变化。
- 非零状态的一步手算，检查三个新状态使用旧值。
- 复位、非零扰动平衡和不同实例独立性。
- 无效初始化、无效输入、周期边界、增益溢出与下溢、更新溢出。
- 失败后状态与配置保持，以及负输入增益方向。

解析测试使用与观测器匹配的欧拉对象，以便独立核对离散解析解。
MATLAB对齐场景使用前几讲的对象轨迹；两种测试的对象离散约定已分开，不混作同一实验。

## 8. MATLAB一键验证怎么运行

根目录 `start.m` 选择教学模式并设置本讲number，按F5运行。
也可打开 `matlab/教学/第16讲_三阶LESO的C++对齐/run_lesson.m`。

本讲新增g++依赖，不需要配置MEX。入口会自动编译、运行C++主机测试和两组数据对齐，再导出中文图。
编译器优先使用环境变量ADRC_CXX指定的完整路径，其次搜索PATH，最后探测本机已安装的MSYS2位置。
例如本机可以显式设置：

```matlab
setenv('ADRC_CXX','D:/msys64/ucrt64/bin/g++.exe')
run('D:/ACE/Infantry_dev/ADRC/matlab/教学/第16讲_三阶LESO的C++对齐/run_lesson.m')
```

编译所需PATH前缀只在函数作用域内临时设置，返回或报错时恢复。
主机验证使用C++11、严格告警、O2、关闭浮点运算融合，并静态链接主机运行库。
静态链接选项属于主机工具，不是给STM32工程增加主机库。

上排图比较含噪声场景的真值、C++与double参考；下排图显示C++与double参考之差。
本机构建的C++与single差值为0，所以不另画三条全零误差线。

输出位于 `build/lesson16/`：

- `leso3_test.exe`、`leso3_runner.exe`：主机测试和逐拍数据驱动程序。
- `clean_input.csv`、`noisy_input.csv`：同一份已量化输入。
- `clean_cpp.csv`、`noisy_cpp.csv`：C++各拍更新前后状态。
- `leso3_comparison.csv`、`alignment_metrics.csv`：当前状态对照与更新前后最大差值。
- `alignment_result.mat`：参数、编译器、教学归属、数据和测试输出。
- `leso3_cpp_alignment.fig/.png/.svg`：中文教学图。

## 9. 嵌入式使用与证据边界

消费工程只需添加 `main/ALG_ADRC.cpp`，并将 `main/` 加入头文件搜索路径。
不需要加入MATLAB、测试程序或CSV驱动程序。仍需由工程提供初始化参数、周期、输入和测量。

本次另完成Cortex-M4与Cortex-M7浮点选项下的核心对象编译，关闭异常与RTTI。
MATLAB入口自动重跑主机验证，不自动重跑ARM编译。
对象编译通过不等于固件链接、板上时序或实际电机验证，当前未声称完成这些环节。

本机MATLAB R2025b已从临时目录、默认搜索路径分别运行课程入口和根目录 `start.m`，均通过。
根目录入口重编译并运行了当前主机测试，完成两组共6002次更新；单精度对照最大差值为0。
还检查了PATH恢复、全局rng保持和中文图形，记录在 `build/lesson16/entry_verification.json`。

继续：[第17讲：二阶LADRC的C++组合](17_二阶LADRC的C++组合.md)。
