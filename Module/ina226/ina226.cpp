
#include "ina226.hpp"
#include <math.h>
/*
使用前依据需求和实际更新：
max_expected_current_A  R_shunt
使用示例
Ina226_c power_monitor(&hi2c1_config, I2C_Callback);
power_monitor.InitIna226();
while(1)
{
power_monitor.ReadCurrent();
power_monitor.ReadBusVoltage();
power_monitor.ReadPower();
power_monitor.ReadShuntVoltage();
}
*/
void Get_Significant_Figures(float* num)
{
    if (*num == 0.0f) return; // 若为0则无需处理，直接返回

    float x = *num;
    float sign = (x > 0) ? 1.0f : -1.0f; // 记录符号
    x = fabsf(x); // 取绝对值方便处理

    // 计算数量级，例如 0.0123 -> -2， 123 -> 2
    int magnitude = (int)floorf(log10f(x));

    // 计算放大倍数，使前三位有效数字变为整数部分
    // factor = 10^(2 - magnitude)，例如 x=0.0123 -> factor=10^(2-(-2))=10^4
    float factor = powf(10.0f, 2 - magnitude); // 2 表示 3 位有效数字的偏移

    // 保留三位有效数字并四舍五入
    x = roundf(x * factor) / factor;

    *num = x * sign; // 还原符号
}

namespace Ina226_n
{
    Ina226_c::Ina226_c(BSP_n::HWI2CConfig_s *i2c_init_config ,
                       void (*hw_iic_callback)(BSP_n::I2C_c *i2c_init_config))
    {
        // 创建 I2C 对象
        ina226_i2c_ = new BSP_n::I2C_c(i2c_init_config, hw_iic_callback);

        address_ = 0x40; // 默认 INA226 器件地址

        // 电流 LSB = 最大电流 / 32768（芯片内部 ADC 满量程）
        current_lsb_A_ = max_expected_current_A_ / 32768.0f;

        Get_Significant_Figures(&current_lsb_A_); // 将电流 LSB 保留 3 位有效数字，提高标定寄存器精度

        power_lsb_W_ = 25.0f * current_lsb_A_; // 功率 LSB = 25 * 电流 LSB（芯片公式）
    }

    Ina226_c::Ina226_c(BSP_n::HWI2CConfig_s *i2c_init_config ,
                       void (*hw_iic_callback)(BSP_n::I2C_c *i2c_init_config),
                       uint8_t address,
                       INA226_AVERAGES average,
                       INA226_CONV_TIME convTime,
                       INA226_MEASURE_MODE mode)
    {
        // 创建 I2C 对象
        ina226_i2c_ = new BSP_n::I2C_c(i2c_init_config, hw_iic_callback);

        address_ = address;     // 用户自定义地址
        average_ = average;     // 用户自定义平均次数
        convTime_ = convTime;   // 用户自定义转换时间
        mode_ = mode;           // 用户自定义测量模式

        // 自动计算电流 LSB
        current_lsb_A_ = max_expected_current_A_ / 32768.0f;

        Get_Significant_Figures(&current_lsb_A_); // 保留三位有效数字

        power_lsb_W_ = 25.0f * current_lsb_A_; // 计算功率 LSB
    }

    void Ina226_c::UpdateSensorAdd(uint8_t new_add)
    {
        // 修改 INA226 器件地址
        this->address_ = new_add;
    }

    void Ina226_c::UpdateConfigRegister()
    {
        // 组装 CONFIG 寄存器
        uint16_t reg = 0x00 + (reset_ << 15); // reset 位位于 bit15

        reg |= (average_ << 9);   // average 配置在 bit[11:9]
        reg |= (convTime_ << 6);  // Bus/ shunt 转换时间
        reg |= (convTime_ << 3);  // 两个 convTime 配置（根据芯片手册）
        reg |= mode_;             // 测量模式（bit[2:0]）

        // 写入配置寄存器
        ina226_i2c_->write2Bytes(address_, CONFIG_REG, reg);
    }

    void Ina226_c::UpdateCalRegister()
    {
        uint16_t cal = 0;

        // 计算标定寄存器值 Cal = 0.00512 / (LSB_current * R_shunt)
        float cal_f = 0.00512f / current_lsb_A_ / R_shunt;

        Get_Significant_Figures(&cal_f); // 保留三位有效数字避免浮点误差

        cal = round(cal_f); // 四舍五入得到整数寄存器值

        // 写入标定寄存器
        ina226_i2c_->write2Bytes(address_, CALIB_REG, cal);
    }

    void Ina226_c::InitIna226()
    {
        // 写入配置寄存器 + 写入标定寄存器
        UpdateConfigRegister();
        UpdateCalRegister();
    }

    void Ina226_c::ReadShuntVoltage()
    {
        uint8_t data[2] = {0x00, 0x00};

        // 读取分流电压寄存器的 2 字节原始 ADC 值
        ina226_i2c_->readMultiBytes(address_, SHUNTVOL_REG, data, 2);

        // 合成 16 位带符号数据
        ina226_row_data_.shunt_voltage = (int16_t)((data[0] << 8) | data[1]);

        // 计算分流电压（单位：V）
        ina226_data_.Shuntvol_V = ina226_row_data_.shunt_voltage * shuntvol_lsb_V_;
    }

    void Ina226_c::ReadBusVoltage()
    {
        uint8_t data[2] = {0x00, 0x00};

        // 读取总线电压
        ina226_i2c_->readMultiBytes(address_, BUSVOL_REG, data, 2);

        // 合成 16 位 ADC 值
        ina226_row_data_.bus_voltage = (int16_t)((data[0] << 8) + data[1]);

        // 计算电压（单位 V）
        ina226_data_.Busvol_V = ina226_row_data_.bus_voltage * busvol_lsb_v_;
    }

    void Ina226_c::ReadCurrent()
    {
        uint8_t data[2] = {0x00, 0x00};

        // 读取电流寄存器
        ina226_i2c_->readMultiBytes(address_, CURRENT_REG, data, 2);

        // 转为带符号 16 位
        ina226_row_data_.current = (int16_t)((data[0] << 8) | data[1]);

        // 计算实际电流（单位 A）
        ina226_data_.Current_A = ina226_row_data_.current * current_lsb_A_;
    }

    void Ina226_c::ReadPower()
    {
        uint8_t data[2] = {0x00, 0x00};

        // 读取功率寄存器
        ina226_i2c_->readMultiBytes(address_, POWER_REG, data, 2);

        // 转为16位整数
        ina226_row_data_.power = (int16_t)((data[0] << 8) | data[1]);

        // 计算功率（单位 W）
        ina226_data_.Power_W = ina226_row_data_.power * power_lsb_W_;
    }
}
