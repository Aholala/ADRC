# 第29讲完整非线性ADRC主机例程

使用原TD2_NL/fhan、ESO2_NL、NLSEF_2fal对应的三个独立核心，不增加强绑控制器类。
控制顺序见control_example.cpp的编号1～5；第5部分为主机对象，真实负载只驱动对象。
CMake开启ADRC_BUILD_EXAMPLES后生成adrc_example，参数为配置、输入CSV、输出CSV。
配置顺序为7个ESO字段、反馈的beta1/alpha1/beta2/alpha2/delta、TD的r/h0、最终限幅。
限幅值0仅在主机例程里表示关闭限幅，不是生产接口的策略约定。
MATLAB第29讲自动生成文件、运行C++自主对象与两层参考；第30讲包含这一验收。
消费工程使用include/adrc中的组件和src中的实现，提供自己的目标、测量、时间、输出处理与失败策略。
