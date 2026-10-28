# 第27讲主机逐拍驱动

CMake开启ADRC_BUILD_EXAMPLES后生成eso_runner。
参数为：配置文件、输入CSV、输出CSV。MATLAB第27讲会自动创建文件并执行，无需手动配置。
配置顺序为b0、三个beta、两个alpha、delta、三个初始状态。
输入仅含时间、实际作用量、测量和周期；输出记录更新前后状态。
只有fal.cpp与eso3.cpp属于此例使用的生产核心，CSV和main不加入固件。
