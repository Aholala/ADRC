#ifndef __DFM113D_CONFIG_HPP
#define __DFM113D_CONFIG_HPP

/**************** MODBUS协议基础-数据包构成(耦合了，考虑把MODBUS部分搬到USART里面) ****************
 *
 * 读：通讯地址-功能码-寄存器地址-操作的寄存器数量-CRC校验码
 *  * 收：通讯地址-功能码-字节数n-DATA(n byte)-CRC校验码
 *
 * 写(单个)：通讯地址-功能码-寄存器地址-DATE(2byte * 1)-CRC校验码
 *  * 收：通讯地址-功能码-寄存器地址-DATE(2byte * 1)-CRC校验码
 *
 * 写(多个)：通讯地址-功能码-寄存器地址-操作的寄存器数量n(uint16_t)-参数字节数-DATA(2byte * n)-CRC校验码
 *  * 收：通讯地址-功能码-寄存器地址-操作的寄存器数量-CRC校验码
 *
 */

// MODBUS协议-功能码
#define MODBUS_CODE_READ_PUBLIC   0x03u   // 读保持寄存器(可读可写)
#define MODBUS_CODE_READ_PRIVATE  0x04u   // 读输入寄存器(只读)
#define MODBUS_CODE_WRITE_ONE     0x06u   // 写单个保持寄存器
#define MODBUS_CODE_WRITE         0x10u   // 写多个保持寄存器

// 数据包长
#define DFM113D_BUFF_MAX 0x0Du      // 写多个模式下写两个u16寄存器
#define DFM113D_LEN_READ_DATA 0x09u // 读数据包长

// 寄存器起始地址（AI帮搞的，可能有💩）
// 密码寄存器
#define DFM113D_ADDR_99OA 0x01u    // 参数密码输入地址，取值范围0~9999，默认0
// 测量参数 以下受密码1111(LOAC)保护，未设置密码时不能进入
#define DFM113D_ADDR_IND  0x33u    // 小数点位置，取值范围0~5，默认值000000
#define DFM113D_ADDR_TRD  0x34u    // 零点跟踪范围，取值范围-200~200（分度），默认值0
#define DFM113D_ADDR_ZROR 0x35u    // 清零范围，取值范围-0.99~0.99，默认值0.99
#define DFM113D_ADDR_FLTR 0x36u    // 数字滤波时间常数，取值范围1~20，默认值1
#define DFM113D_ADDR_NOTN 0x37u    // 变动检测阈值，取值范围0~200（分度），默认值0
#define DFM113D_ADDR_ARMA 0x37u    // 滑动平均滤波系数，取值范围1~20，默认值1
#define DFM113D_ADDR_MOTH 0x39u    // 测量修正判断门限，取值范围 -199999~999999，默认值 0
#define DFM113D_ADDR_MOV  0x3Au    // 测量修正数值，取值范围 -199999~999999，默认值 0
#define DFM113D_ADDR_AT   0x3Bu    // 显示更新速率，取值范围 10、20（次/秒），默认值 10
#define DFM113D_ADDR_SPS  0x3Cu    // 测量速率选择，取值范围 10/80（次/秒），默认值 10
#define DFM113D_ADDR_MAT  0x3Eu    // 峰值阈值，取值范围 -199999~999999，默认值 - 199999
#define DFM113D_ADDR_MAB  0x3Fu    // 峰值回差，取值范围 -199999~999999，默认值 0
#define DFM113D_ADDR_MINT 0x40u    // 谷值阈值，取值范围 -199999~999999，默认值 999999
#define DFM113D_ADDR_MINB 0x41u    // 谷值回差，取值范围 -199999~999999，默认值 0
#define DFM113D_ADDR_DI0F 0x42u    // 开关量输入功能选择，取值范围 0~10，默认值 ZEro
#define DFM113D_ADDR_POC  0x101u   // 上电清零功能选择，取值范围 0：oFF/1：on/2：delay，默认值 oFF
#define DFM113D_ADDR_DISP 0x102u   // 上电显示内容选择，取值范围 0~6，默认值 Gross
#define DFM113D_ADDR_TRS  0x103u   // 零点跟踪时间，取值范围 0.0~10.0，默认值 0.0
// 通讯参数 以下受密码1111(LOAC)保护，未设置密码时不能进入
#define DFM113D_ADDR_ADD  0x48u    // 通讯地址，取值范围 1~255，默认值 1
#define DFM113D_ADDR_BAUD 0x49u    // 通讯速率选择，取值范围 0~6，默认值 9600
#define DFM113D_ADDR_OES  0x4Au    // 校验方式选择（仅 Modbus），取值范围 0~2，默认值 n
#define DFM113D_ADDR_PRO  0x4Du    // 通讯协议选择，取值范围 0: tc-ASC/1:Modbus，默认值 Modbus
#define DFM113D_ADDR_ACT  0x4Eu    // 主动发送选择（仅 tc-ASC），取值范围 0~8，默认值 nonE
#define DFM113D_ADDR_STOP 0x4Fu    // 停止位选择（仅 Modbus），取值范围 1~2，默认值 1
#define DFM113D_ADDR_DLY  0x105u   // 模块向主机发送应答前的延迟，单位为 us，取值范围 - 2~127，默认值 - 1
// 折线修正参数(N为折线函数) 以下受密码1111(LOAC)保护，未设置密码时不能进入
#define DFM113D_ADDR_FNUM 0x4Fu    // 折线修正段数选择，取值范围 0~10，默认值 0
#define DFM113D_ADDR_F1   0x50u    // 第 1 点测量值，取值范围 - 199999~999999，默认值 0.0001~0.0010（第n点地址计算公式：50u+(n-1)×2）
#define DFM113D_ADDR_S1   0x51u    // 第 1 点标准值，取值范围 - 199999~999999，默认值 1~10（第n点地址计算公式：51u+(n-1)×2）
#define DFM113D_ADDR_FMV  0x80u    // 折线物理量选择，取值范围 0：oFF/1：on，默认值 oFF
// 标定参数 以下受密码1111(LOAC)保护，未设置密码时不能进入
#define DFM113D_ADDR_CALM 0x64u    // 标定方式选择，取值范围0：norm/1：tEmP/2：Auto，默认值norm
#define DFM113D_ADDR_CALT 0x65u    // 标定时间，取值范围1~120（分钟），默认值20
#define DFM113D_ADDR_MVV  0x66u    // 传感器灵敏度（仅无砝码标定），取值范围0.10000~5.00000（mV/V），默认值2.00000
#define DFM113D_ADDR_CAL0 0x67u    // 零点标定时的零点mV值，取值范围以实时测量值为准，无默认值
#define DFM113D_ADDR_CALF 0x68u    // 增益标定时的增益mV值（仅砝码标定），无取值范围，无默认值
#define DFM113D_ADDR_CALP 0x69u    // 增益标定对应的重量值，取值范围1~999999，默认值10000
#define DFM113D_ADDR_INA  0x6Au    // 零点修正值（仅无砝码标定），取值范围-199999~999999，默认值0
#define DFM113D_ADDR_FI   0x6Bu    // 满度修正值（仅无砝码标定），取值范围-1.99999~9.99999，默认值1.00000
#define DFM113D_ADDR_FD   0x6Cu    // 测量值分度，取值范围1,2,5,10,20,50，默认值1
#define DFM113D_ADDR_FR   0x6Du    // 模块最大量程，取值范围1~999999，默认值15000
#define DFM113D_ADDR_LOCK 0x6Eu    // 锁定标定系数选择，取值范围0：oFF/1：on，默认值oFF
// 用户参数 以下受密码2027保护，未设置密码时不能进入
#define DFM113D_ADDR_SAVE 0x1FF1u  // 用户备份参数，取值范围0：oFF/1：on，默认值oFF
#define DFM113D_ADDR_LOAD 0x1FF2u  // 恢复用户备份参数，取值范围0：oFF/1：on，默认值oFF
#define DFM113D_ADDR_DEF  0x1FF3u  // 恢复出厂参数，取值范围0：oFF/1：on，默认值oFF
#define DFM113D_ADDR_VER  0x1FF5u  // 模块软件版本
#define DFM113D_ADDR_ACOP 0x1FF6u  // 在智能模块上读取或存入自动标定参数，取值范围0：nonE/1：c-SAuE/2：c-LoAd，默认值nonE
#define DFM113D_ADDR_TEDS 0x1FF7u  // 选择智能模块通讯模式，取值范围0：oFF/1：on，默认值on
#define DFM113D_ADDR_LOAC 0x1FF8u  // 替换参数密码，取值范围0~9999，默认值1111

#endif //! __DFM113D_CONFIG_HPP
