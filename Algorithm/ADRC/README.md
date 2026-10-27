# TD：从线性跟踪微分器开始

这一版只实现独立的线性二阶 TD，不包含 ESO、扰动补偿、电机或任务调度。
仿真与嵌入式使用同一份 `td.cpp`，Python 只负责编译、运行和画图。

## 1. 输入和输出

- 输入 `input`：希望跟踪的标量信号。
- 输出 `x1 = GetValue()`：平滑后的信号。
- 输出 `x2 = GetRate()`：平滑信号的一阶变化率。

例如输入角度（rad），输出就是平滑角度（rad）和变化率（rad/s）。
输入速度时，`x2` 就是加速度。TD 不读取电机反馈，也不是 ESO。
两个状态称为二阶 TD，与被控对象或后续 ADRC 的阶次没有绑定关系。

## 2. 最基础的公式

连续模型：

```text
dx1/dt = x2
dx2/dt = -omega^2 * (x1 - input) - 2 * omega * x2
```

这是阻尼比为 1 的线性二阶系统。其连续传递函数为：

```text
X1(s) / Input(s) = omega^2 / (s + omega)^2
X2(s) / Input(s) = s * omega^2 / (s + omega)^2
```

`x2` 是 `x1` 的导数；对原始输入的求导带有滤波滞后，不是理想微分。
对于斜率为 `v` 的持续斜坡，稳态 `x2` 趋于 `v`，但输入与 `x1`
之间仍有 `2*v/omega` 的跟踪误差。正弦输入也会出现幅值衰减和相位滞后。

离散更新沿用 ADRC1 中 `MyLibrary/TD2_L.m:33-35` 的前向欧拉形式：

```text
acceleration = -omega^2 * (x1[k] - input[k]) - 2 * omega * x2[k]
x1[k+1] = x1[k] + dt * x2[k]
x2[k+1] = x2[k] + dt * acceleration
```

必须先用旧状态算完右侧，再更新状态。`Calc()` 返回后，getter 读取的是
`x[k+1]`。仿真 CSV 先记录 `x[k]` 与 `input[k]`，再调用 `Calc()`。

## 3. 参数与边界

- `omega`：接口中叫 `bandwidth`，频率参数，单位 rad/s，不是速度或加速度限幅。
  它是连续模型的自然频率，不等于该二阶低通的 -3 dB 截止频率。
- `dt`：实际执行周期，单位 s；调用方负责提供，不读取系统时钟。
- `omega` 越大，固定周期下通常跟随越快，同时输入噪声更容易进入输出。

前向欧拉离散极点为 `1 - omega*dt`，固定周期稳定条件为：

```text
0 < omega * dt < 2
```

这只是稳定条件，不保证良好的波形或足够精确地逼近连续模型。
`omega*dt` 接近 2 时可能有强烈交替响应，应通过减小步长检查结果是否收敛。
核心拒绝稳定范围外的周期，不自动替换周期、限幅状态或猜测机构参数。

未初始化、非有限输入、无效周期、无法表示的配置或运算溢出都返回 `false`。
失败保留原配置及状态；`Init()` 成功时重新设置参数与状态，`Reset()` 只改状态。
极小或极大的尺度仍可能导致浮点精度损失，应选择合适的单位。
依赖非有限值检查时不要开启破坏 IEEE 浮点语义的 `-ffast-math`。

## 4. 嵌入式调用

把 `td.cpp` 加入消费工程的源文件，并添加本目录为头文件搜索路径。
无需 HAL、RTOS、动态内存或电机模块；本次没有修改框架构建模板。

```cpp
#include "td.hpp"

alg_n::LinearTD_c target_td;

bool InitTargetTD(float omega, float initial_value, float initial_rate)
{
    return target_td.Init(omega, initial_value, initial_rate);
}

bool UpdateTargetTD(float target, float actual_dt,
                    float& smooth_target, float& target_rate)
{
    if (!target_td.Calc(target, actual_dt))
        return false; // 调用方决定异常时的业务行为。

    smooth_target = target_td.GetValue();
    target_rate = target_td.GetRate();
    return true;
}
```

初始值和复位值由使用项目决定。若希望接入时从当前状态过渡，可显式使用
`Reset(current_value, current_rate)`。每个实例保存独立状态，调用方保证同一实例
串行更新，每个控制周期只推进一次。圆周角度需在调用前选择连续角度表示，
本模块不猜测角度单位或跨圈策略。

## 5. 运行仿真

依赖：Python、matplotlib，以及 PATH 中的 `g++`（或 `--compiler` 指定的兼容编译器）。
在仓库根目录运行：

```powershell
python Algorithm/ADRC/examples/run_td_sim.py
```

脚本依次编译并运行数值检查、编译并运行仿真，再生成：

- `build/td-sim/td_sim.csv`：阶跃、反向阶跃、斜坡、正弦、含噪正弦数据。
- `build/td-sim/td_response.png`、`td_response.svg`：值和变化率曲线。
- `build/td-sim/td_test.exe`、`td_sim.exe`：Windows 主机可执行文件。

仿真使用 `omega=20 rad/s`、`dt=1 ms`，只是演示参数，不是机器人默认参数。
阶跃不画普通导数参考线，因为跳变时刻的导数不属于有限普通函数。
含噪正弦图中的导数参考是无噪声正弦的解析导数，不是噪声信号的导数。

数值检查包含离散阶跃解析响应、斜坡稳态变化率和滞后、复位、无效输入与
多实例独立性。主机结果与 ARM 对象编译不能代替完整固件链接、任务时序和实机验证。
