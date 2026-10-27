#ifndef BSP_I2C_HPP
#define BSP_I2C_HPP

#define I2C_DEVICE_CNT 2    // C板引出了I2C2和I2C3
#define MX_I2C_SLAVE_CNT 8  // 最大从机数目,根据需要修改
#define I2C_DELAY_TIME 0.000002 // dwt暂时只支持ms级延时

/*I2C线路高低电平设置，并不是逻辑电平*/
#define I2C_SCL_H()     SW_I2C_W_SCL(1)
#define I2C_SCL_L()     SW_I2C_W_SCL(0)
#define I2C_SDA_H()     SW_I2C_W_SDA(1)
#define I2C_SDA_L()     SW_I2C_W_SDA(0)


#ifdef __cplusplus
extern "C"{
#endif

#include "main.h"
#include <stdint.h>
#include "i2c.h"
#include "gpio.h"
#include "stdlib.h"
#include "bsp_dwt.hpp"

#ifdef __cplusplus
}

#endif

namespace BSP_n
{
    /* I2C 工作模式枚举 */
    typedef enum
    {
        // 基本工作模式
        I2C_BLOCK_MODE = 0, // 阻塞模式
        I2C_IT_MODE,        // 中断模式
        I2C_DMA_MODE,       // DMA模式
        Software_I2C,       // 模拟IIC

    } I2CWorkMode_e;

    /* I2C MEM工作模式枚举,这两种方法都是阻塞 */
    typedef enum
    {
        I2C_READ_MEM = 0, // 读取从机内部的寄存器or内存
        I2C_WRITE_MEM,    // 写入从机内部的寄存器or内存
    } I2CMemMode_e;

    /* I2C回调函数类型枚举 */
    typedef enum
    {
        I2C_Master = 0, //Master
        I2C_Mem,        //Mem
    } I2CCallbackMode_e;

    /* I2C状态枚举 */
    typedef enum {
        I2C_Result_Ok = 0x00,
        I2C_Result_Error,
    } I2CResult_t;

    /* 模拟I2C应答信号状态枚举 */
    typedef enum
    {
        NACK = 0,
        ACK  = 1
    } I2CACKSTATUS_e;

    /* 模拟I2C线路读写状态 */
    typedef enum
    {
        Read =0,
        Write,
    } I2CRWStatus_e;

    /* 硬件I2C 初始化结构体配置 */
    typedef struct
    {
        I2C_HandleTypeDef *i2c_handle;       // i2c handle
        uint8_t device_address;             // 设置写入数据的地址
        I2CWorkMode_e work_mode;         // 工作模式
    } HWI2CConfig_s;

    /* 模拟I2C 初始化结构体配置 */
    typedef struct
    {
        GPIO_TypeDef *i2c_scl_port;         //i2c scl的gpio端口配置
        uint16_t i2c_scl_pin;          //i2c scl的gpio引脚配置
        GPIO_TypeDef *i2c_sda_port;         //i2c sda的gpio端口配置
        uint16_t i2c_sda_pin;          //i2c sda的gpio引脚配置
        uint8_t device_address;            // 设置写入数据的地址
    } SWI2CConfig_s;


    class I2C_c
    {
    public:
        I2C_c(SWI2CConfig_s *i2c_init_config);
        I2C_c(HWI2CConfig_s *i2c_init_config ,void (*hw_iic_callback)(I2C_c* i2c_instance));
        void SW_I2C_Init(void);
        /*硬件I2C函数*/
        void HW_I2C_Transmit(uint8_t *data, uint16_t size);     //I2C发送数据
        void HW_I2C_Receive(uint8_t *data, uint16_t size);     //I2C接收数据
        //I2C读取从机寄存器(内存),只支持阻塞模式,超时默认为1ms
        void HW_I2CAccessMem(uint16_t mem_addr, uint8_t *data, uint16_t size, I2CMemMode_e mem_mode, uint8_t mem8bit_flag);
        /*以下为旧时代产物，能不能用自己测*/
        void I2cClassInit(I2C_HandleTypeDef* hi2c);
        I2CResult_t writeByte(uint8_t device_address, uint8_t register_address, uint8_t data);
        I2CResult_t writeByte(uint8_t device_address, uint8_t data);
        I2CResult_t write2Bytes(uint8_t device_address, uint8_t register_address, uint16_t data);
        I2CResult_t readMultiBytes(uint8_t device_address, uint8_t register_address, uint8_t* data, uint16_t count);
        uint8_t readOneByte(uint8_t device_address, uint8_t register_address);
        /*以下为回调函数*/
        static void Bsp_HW_I2C_TxCallback(I2C_HandleTypeDef *hi2c, I2CCallbackMode_e callback_type);
        I2CCallbackMode_e Callback_type_;          //回调类型
        /*模拟I2C函数*/
        void SW_I2C_W_SCL(uint8_t bit);           //写SCL
        void SW_I2C_W_SDA(uint8_t bit);           //写SDA
        uint8_t SW_I2C_R_SCL(void);           //读SCL
        uint8_t SW_I2C_R_SDA(void);           //读SDA
        void SW_I2C_Output(void);        //设置I2C为输出
        void SW_I2C_Input(void);         //设置I2C为输入
        //void SW_I2C_Init(void);
        void SW_I2C_Start(void);                                //I2C起始信号
        void SW_I2C_Stop(void);                                 //I2C停止信号
        void SW_I2C_ACK(void);                                  //I2C应答信号
        void SW_I2C_NACK(void);                                 //I2C无应答信号
        uint8_t SW_I2C_Wait_ACK(void);                          //I2C等待应答信号
        void SW_I2C_W_H(void);                                  //I2C写数据1
        void SW_I2C_W_L(void);                                  //I2C写数据0
        uint8_t SW_I2C_Write_Byte(uint8_t Byte);                //I2C写入单个数据
        uint8_t SW_I2C_Recv_Byte(I2CACKSTATUS_e ack_sta);           //I2C读取单个数据
    protected:
        static I2C_c *i2c_instance_[I2C_DEVICE_CNT];      //I2C实例指针数组
        static uint8_t idx_;                                        // 全局I2C实例索引,每次有新的模块注册会自增
        static uint8_t hw_idx_;                                     //硬件I2C实例数量
        //初始化的数据
        //共有的类型
        uint8_t device_address_;                            //从机地址
        //硬件的
        I2C_HandleTypeDef *i2c_handle_ = nullptr;          // i2c句柄
        I2CWorkMode_e work_mode_;                       //工作模式
        //软件的
        GPIO_TypeDef *i2c_scl_port_;         //i2c scl的gpio端口配置
        uint16_t i2c_scl_pin_;          //i2c scl的gpio引脚配置
        GPIO_TypeDef *i2c_sda_port_;         //i2c sda的gpio端口配置
        uint16_t i2c_sda_pin_;          //i2c sda的gpio引脚配置

        uint8_t *rx_buffer;                     // 接收缓冲区指针
        uint8_t rx_len_;                         // 接收长度
        void (*hw_iic_callback)(I2C_c* I2C_Instance);        // 接收完成后的回调函数
    };
}
#endif // !BSP_I2C_HPP
