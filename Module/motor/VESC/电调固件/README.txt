南邮电调固件基于[VESC开源](https://github.com/vedderb/bldc)版本6.05的fw_60修改得到

```
hwconf/trampa/vesc6/hw_60_core.h

修改 CURRENT_AMP_GAIN 为 50.0
修改 CURRENT_SHUNT_RES 为 0.001
```



A版优化电调在南邮基础上将栅极驱动8301改为8323，采用栅驱内部电流采样，其固件也可以基于[VESC开源](https://github.com/vedderb/bldc)版本6.05的fw_60修改得到

```
hwconf/trampa/vesc6/hw_60_core.c

修改 #include "drv8301.h" 为 #include "drv8323s.h"
修改 drv8301_init(); 为 drv8323s_init();
```

```
hwconf/trampa/vesc6/hw_60_core.h

修改 #define HW_HAS_DRV8301 为 #define HW_HAS_DRV8323S
删除 #define HW_HAS_PHASE_SHUNTS
新增 #define DRV8323S_CUSTOM_SETTINGS()	drv8323s_set_current_amp_gain(CURRENT_AMP_GAIN); \
									drv8323s_write_reg(3, 0x3af); \
									drv8323s_write_reg(4, 0x7af)
定义 #define DCCAL_ON() 				drv8323s_dccal_on()
定义 #define DCCAL_OFF() 				drv8323s_dccal_off()
修改 CURRENT_AMP_GAIN 为 40
修改 CURRENT_SHUNT_RES 为 0.001

/* SPI定义中有关DRV8301_*修改为DRV8323_* */
```



测试使用的fw版本为6.05，通过上述修改理论能够兼容高版本