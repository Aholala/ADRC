//
// Created by 47764 on 2025/11/30.
//

#ifndef INA226_HPP
#define INA226_HPP
#include <stdint.h>
#include <math.h>
#include "bsp_i2c.hpp"
#define max_expected_current_A 30.0f //最大预期电流30A
#define R_shunt 0.002f
namespace  Ina226_n
{
    //All Register Addresses of the INA226, each Register 16 Bits in length，INA226的所有寄存器地址
    const uint8_t CONFIG_REG 	= 0x00;		//Config Register
    const uint8_t SHUNTVOL_REG 	= 0x01;		//Shunt Voltage Register
    const uint8_t BUSVOL_REG 	= 0x02;		//Bus Voltage Register
    const uint8_t POWER_REG 	= 0x03;		//Power Register
    const uint8_t CURRENT_REG 	= 0x04;		//Current Register
    const uint8_t CALIB_REG 	= 0x05;		//Calibration Register
    const uint8_t MASKEN_REG 	= 0x06;		//Mask / Enable Register
    const uint8_t ALERT_REG 	= 0x07;		//Alert Register
    const uint8_t MANUF_REG 	= 0xFE;		//Manufacturer ID Register
    const uint8_t UNQID_REG 	= 0xFF;		//Unique ID Register

    enum INA226_AVERAGES{		//Number of samples to be averaged，平均模式
        AVERAGE_1       = 0b000,//1次平均
        AVERAGE_4       = 0b001,//4次平均
        AVERAGE_16      = 0b010,//16次平均
        AVERAGE_64      = 0b011,//64次平均
        AVERAGE_128     = 0b100,//128次平均
        AVERAGE_256     = 0b101,//256次平均
        AVERAGE_512     = 0b110,//256次平均
        AVERAGE_1024    = 0b111//256次平均
    };

    enum INA226_CONV_TIME{		//Conversion Time in us
        CONV_TIME_140   = 0b000,//140 μs
        CONV_TIME_204   = 0b001,//204 μs
        CONV_TIME_332   = 0b010,//332 μs
        CONV_TIME_588   = 0b011,//588 μs
        CONV_TIME_1100  = 0b100,//1.1 ms
        CONV_TIME_2116  = 0b101,//2.116 ms
        CONV_TIME_4156  = 0b110,//4.156 ms
        CONV_TIME_8244  = 0b111 //8.244 ms
    };

    enum INA226_MEASURE_MODE{	//Measure Modes，工作模式
        POWER_DOWN      	= 0b000,//断电模式（或关机）
        SHUNT_TRIGGERED     = 0b001,// 连续测量电流，触发测量电压
        BUS_TRIGGERED      	= 0b010,//连续测量总线电压，触发测量电流
        SHUNTBUS_TRIGGERED	= 0b011,//连续测量电流和总线电压，触发测量
        SHUNT_CONTINOUS		= 0b101,//连续测量电流
        BUS_CONTINOUS		= 0b110,//连续测量总线电压
        SHUNTBUS_CONTINOUS	= 0b111//连续测量电流和总线电压（默认）
    };
    typedef struct
    {
        int16_t shunt_voltage;	//Shunt Voltage in V，分流电压，单位V
        int16_t bus_voltage;		//Bus Voltage in V，总线电压，单位V
        int16_t current;			//Current in A，电流，单位A
        int16_t power;			//Power in W，功率，单位W
    }Ina226RowData_t;

    typedef struct
    {
        float Shuntvol_V;
        float Busvol_V;
        float Power_W;
        float Current_A;
    }Ina226Data_t;

    class Ina226_c
    {
        const uint8_t reset_=0x00;
        const float busvol_lsb_v_=0.00125f;//总线电压最小分辨率
        const float shuntvol_lsb_V_=2.5e-6;//分流电压最小分辨率
        float max_expected_current_A_=max_expected_current_A;
        float current_lsb_A_;//电流最小分辨率
        float power_lsb_W_;//功率最小分辨率
        Ina226RowData_t ina226_row_data_;
        uint8_t address_=0x41;
        INA226_AVERAGES average_ = AVERAGE_4;//4次平均
        INA226_CONV_TIME convTime_ = CONV_TIME_332;//332 μs
        INA226_MEASURE_MODE mode_ = SHUNTBUS_CONTINOUS;//连续测量电流和总线电压（默认）
    public:
        Ina226Data_t ina226_data_;
        BSP_n::I2C_c *ina226_i2c_=nullptr;
        Ina226_c(){}
        Ina226_c(BSP_n::HWI2CConfig_s *i2c_init_config ,void (*hw_iic_callback)(BSP_n::I2C_c *i2c_init_config));
        Ina226_c(BSP_n::HWI2CConfig_s *i2c_init_config ,void (*hw_iic_callback)(BSP_n::I2C_c *i2c_init_config),uint8_t address,INA226_AVERAGES average,INA226_CONV_TIME convTime,INA226_MEASURE_MODE mode);
        void InitIna226();
        void UpdateSensorAdd(uint8_t new_add);
        void UpdateCalRegister();
        void UpdateConfigRegister();
        void ReadShuntVoltage();
        void ReadBusVoltage();
        void ReadCurrent();
        void ReadPower();
    };
};
#endif //INA226_HPP