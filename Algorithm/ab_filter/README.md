# AB 滤波器

AB 滤波用于对一维信号进行轻量级预测和平滑。它维护两个状态：

- `value`：当前滤波值，例如角度、位置、高度、电流。
- `rate`：当前变化率，例如角速度、速度、上升速度、电流变化率。

本库提供两个版本：

- `ABFilter_c`：标准 AB 滤波。
- `ABRateFusionFilter_c`：支持在预测阶段融合外部输入变化率。

---

## 1. 最快调用

### 1.1 标准 AB 滤波

```cpp
#include "ab_filter.hpp"

alg_n::ABFilter_c pos_filter(0.6f, 0.2f);

void Task()
{
    float dt = 0.001f;
    float measure_pos = GetPosMeasure();

    pos_filter.Predict(dt);
    float pos = pos_filter.Correct(measure_pos);
    float vel = pos_filter.GetRate();
}
```

### 1.2 融合外部变化率

```cpp
#include "ab_filter.hpp"

alg_n::ABRateFusionFilter_c angle_filter(0.6f, 0.2f, 0.8f);

void Task()
{
    float dt = 0.001f;
    float measure_angle = GetAngleMeasure();
    float gyro_rate = GetGyroRate();

    angle_filter.Predict(gyro_rate, dt);
    float angle = angle_filter.Correct(measure_angle);
    float rate = angle_filter.GetRate();
}
```

---

## 2. 什么时候用

适合：

- 需要比一阶低通更少滞后的平滑场景。
- 测量值有噪声，但信号大体连续变化。
- 需要同时得到平滑值和变化率估计。
- 有外部速度/角速度可用，希望减小预测滞后。

不适合：

- 多维强耦合系统。
- 噪声统计特性复杂、需要协方差建模的场景。
- 测量值存在大量跳变且没有额外异常值处理的场景。

这些复杂场景建议使用卡尔曼滤波或专门的状态估计算法。

---

## 3. 参数怎么调

### 3.1 `alpha`

`alpha` 控制滤波值对测量值的跟随强度：

- 越大：越相信测量值，响应更快，噪声更多。
- 越小：更平滑，滞后更明显。

常用起点：`0.4f ~ 0.8f`。

### 3.2 `beta`

`beta` 控制变化率对误差的修正强度：

- 越大：变化率估计跟随更快，但容易抖。
- 越小：变化率更平滑，但动态响应慢。

常用起点：`0.05f ~ 0.3f`。

### 3.3 `rate_weight`

`ABRateFusionFilter_c` 独有，控制外部变化率在预测中的权重：

- `0.0f`：只使用滤波器内部估计的变化率。
- `1.0f`：只使用外部输入变化率预测。
- `0.5f ~ 0.9f`：常见融合范围。

如果陀螺仪、编码器速度等变化率信号较可信，可以取大一些；如果变化率信号噪声大或偶发异常，可以取小一些。

---

## 4. Predict / Correct 流程

AB 滤波分两步执行。

### 4.1 Predict

标准版本：

```text
value = value + rate * dt
```

融合外部变化率版本：

```text
rate  = rate_weight * input_rate + (1 - rate_weight) * rate
value = value + rate * dt
```

这一步只根据模型和变化率向前预测，不使用新的测量值。

### 4.2 Correct

```text
residual = measure - value
value    = value + alpha * residual
rate     = rate + beta * residual / dt
```

这一步只根据测量值修正预测结果。

---

## 5. 接口说明

### 5.1 `ABFilter_c`

```cpp
ABFilter_c(float alpha = 0.6f, float beta = 0.2f);
void Init(float alpha, float beta, float value = 0.0f, float rate = 0.0f);
void SetGain(float alpha, float beta);
void Reset(float value = 0.0f, float rate = 0.0f);
float Predict(float dt);
float Correct(float measure);
float GetValue() const;
float GetRate() const;
float GetResidual() const;
```

### 5.2 `ABRateFusionFilter_c`

```cpp
ABRateFusionFilter_c(float alpha = 0.6f, float beta = 0.2f, float rate_weight = 0.8f);
void Init(float alpha, float beta, float rate_weight, float value = 0.0f, float rate = 0.0f);
void SetGain(float alpha, float beta, float rate_weight);
void Reset(float value = 0.0f, float rate = 0.0f);
float Predict(float input_rate, float dt);
float Correct(float measure);
float GetValue() const;
float GetRate() const;
float GetResidual() const;
```

---

## 6. 使用注意

- 每次 `Correct()` 前建议先调用一次 `Predict()`。
- `dt` 必须大于 0，单位要和变化率单位匹配。
- 本库会把 `alpha`、`beta`、`rate_weight` 自动限制在 `0~1`。
- 当传感器首次上电或重定位时，建议调用 `Reset(value, rate)` 设置初值。
- 如果测量值明显异常，应由上层先做限幅、丢包判断或异常值剔除。
