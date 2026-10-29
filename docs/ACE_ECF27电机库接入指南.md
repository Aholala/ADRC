# ACE_ECF27电机库接入ADRC指南

本文说明如何让ADRC负责控制计算，让ECF27电机驱动继续负责反馈解析和命令发送。电机型号改变反馈换算与输出接口；同一个ADRC类可以由不同电机分别建立实例使用。

本文属于实际接入说明，与matlab/教学分开。示例是消费工程控制任务内的代码片段，电机对象、参数和任务必须由现有工程提供，不是独立固件。

核查日期：2026-10-30。参考目录为D:/ACE/ACE_ECF_27，分支Develop，提交6ccedeaecc59d2ea8f5d109f3f65f6fe8a232ebf，核查开始时工作区干净。本次只读参考工程，不修改电机驱动、固件配置或控制任务。

## 1. 先确定ADRC的输入和输出

这里的“输出”是控制器交给电机驱动的命令，例如力矩或电流；“反馈”是测得的转速、角度等。它们的单位决定b0的含义，不能只按电机名称选择参数。

| 控制任务与接口 | 起步组合 | 测量y | 命令u |
| --- | --- | --- | --- |
| 电流/力矩接口控制转速 | LADRC1，或ESO2+NLESF1 | rad/s | A或N·m，选定一种 |
| 力矩接口控制角度 | LADRC2，或LESO3+NLESF2 | rad | N·m |
| 使用非线性观测器的角度控制 | ESO3+NLESF2 | rad | N·m |
| 需要平滑目标及目标变化率 | 在上述控制器外增加LTD2或TD2 | 与目标同单位 | 提供平滑目标和变化率 |
| 只增加一阶扰动估计 | LESO2或RLESO1，反馈律由项目选择 | 例如rad/s | 与建模输入一致 |

若驱动器接收的是目标速度，ADRC所面对的已经包含驱动器内部速度控制，不能照搬“角度对应二阶对象”的参数与结构。本文重点给出力矩控制DJI/DM和电流控制VESC的接法。

对于y为rad/s、u为N·m的一阶模型，b0表示单位力矩对角加速度的标称作用；对于y为rad、u为N·m的二阶模型，含义相应一致。若u改为A，必须使用按电流输入确定的b0。不要把MATLAB的示例数值直接用于实机。

## 2. 当前电机接口对应表

| 当前驱动 | 可用反馈 | ADRC输出接到哪里 | 关键边界 |
| --- | --- | --- | --- |
| DJI的M3508、M3508NEW、M2006 | 原始RPM，或累计角度 | SetMotorOutputFix(torque_nm)，再走分组发送 | 参数是力矩，内部转为int16电流编码量；不是直接传A |
| DJI的GM6020 | 原始RPM、角度 | 需先在消费工程确认可用的电压/命令编码输出路径 | 当前SetMotorOutputFix对它直接置零，不能使用上面的力矩接法 |
| DM系列 | 原始rad、rad/s、N·m；加工后的角度为度 | SetMotorOutputFix(torque_nm) | 内部设置MIT的kp=kd=0并立即发送；调用方先按实际协议范围限幅 |
| VESC | data().erpm，data().current等 | Set_Current(current_A) | 使用独立类，不继承MotorBase_c；调用即发送 |
| Cybergear | 当前回调填写原始位置、速度、力矩 | 协议层有MiMotorControlMode(torque,0,0,0,0) | 当前类缺少基类纯虚函数实现，不能直接作为已接好的驱动使用，见第8节 |
| C615 | 当前类不提供转速反馈 | SetSpeed(percent)是PWM命令 | 需要外部速度反馈才能形成速度控制；不是“设定RPM”接口 |

MotorBase_c的SetMotorOutputFix默认实现只改状态和pid_output，不负责实现所有电机的实际发送。不要仅凭“继承同一基类”就把这些驱动当作完全相同的执行器接口。

源码入口：[电机基类](D:/ACE/ACE_ECF_27/Module/motor/motor_base_def.hpp:702)、[DJI固定输出](D:/ACE/ACE_ECF_27/Module/motor/DJI/dji_driver.cpp:660)、[DM固定输出](D:/ACE/ACE_ECF_27/Module/motor/DM/dm_driver.cpp:41)、[VESC接口](D:/ACE/ACE_ECF_27/Module/motor/VESC/vesc_driver.hpp:85)。

## 3. 将两个算法文件加入消费固件

运行库仍只有[ALG_ADRC.hpp](D:/ACE/Infantry_dev/ADRC/main/ALG_ADRC.hpp)和[ALG_ADRC.cpp](D:/ACE/Infantry_dev/ADRC/main/ALG_ADRC.cpp)。

例如，把它们复制到消费固件的Usercode/Algorithm/ADRC目录，在消费工程CMake的user custom区域加入：

```cmake
USER_ADD_DIR(${CMAKE_SOURCE_DIR}/Usercode/Algorithm/ADRC)
```

这是[当前模板](D:/ACE/ACE_ECF_27/template/CMakeLists.txt)实际提供的函数，会加入该目录的源码和头文件路径。若你的工程不用此函数，则显式target_sources加入cpp、target_include_directories加入目录。两种方式选一种，避免重复编译。

当前配置生成器提供USE_DJI_MOTOR_F4/H7、USE_DM_MOTOR_F4/H7、USE_VESC_MOTOR_F4/H7等选项，具体依赖以[rule.json](D:/ACE/ACE_ECF_27/ConfigFileGenerate/MainDate/rule.json:353)为准。当前模板没有名为USE_ADRC的现成开关，不在指南里虚构一个。

ADRC使用C++11；ECF27当前模板使用C++20，可以编译这个库。只加入两个核心文件，不把examples、tests、MATLAB或ADRC主机CMake工程整体加入固件。保留正常浮点语义，勿启用会使有限值检查失效的fast-math；如需保持现有单精度对照的运算约定，对ALG_ADRC.cpp使用-ffp-contract=off。

## 4. 统一反馈、方向和时间

### 4.1 反馈单位

以下换算常数和普通限幅函数放在消费工程cpp的函数外；它们是示例辅助代码，不是新增电机接口：

```cpp
constexpr float deg_to_rad = 0.01745329252f;
constexpr float rpm_to_rad_s = 0.10471975512f;
float Clamp(float value, float minimum, float maximum)
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}
```

| 数据路径 | 当前源码中的含义 | 示例换算 |
| --- | --- | --- |
| DJI motor_raw_data.feedback_speed | 电机轴RPM | raw_rpm*rpm_to_rad_s/ratio，得到输出轴rad/s |
| DJI motor_processed_data.total_angle | 已按ratio换算的累计角度，度 | total_angle*deg_to_rad；不再除ratio |
| DM motor_raw_data.position | 驱动协议直接解码的角度，rad | 使用该坐标或自行进行关节映射 |
| DM motor_raw_data.feedback_speed | 驱动协议速度，rad/s | 不因型号里保存了ratio就再次除一次 |
| DM motor_processed_data.total_angle | 按相对零位建立的连续角度，度 | total_angle*deg_to_rad |
| DM motor_processed_data.speed | 度/s | speed*deg_to_rad，或直接选原始rad/s |
| VESC data().erpm | 电转速，eRPM | erpm/pole_pairs*rpm_to_rad_s/gear_ratio |
| VESC data().current | 电流的0.1A编码值 | current*0.1f；不是已经以A存储 |

pole_pairs是电机极对数，即一圈机械转动包含多少圈电角度；gear_ratio是电机轴转速与所控输出轴转速之比。这两个量由实际电机和机械传动提供，不在ADRC里设置默认型号参数。

DJI当前processed speed的递推式在求和后整体除ratio；relative_angle及relative_angle_180路径也有再次除ratio的操作。因此本文的DJI速度示例选原始RPM按ratio换算一次，角度选total_angle，不把这几项加工值不加核对地交给ADRC。本文不修改这些驱动公式。若需要速度滤波，由消费工程在统一单位后使用已有滤波器。

累计角度表示连续跨圈，不等于已经完成机械零位标定；DJI的total_angle计算路径没有直接应用zero_offset。目标和测量必须采用同一零位。单圈角跨±180°或0/360°时，先在项目中展开或按所需路径生成连续目标，不能将跳变直接送入ESO或差分得到目标速度。

对应源码：[数据结构](D:/ACE/ACE_ECF_27/Module/motor/motor_base_def.hpp:516)、[DJI反馈解析](D:/ACE/ACE_ECF_27/Module/motor/DJI/dji_driver.cpp:729)、[DM反馈解析](D:/ACE/ACE_ECF_27/Module/motor/DM/dm_driver.cpp:138)。

### 4.2 方向和反馈来源

本指南用direction=+1或-1表示项目坐标与驱动坐标的关系：

```text
项目测量 = direction*驱动测量 + 位置零位偏置
驱动命令 = direction*项目命令
ESO输入  = direction*最终驱动命令
```

位置零位偏置只加在位置上，不加在速度上。若关节坐标是“标定角减电机角”，则方向为-1，偏置由你的机构标定提供。

原motor_reverse_flag、feedback_reverse_flag及前馈指针等由MotorPIDCalculate处理。绕过它以后，这些设置不会自动作用到ADRC的计算上；方向、重力补偿和其他前馈需由项目明确处理一次。不要在任务、驱动和b0上重复反向。

云台若控制的是惯性角，反馈应来自现有IMU坐标处理；电机编码器角与惯性角不是同一个量。ADRC直接读取项目选定的反馈，原PID的OTHER_FEED指针不会自动被ADRC读取。

### 4.3 控制周期与调用顺序

MotorBase_c::dt_在[Online](D:/ACE/ACE_ECF_27/Module/motor/motor_base_def.hpp:941)中更新，表示反馈到达间隔，不是你的控制任务周期。ADRC的dt由控制任务提供，单位s；例如已确认的1kHz固定预测周期为0.001s。稳定任务中可使用所设计的固定周期，同时记录实际调度间隔；不能把上一次反馈间隔当作下一段命令必然维持的时间。

本库采用预测式顺序，当前测量为y[k]，当前估计状态为z[k]：

```text
读取本拍反馈和目标
→ Calc用z[k]算请求
→ 应用补偿、方向、限幅、功率分配和编码，提交本拍命令u[k]
→ UpdateObserver(u[k],y[k],dt)预测z[k+1]
→ 下一拍使用新估计状态
```

使用包含本拍最终输出处理的输入，不能把请求值当作受限后的输入。也不要混用另一套“先拿上一拍输出更新ESO，再计算本拍输出”的调用顺序。同一实例每区间更新一次，且由一个任务串行使用。
Calc失败时尚未提交这次新请求；UpdateObserver失败时命令可能已经提交，不能仅凭后者返回false就认为本拍没有发送。

运行库只检查基本参数、有限正周期和有效结果，不在每拍判定理论稳定性；参数与周期应先离线验证。原有启停、在线判断和失效处理继续由消费项目负责，不为ADRC新增一套电机安全任务。在线标志也不代替反馈有效性和更新时间判断。

## 5. DJI接入：力矩命令、原始RPM反馈

以下适用于M3508、M3508NEW、M2006。GM6020不适用这个固定力矩接口。

创建一个长期存在的ADRC::LADRC1 speed_adrc，在允许接管且反馈有效时，使用当前输出轴转速初始化：

```cpp
// motor是现有DjiDriver_c对象，direction为+1或-1。
const auto feedback = motor.GetMotorData();
const float measured_speed = direction * feedback.motor_raw_data.feedback_speed
    * rpm_to_rad_s / feedback.motor_fixed_param.ratio;
if (!speed_adrc.Init(b0_speed, omega_c, omega_o, measured_speed, initial_f))
    return;
```

b0_speed、omega_c、omega_o由项目选择；initial_f是初始总扰动估计，不能把N·m直接当作它。静止开始可选择0；从已有输出接管时，按[初始化说明](D:/ACE/Infantry_dev/ADRC/docs/库接入与应用说明.md)计算。不要在每次控制循环里重新Init。

控制任务中的单电机示例：target_speed是输出轴rad/s，torque_limit_nm是项目设置的力矩上限，dt是本次预测周期。

```cpp
const auto feedback = motor.GetMotorData();
const auto& fixed = feedback.motor_fixed_param;
const float measured_speed = direction * feedback.motor_raw_data.feedback_speed
    * rpm_to_rad_s / fixed.ratio;

float request_nm = 0.0f;
if (!speed_adrc.Calc(target_speed, request_nm)) return;

// Clamp使用第4节给出的普通限幅函数，或替换为项目已有的同类函数。
const float nm_per_code = fixed.torque_constant * fixed.current_resolution;
float driver_nm = Clamp(direction * request_nm, -torque_limit_nm, torque_limit_nm);
driver_nm = Clamp(driver_nm,
    motor.motor_min_output_ * nm_per_code,
    motor.motor_max_output_ * nm_per_code);
motor.SetMotorOutputFix(driver_nm);

Motor_n::DjiMotor_n::DjiMotorControl();
// 如项目有功率分配，在这里沿用原流程，最终写入各电机give_current_。
Motor_n::DjiMotor_n::set_GiveCurrent();
Motor_n::DjiMotor_n::MotorTransmit();

const float submitted_nm = direction * static_cast<float>(motor.give_current_) * nm_per_code;
if (!speed_adrc.UpdateObserver(submitted_nm, measured_speed, dt)) return;
```

这个示例假定电机已经按实际型号配置好ratio、力矩换算系数和有正有负的输出范围；编码限值应在int16可表示范围内。先限幅再调用SetMotorOutputFix，是为了让它内部的float到int16转换处于有效范围，不能等转换后再补救。具体数值来自现有项目，不在示例中硬编码。

SetMotorOutputFix接收N·m，内部按torque_constant和current_resolution换算。它把状态设为MOTOR_OUTPUT_ONLY_ME，DjiMotorControl便跳过该电机原PID。给它传A或电流编码值，都会再次换算而产生单位错误。

set_GiveCurrent还会按motor_min_output_/motor_max_output_进行最后限幅，所以观察器使用最终give_current_反算的N·m。motor_controller_.pid_output保存的可能仍是限功率前的值，不能据此认为已获得最终输入。当前RLS类功率控制与电机字段之间使用电流编码量的路径，接入时保留原单位关系，不将ADRC的N·m直接塞进该接口。

**多个DJI电机不能把上面的完整发送流程逐个执行。** 每拍先分别读取各轴反馈并计算请求、调用各自SetMotorOutputFix；全体只执行一次DjiMotorControl、一次现有功率分配、一次set_GiveCurrent和一次MotorTransmit，最后分别用各轴最终give_current_更新对应观测器。

该驱动分组发送，一组由多个电机共享。OnlyReceive会关闭对应组的发送标志，不能用它来“关闭一个电机的PID但保留组内发送”；ADRC固定输出应使用已经存在的MOTOR_OUTPUT_ONLY_ME路径。不要每拍调用Enable把这个模式改回去，也不要同时让其他任务写同一电机输出。

调用链：[固定力矩](D:/ACE/ACE_ECF_27/Module/motor/DJI/dji_driver.cpp:660) → [全体控制](D:/ACE/ACE_ECF_27/Module/motor/DJI/dji_driver.cpp:459) → [最后编码](D:/ACE/ACE_ECF_27/Module/motor/DJI/dji_driver.cpp:517) → [分组发送](D:/ACE/ACE_ECF_27/Module/motor/DJI/dji_driver.cpp:544)。

## 6. DM接入：位置与目标速度到力矩

创建长期存在的ADRC::LADRC2 angle_adrc。使用当前项目角度与当前角速度初始化，例如Init(b0_angle,omega_c,omega_o,measured_angle,measured_rate,initial_f)。电机先完成现有使能流程；Enable和标定不放在每拍控制计算中。

下列片段中motor是现有DmDriver_c对象，direction为项目方向，zero_bias_rad为项目位置偏置；target_angle与target_rate分别为rad、rad/s。tau_min_nm/tau_max_nm是项目最终力矩范围，须处于构造电机时DM_ModePrame_s的t_min/t_max协议范围内，后两项参数应保留在项目配置中，不能访问驱动的私有param_。

```cpp
const auto feedback = motor.GetMotorData();
const float measured_angle = direction
    * feedback.motor_processed_data.total_angle * deg_to_rad + zero_bias_rad;
const float measured_rate = direction * feedback.motor_raw_data.feedback_speed;

float request_nm = 0.0f;
if (!angle_adrc.Calc(target_angle, target_rate, request_nm)) return;

// 在这里加入项目已有补偿，再按驱动坐标限制最终力矩。
const float driver_nm = Clamp(direction * request_nm, tau_min_nm, tau_max_nm);
motor.SetMotorOutputFix(driver_nm);  // 本函数已经编码并立即发送。

// 按同一12位映射反算所提交的力矩命令；它仍不是实测电磁力矩。
const uint16_t code = Motor_n::float_to_uint(driver_nm, t_min, t_max, 12);
const float submitted_nm = direction * Motor_n::uint_to_float(code, t_min, t_max, 12);
if (!angle_adrc.UpdateObserver(submitted_nm, measured_angle, dt)) return;
```

本方案在MCU中完成位置反馈计算，SetMotorOutputFix内部使用SetMITData(0,0,0,0,torque)，不会再启用电机端位置/速度反馈项。MIT是这个驱动发送位置、速度、增益和力矩等参数的协议形式。

SetMITData本身只是缓存，不会自动发送，也不会自动按协议限幅。普通Transmit(float)在MOTOR_OUTPUT_ONLY_ME状态会跳过发送；而SetMotorOutputFix已经直接发送。因此本文不再追加Transmit或DmTransmit。使用另一条SetMITData+Transmit路径时，必须重新核对状态条件，不能与本文固定输出混用。

不要同时调用SelfLock发送位置保持命令。已显式加入重力或支撑力矩时，观测器输入应与所选模型和最终命令统一；不要直接照抄“不让前馈进ESO”的结论。普通同通道叠加方案使用含补偿与限幅的最终输入；若设计成独立力控通道，需要另行确定模型关系。

如果视觉端已提供有效目标速度，直接使用Calc(target_angle,target_rate,request)。若只有跳变目标角，可在外部使用TD平滑，不能将原始跳变直接差分成目标速度。

源码：[DM固定输出和MIT发送](D:/ACE/ACE_ECF_27/Module/motor/DM/dm_driver.cpp:41)、[普通Transmit状态条件](D:/ACE/ACE_ECF_27/Module/motor/DM/dm_driver.cpp:90)、[使能](D:/ACE/ACE_ECF_27/Module/motor/DM/dm_driver.cpp:228)。

## 7. VESC接入：速度到电流

Vesc_c位于Motor_n命名空间，当前有data()和is_online()接口，不使用MotorBase_c的字段。先启用RX_ENABLE_STATUS1，并在取得有效速度反馈后初始化ADRC::LADRC1 speed_adrc。is_online表示收到某个已启用状态报文，不能单独证明STATUS1速度刚刚刷新。

以下motor是现有Motor_n::Vesc_c对象，pole_pairs、gear_ratio和direction来自项目；target_speed为输出轴rad/s，current_min_A/current_max_A为项目最终电流限制。b0按A作为输入确定。

```cpp
const auto feedback = motor.data();
const float measured_speed = direction * static_cast<float>(feedback.erpm)
    * rpm_to_rad_s / pole_pairs / gear_ratio;

float request_A = 0.0f;
if (!speed_adrc.Calc(target_speed, request_A)) return;

// 若有多电机功率分配，先完成分配，再提交这里的最终电流。
const float driver_A = Clamp(direction * request_A, current_min_A, current_max_A);
motor.Set_Current(driver_A);  // A；接口内部乘1000后按int32发送。
const float submitted_A = direction
    * static_cast<float>(static_cast<int32_t>(driver_A * 1000.0)) * 0.001f;
if (!speed_adrc.UpdateObserver(submitted_A, measured_speed, dt)) return;
```

电流范围、极对数、齿比需在配置时确定，不能使用0极对数或0齿比。编码量也应在接口int32范围内。该驱动不会替ADRC统一这些单位与限制。

此方案不再调用Set_ERPM，因为Set_ERPM会给电调一个电转速目标，输入关系不同。Set_Current_Brake也不是普通有符号驱动电流，不能作为负电流命令的直接替代。框架当前没有STATUS1独立更新时间接口，反馈新鲜度由消费工程补充或沿用现有通信记录。

源码：[VESC反馈与在线接口](D:/ACE/ACE_ECF_27/Module/motor/VESC/vesc_driver.hpp:63)、[电流输出](D:/ACE/ACE_ECF_27/Module/motor/VESC/vesc_driver.cpp:358)、[电转速输出](D:/ACE/ACE_ECF_27/Module/motor/VESC/vesc_driver.cpp:387)。

## 8. GM6020、Cybergear、C615的当前限制

### GM6020

当前DjiDriver_c::SetMotorOutputFix在识别到GM6020时将pid_output和give_current_置零后直接返回。这是实际代码行为，不是ADRC没有位置控制能力。

若消费工程已有直接电压编码输出路径，可让ADRC使用该编码量作为u并按同一单位确定b0，再复用分组发送；但那是另一个明确的适配方案。本指南没有修改该驱动，也没有将公开的give_current_字段包装成一个已经验证过的通用电压接口。

### Cybergear

当前[CyberGearDriver_c声明](D:/ACE/ACE_ECF_27/Module/motor/Cybergear/cybergear_driver.hpp:159)继承MotorBase_c，却没有实现其[ParseCANData纯虚函数](D:/ACE/ACE_ECF_27/Module/motor/motor_base_def.hpp:920)，按当前声明仍为抽象类。纯虚函数是要求派生类提供实现的接口，缺少实现时不能直接建立该派生类对象。

现有MiMotorReceiveCallback直接填写原始反馈，但没有像DJI/DM通用回调那样建立相同的Online更新路径，也没有填充共同的processed speed/angle。构造函数还会发送设置机械零位命令。不能仅套用DM示例就宣称可接入。

后续修好并验证驱动后，可以研究MiMotorControlMode(torque,0,0,0,0)的力矩通道和原始反馈；SetMotorOutputFix当前只是继承基类的缓存动作，不等价于这个协议发送函数。本次不实施这些驱动修复。

### C615

当前类只有PWM输出，没有转速反馈。SetSpeed(percent)设置百分比，不是RPM；当前实现拒绝小于0.001或大于99.999的百分比，传入0不会执行一个停止动作，停止应按该驱动的Disable路径处理。

只有在项目有外部转速测量、并确定了PWM命令与速度的输入关系后，才能构造相应ADRC速度控制；模型输入为有效PWM命令，不能继续套用电流/力矩输入的b0。[C615实现](D:/ACE/ACE_ECF_27/Module/motor/C615/c615_driver.cpp:45)

motor_old下的LK、Unitree和旧版DJI/DM/Cybergear不属于本次已核查的当前驱动入口，不以旧目录名推断现版本的单位和调用行为。

## 9. TD、非线性反馈与在线改参如何接入

TD负责生成平滑目标及其变化率，不负责电机通信。Init时用当前目标或当前位置建立初态；每个区间读取当前TD输出给控制器使用，再Update推进到下一拍。LTD2和TD2均可显式设置速度/加速度限制，默认关闭。

若用非线性反馈替代线性反馈，保留同一套单位与最终输出流程：

```text
速度：ESO2的输出/扰动估计 → NLESF1::Calc(target,output,disturbance,command)
角度：LESO3或ESO3的输出/速度/扰动估计
      → NLESF2::Calc(target,target_rate,output,rate,disturbance,command)
```

观测器只在最终输出明确后更新一次，不再同时更新一个未使用的LADRC内部观测器。RLESO1要先用当前测量调用Calc重构扰动，再供反馈律使用，最后按本区间输入Update辅助状态。

SetParameters用于运行中改参并保留估计状态；它不保证切换后的力矩命令连续。正常模式/悬挂模式切换、接管旧PID、重力和支撑前馈由项目安排。参数组、单位和命令上限不要按电机型号写死在ADRC核心中。完整接口见[在线改参与状态限制](D:/ACE/Infantry_dev/ADRC/docs/在线改参与状态限制.md)。

## 10. 接入时观察哪些量

调试观察值是用于核对控制过程的实时变量。先记录同一坐标、同一单位的一组数据：

| 变量 | 单位或含义 |
| --- | --- |
| target_speed或target_angle/target_rate | rad/s或rad、rad/s |
| measured_speed或measured_angle/measured_rate | 与目标一致 |
| request_nm或request_A | ADRC原始请求 |
| driver_nm或driver_A | 驱动坐标下的提交请求 |
| DJI give_current_，或DM/VESC编码反算值 | 最终软件命令；观察器要使用同一模型单位 |
| GetEstimatedOutput/GetEstimatedRate | LADRC状态估计，分别为输出单位和输出单位/s |
| GetEstimatedDisturbance | 总扰动估计；不是直接读出的负载力矩 |
| dt与实际任务间隔 | s；另与反馈到达间隔区分 |
| 原有在线、工作模式和项目控制状态 | 确认由谁持有输出权，当前反馈是否可用 |

例程对反馈做普通结构复制，不等于建立中断与任务间的一致性保证；消费工程沿用已有反馈快照或同步方式。所有发送接口提交的是软件命令，不证明CAN已成功到达，也不等于测得的实际电流/力矩。

## 11. 本指南验证到哪一层

本次按当前源码核对了类、成员、单位、固定输出/发送路径、状态行为及构建入口，未修改ECF27源码。没有进行消费固件ELF链接、总线、电机和整车测试；代码片段也不代表已在某个实际HAL配置下编译通过。

ADRC原有主机与MATLAB检查证明库在其验证条件下的计算一致性，不能代替这里的驱动接入验证。实际接入可先选择已有反馈和有效力矩/电流接口的一路电机，完成单位、方向、命令提交值及状态时序核对，再扩展到多电机任务。
