#ifndef __DFM113D_HPP
#define __DFM113D_HPP

#include "bsp_usart.hpp"
#include "bsp_dwt.hpp"
#include "dfm113d_def.hpp"

namespace Dfm113d_n
{
    typedef enum class PendingCmd_e : uint8_t
    {
        NONE = 0,
        READ_DATA,
        READ_SETTING,
        WRITE_SETTING,
    }PendingCmd_e;

    typedef enum class TxStage_e : uint8_t
    {
        IDLE = 0,
        WAIT_UNLOCK_ACK,
        WAIT_CMD_ACK,
    }TxStage_e;

    // 工作状态
    typedef enum class WorkingState_e : int8_t
    {
        Error  = -1, // 未定义异常
        Normal = 0,  // 正常工作
        Busy   = 1,  // 等待应答
        Lock   = 2,  // 锁定无法改配置
    }WorkingState_e;

    // 读数据码
    typedef enum class ReadDataType_e : uint16_t
    {
        GROSS = 0x0000u, // 仪表总值
        NET   = 0x0004u, // 仪表净值
        PEAK  = 0x0008u, // 仪表峰值
        VALL  = 0x000Cu, // 仪表谷值
        P_V   = 0x0010u, // 仪表峰-谷值
        tP    = 0x0014u, // 仪表峰值过程量
        tv    = 0x0018u, // 仪表谷值过程量
//        Show  = 0x001Cu, // 仪表显示值
    }ReadDataType_e;

    typedef struct
    {
        UART_HandleTypeDef* use_usart = nullptr;
        uint8_t addr = 1;
        float password = 1111;
    } Config_t;

    typedef struct
    {
        uint8_t modbus_code; // 功能码
        float date;          // 返回数据
        uint8_t fresh_num;   // 刷新(接收成功+1，用于外部调用判断数据是否为最新)
    }RxData_t;

    class Dfm113d_c
    {
    public:
        Dfm113d_c(Config_t config);

        /* 有关信息获取 */
        RxData_t* rx_data() { return &rx_data_;}
        uint16_t CheckLost();

        /* 统一调度辅助成员 */
        UART_HandleTypeDef* usart_handle_ = nullptr;
        uint8_t last_send_valid_ = 0;
        uint32_t last_send_cnt_ = 0;
        Dfm113d_c* next_device_ = nullptr;
        void _send_frame(uint8_t* pdata, uint16_t psize, TxStage_e stage);
        void _build_read_data_frame(ReadDataType_e type);
        void _build_read_setting_frame(uint16_t addr);
        void _build_write_setting_frame(uint16_t addr, float setting);
        void _build_unlock_frame(uint8_t* pdata, uint16_t* psize);
        uint8_t _has_pending_frame() const { return pending_cmd_ != PendingCmd_e::NONE;}

        /* 主要函数 */
        WorkingState_e Fill_ReadData(ReadDataType_e type);
        WorkingState_e Fill_ReadSetting(uint16_t addr);
        WorkingState_e Fill_WriteSetting(uint16_t addr, float setting);

        friend void Dfm113dCallback(const uint8_t *pdata, uint16_t psize);
        friend void AllUpdate();

    private:
        /* user define date */
        uint8_t driver_addr_;   // 设备地址
        float password_;        // 密码
        uint8_t is_lock_ = 2;   // 是否锁定, 2锁定，1正在解锁，0已解锁

        /* package date */
        RxData_t rx_data_ = {};
        uint16_t no_ack_package_num_ = 0;               // 一个无应答 变量加一
        PendingCmd_e pending_cmd_ = PendingCmd_e::NONE; // 在干啥
        TxStage_e tx_stage_ = TxStage_e::IDLE;          // 干的咋样了
        uint8_t pending_tx_buff_[DFM113D_BUFF_MAX] = {};
        uint16_t pending_tx_size_ = 0;

        /* tool */
        BSP_n::USART_c *use_usart_ = nullptr;
        uint8_t tx_buff_[DFM113D_BUFF_MAX] = {};
        uint8_t rx_buff_[2][DFM113D_BUFF_MAX] = {}; // H7双缓冲DMA爽用

        /* timer */
        float d_time = .0f;
        uint32_t timer_cnt_ = 0;

        uint8_t _is_wait_ack() const { return tx_stage_ != TxStage_e::IDLE;}
        uint8_t _is_busy() const { return _is_wait_ack() || _has_pending_frame();}
        void ProcessData(const uint8_t *pdata, uint16_t psize);
    };
    void Dfm113dCallback(const uint8_t *pdata, uint16_t psize);
    void AllUpdate();
}

#endif //! __DFM113D_HPP
