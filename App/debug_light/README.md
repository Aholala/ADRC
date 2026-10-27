## 前置需求

H7->TIM1_CH2 Psc: 0, Arr: 300 - 1

F4->TIM3_CH3 Psc: 0, Arr: 105 - 1

![image-20260201201814491](https://gitee.com/lzero123/typora_-img/raw/master/PicGo/image-20260201201814491.png)

### 开全局中断（高级定时器就开Update&break中断）

![image-20260201201910869](https://gitee.com/lzero123/typora_-img/raw/master/PicGo/image-20260201201910869.png)

### DMA方向改为Memory->Peripheral

![image-20260203035125429](https://gitee.com/lzero123/typora_-img/raw/master/PicGo/image-20260203035125429.png)

### 多任务配置

配成比你default大一点就行

![image-20260203035354052](https://gitee.com/lzero123/typora_-img/raw/master/PicGo/image-20260203035354052.png)

不创也行，自己调用Debug_Task()里面的内容，20ms

## 可用功能(#include "debug_light.hpp")

函数

```
Light_n::ShowStatic(StaticEffect_e type); // 显示预设灯效
Light_n::ShowBlink(StaticEffect_e type, uint8_t times); // 显示预设灯效，并使B灯闪烁times次
```

预设灯效

[飞书文档--ACE 通用机器人信号灯语](https://bnmp7m19gb.feishu.cn/docx/UQfHdlIm3o5wpuxJmoccRgQinWb) 满足v26.1.1.1全灯效

```
/* 正常灯效 */
Init = 0,
Normal = 1,
Debug = 2,
// 3、4 保留

/* CAN异常灯效 */
Can1_Lost  = 5,
Can1_Error = 6,
Can2_Lost  = 7,
Can2_Error = 8,
Can3_Lost  = 9,
Can3_Error = 10,
// 8、9、10 保留

/* 通讯异常灯效 */
VT13_Lost = 11,
MiniPC_Lost = 12,
PowCR_Lost = 13,
DR16_Lost = 14,
// 15、16 保留

/* 酷炫灯效 */
RGB = 17,
RGB_Blink = 18,
RGB_Breath = 19,
```

可以自创灯效触发类

```
float max = 100;
float min = 0;
float set_float = 10;
bool set_bool = true;
Light_n::Debug_c* rgb_light;
void SomethingInit(void)
{
    Light_n::Effect_t effect[2] = {Light_n::RGB_BREATH_EFFECT, Light_n::RGB_BLINK_EFFECT};
//  rgb_light = new Light_n::Debug_c(effect); // 手动触发灯效，调用类函数ManualTrigger触发
//  rgb_light = new Light_n::Debug_c(effect, &set_float, min, max); // 阈值触发灯效，超阈值触发
//  rgb_light = new Light_n::Debug_c(effect, &set_bool); // 布尔值触发灯效，当布尔值为false触发
}
```

可以自创灯效结构体

```
typedef struct Effect_t
{
    char* name = nullptr;		       // 名称,同名称灯效不能重复作用于同一个灯
    Priority_e priority = Priority_e::Idle; // 优先级，同一个灯上同优先级取后注册的
    float duration = 1;                 // 此灯效持续时间，持续时间过后优先级变为最低
    uint8_t mode = 0;                   // 效果模式 Breath | ColorCycle | SaturationCycle| Blink
    float breath_speed = 0;             // 呼吸速度(百分比/更新周期)
    float breath_min = 0;               // 亮度最小阈值(百分制)
    float breath_max = 100;             // 亮度最大阈值(百分制)
    float color_speed = 0;              // 色环循环速度(°/更新周期)
    float saturation_speed = 0;         // 饱和度循环速度(百分比/更新周期)
    uint8_t blink_times = 0;            // 闪烁次数
    Ws2812_n::HSV_t color = {0, 0, 1};  // HSV颜色
} Effect_t;
```

