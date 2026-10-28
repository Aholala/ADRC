# 第22讲：降阶LESO的C++逐拍驱动

`reduced_leso_runner.cpp` 读取MATLAB生成的输入和测量，输出真实C++核心的当前q、当前扰动估计以及下一拍q。
这是开环同输入回放，不包含控制器、对象模型或限幅策略。

运行根目录 `start.m`，选择第22讲；它自动编译核心、主机测试和本驱动。
核心只需要 `include/adrc/reduced_linear_eso.hpp` 与 `src/reduced_linear_eso.cpp`。
CSV读写、命令行解析和输出日志属于主机工具，不加入固件控制环。

时序为 `Calc(measurement,estimate)` → 消费本拍估计 → `Update(applied_input,measurement,dt)`。
下一拍用新的测量再次Calc；q_after不能直接充当下一拍扰动估计。
完整说明见 `docs/22_降阶LESO的C++实现.md`。
