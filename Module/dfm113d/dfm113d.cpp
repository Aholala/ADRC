/*************************** Dongguan-University of Technology -ACE**************************
 * @file    dfm113d.cpp
 * @author  L_Zero
 * @brief   力传感测量计dfm113d通讯&驱动库
 *
 ********************************************************************************************
 * @how_to_use
 *  创建实例，Fill_*填充发送数据，循环运行AllUpdate()更新实现发送;
 *  成员rx_date_.date存有收到的数据;
 *
 * @attention
 *  多个地址相同的测量计会导致回调函数访问错误，需要单独连接修改测量计的地址
 *  采用DMA双缓冲+串口空闲中断，记得配置CubeMX
 *  所有基础通讯采用IEEE-754数据格式，全是float
 *  写参数最多可重复写 10 万次，编程时要特别注意！不要频繁写入！
 *
 * @version
 *  v26.0.0.0 -- 25/12/18
 *  *   写过最恶心的库，CRC用的和裁判系统不一样，寄存器还多，一堆要实现的功能
 *  *   总之就写了部分功能能用就行吧，待以后完善
 *  v26.0.1.0 -- 26/01/15
 *  *   不错，优化了函数，大部分功能实现了，CRC还是要自己算XD
 *  v26.1.0.0 -- 26/06/15
 *  *   完成MODBUS CRC-16自动计算（modbus_crc），不再需要手动传入CRC校验码
 *  *   Config_t移除crc16字段，函数签名移除crc参数
 *  v26.1.1.0 -- 26.6.16
 *  *   优化写寄存器操作，不需要再手动解锁
 *  v26.2.0.0 -- 26.6.22
 *  *   完善使用流程，更简单的发送
 *
 ************************** Dongguan-University of Technology -ACE***************************/

#include "dfm113d.hpp"
#include "modbus_crc.hpp"
#include "ecf_configList.h"

extern "C"
{
#include "string.h"
}

namespace Dfm113d_n
{
    // 注册设备链表
    Dfm113d_c* dfm113d_instance_head = nullptr;

    static uint8_t _is_usart_send_ready(UART_HandleTypeDef* usart_handle)
    {
        Dfm113d_c* temp = dfm113d_instance_head;
        while (temp != nullptr)
        {
            // 同一底层串口共用发送节流，避免链表轮询过快导致连续发包
            if (temp->usart_handle_ == usart_handle && temp->last_send_valid_ != 0)
            {
                uint32_t temp_cnt = temp->last_send_cnt_;
                if (BSP_n::DWT_c::Get_DwtInstance()->GetDeltaT(&temp_cnt) < DFM113D_SEND_TIME)
                {
                    return 0;
                }
            }
            temp = temp->next_device_;
        }
        return 1;
    }

    void _memrevcpy(void* target, const void *source, size_t len)
    { // 反向复制
        char* t = (char*)target;
        const char* s = (const char*)source;
        for (size_t i = 0; i < len; i++)
        {
            t[i] = s[len - 1 - i];
        }
    }

    // 构造
    Dfm113d_c::Dfm113d_c(Config_t config) : driver_addr_(config.addr), password_(config.password), usart_handle_(config.use_usart)
    {
        BSP_n::USART_c::Config usart_param = {
            .Handle = config.use_usart,
            .RxBuffSize = DFM113D_BUFF_MAX * 2,      // 接收区大小
            .RxType = BSP_n::USART_c::RxType_t::DMA_IDLE_DOUBLE,// 接收类型 DMA_IDLE_DOUBLE
            .TxType = BSP_n::USART_c::TxType_t::DMA,
            .pTxCallback = nullptr,
            .pRxCallback = &Dfm113dCallback,
            .expected_data_length = DFM113D_BUFF_MAX,
            .pRxBuffer = rx_buff_[0],
            .pSecRxBuffer = rx_buff_[1]};
        use_usart_ = new BSP_n::USART_c(usart_param);
        use_usart_->start_receive();

        // 链表注册
        Dfm113d_c *temp = dfm113d_instance_head;
        if(dfm113d_instance_head == nullptr)
        {
            dfm113d_instance_head = this;
            return;
        }
        while(temp->next_device_ != nullptr) temp = temp->next_device_;
        temp->next_device_ = this;
    }

    // 回调
    void Dfm113dCallback(const uint8_t *pdata, uint16_t psize)
    {
        Dfm113d_c *temp = dfm113d_instance_head;
        while (temp != nullptr)
        {
            if(temp->driver_addr_ == pdata[0]) // byte0 设备ID
            {
                temp->ProcessData(pdata, psize);
                return;
            }
            temp = temp->next_device_;
        }
    }

    // 反馈数据处理 CRC检测因为算量大 不检测了
    void Dfm113d_c::ProcessData(const uint8_t *pdata, uint16_t psize)
    {
        switch (pdata[1]) // byte1 功能码
        {
            case MODBUS_CODE_READ_PRIVATE: // 读私有，一般是测量值
            {
                if(psize != 9)return;
                _memrevcpy(&rx_data_.date, pdata + 3, 4);
                // 读命令收到合法回包后，当前待发命令已经完成
                tx_stage_ = TxStage_e::IDLE;
                pending_cmd_ = PendingCmd_e::NONE;
                pending_tx_size_ = 0;
                rx_data_.modbus_code = MODBUS_CODE_READ_PRIVATE;
                rx_data_.fresh_num++;
                break;
            }
            case MODBUS_CODE_READ_PUBLIC: // 读私公开，一般是设置值
            {
                if(psize != 9)return;
                _memrevcpy(&rx_data_.date, pdata + 3, 4);
                // 读命令收到合法回包后，当前待发命令已经完成
                tx_stage_ = TxStage_e::IDLE;
                pending_cmd_ = PendingCmd_e::NONE;
                pending_tx_size_ = 0;
                rx_data_.modbus_code = MODBUS_CODE_READ_PUBLIC;
                rx_data_.fresh_num++;
                break;
            }
            case MODBUS_CODE_WRITE_ONE:
            {
                // 因为dfm113d的内部寄存器都是float(两个uint16)，不可能有单寄存器情况
                break;
            }
            case MODBUS_CODE_WRITE: // 写多寄存器，一般是设置值
            {
                if(psize != 8)return;
                if(tx_stage_ == TxStage_e::WAIT_UNLOCK_ACK)
                {
                    // 解锁回包到达后，仅切换为已解锁，真正写寄存器留给后续AllUpdate发送
                    is_lock_ = 0;
                }
                else if(tx_stage_ == TxStage_e::WAIT_CMD_ACK)
                {
                    // 写寄存器回包到达后，本次写流程完整结束，设备重新视为锁定
                    is_lock_ = 2;
                    pending_cmd_ = PendingCmd_e::NONE;
                    pending_tx_size_ = 0;
                }
                tx_stage_ = TxStage_e::IDLE;
                rx_data_.modbus_code = MODBUS_CODE_WRITE;
                rx_data_.fresh_num++;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    // 检测丢包
    uint16_t Dfm113d_c::CheckLost()
    {
        if(_is_wait_ack())
        {
            uint32_t temp_cnt = timer_cnt_;
            if(BSP_n::DWT_c::Get_DwtInstance()->GetDeltaT(&temp_cnt) > DFM113D_NOACK_TIME)
            { // 计一次丢包，并结束本轮等待流程，防止卡包
                no_ack_package_num_ = no_ack_package_num_ < UINT16_MAX ? no_ack_package_num_ + 1 : no_ack_package_num_;
                if(tx_stage_ == TxStage_e::WAIT_UNLOCK_ACK)
                {
                    // 解锁超时后恢复成锁定态，等待后续统一调度重新解锁
                    is_lock_ = 2;
                }
                if(tx_stage_ == TxStage_e::WAIT_CMD_ACK && pending_cmd_ == PendingCmd_e::WRITE_SETTING)
                {
                    // 写设置超时后保留待写命令，仅回到锁定态，便于后续重试完整流程
                    is_lock_ = 2;
                }
                if(tx_stage_ == TxStage_e::WAIT_CMD_ACK && pending_cmd_ != PendingCmd_e::WRITE_SETTING)
                {
                    // 读命令超时直接丢弃，避免旧请求长期滞留
                    pending_cmd_ = PendingCmd_e::NONE;
                    pending_tx_size_ = 0;
                }
                tx_stage_ = TxStage_e::IDLE;
            }
        }
        return no_ack_package_num_;
    }

    void Dfm113d_c::_send_frame(uint8_t* pdata, uint16_t psize, TxStage_e stage)
    {
        use_usart_->send(pdata, psize);
        // 统一通过DWT封装接口刷新发送时刻，供同串口实例统一限速
        BSP_n::DWT_c::Get_DwtInstance()->GetDeltaT(&last_send_cnt_);
        last_send_valid_ = 1;
        BSP_n::DWT_c::Get_DwtInstance()->GetDeltaT(&timer_cnt_);
        tx_stage_ = stage;
    }

    void Dfm113d_c::_build_read_data_frame(ReadDataType_e type)
    {
        memset(pending_tx_buff_, 0, DFM113D_BUFF_MAX);
        pending_tx_buff_[0] = driver_addr_;
        pending_tx_buff_[1] = MODBUS_CODE_READ_PRIVATE;
        pending_tx_buff_[2] = (uint16_t)type >> 8;
        pending_tx_buff_[3] = (uint16_t)type;
        pending_tx_buff_[5] = 2; // 两个uint16寄存器

        uint16_t crc = Modbus_n::crc16(pending_tx_buff_, 6);
        pending_tx_buff_[6] = crc >> 8;
        pending_tx_buff_[7] = crc;
        pending_tx_size_ = 8;
        pending_cmd_ = PendingCmd_e::READ_DATA;
    }

    void Dfm113d_c::_build_read_setting_frame(uint16_t addr)
    {
        memset(pending_tx_buff_, 0, DFM113D_BUFF_MAX);
        pending_tx_buff_[0] = driver_addr_;
        pending_tx_buff_[1] = MODBUS_CODE_READ_PUBLIC;
        addr *= 2; // 寄存器地址转化为半字地址
        pending_tx_buff_[2] = addr >> 8;
        pending_tx_buff_[3] = addr;
        pending_tx_buff_[5] = 2; // 两个uint16寄存器

        uint16_t crc = Modbus_n::crc16(pending_tx_buff_, 6);
        pending_tx_buff_[6] = crc >> 8;
        pending_tx_buff_[7] = crc;
        pending_tx_size_ = 8;
        pending_cmd_ = PendingCmd_e::READ_SETTING;
    }

    void Dfm113d_c::_build_write_setting_frame(uint16_t addr, float setting)
    {
        memset(pending_tx_buff_, 0, DFM113D_BUFF_MAX);
        pending_tx_buff_[0] = driver_addr_;
        pending_tx_buff_[1] = MODBUS_CODE_WRITE;
        addr *= 2; // 寄存器地址转化为半字地址
        pending_tx_buff_[2] = addr >> 8;
        pending_tx_buff_[3] = addr;
        pending_tx_buff_[5] = 2; // 两个寄存器
        pending_tx_buff_[6] = 4;
        _memrevcpy(pending_tx_buff_ + 7, &setting, 4);

        uint16_t crc = Modbus_n::crc16(pending_tx_buff_, 11);
        pending_tx_buff_[11] = crc >> 8;
        pending_tx_buff_[12] = crc;
        pending_tx_size_ = 13;
        pending_cmd_ = PendingCmd_e::WRITE_SETTING;
    }

    void Dfm113d_c::_build_unlock_frame(uint8_t* pdata, uint16_t* psize)
    {
        memset(pdata, 0, DFM113D_BUFF_MAX);
        pdata[0] = driver_addr_;
        pdata[1] = MODBUS_CODE_WRITE;
        pdata[3] = DFM113D_ADDR_99OA * 2;
        pdata[5] = 2; // 两个寄存器
        pdata[6] = 4;
        _memrevcpy(pdata + 7, &password_, 4);

        uint16_t crc = Modbus_n::crc16(pdata, 11);
        pdata[11] = crc >> 8;
        pdata[12] = crc;
        *psize = 13;
    }

    // 请求读数据
    WorkingState_e Dfm113d_c::Fill_ReadData(ReadDataType_e type)
    {
        CheckLost();
        if (_is_busy())
        {
            // 上一次填充的数据还未发出或仍在等待应答，本次不覆盖
            return WorkingState_e::Busy;
        }

        // Fill接口只负责缓存待发帧，真正发送由AllUpdate统一调度
        _build_read_data_frame(type);
        return WorkingState_e::Normal;
    }

    // 请求读设置
    WorkingState_e Dfm113d_c::Fill_ReadSetting(uint16_t addr)
    {
        CheckLost();
        if (_is_busy())
        {
            // 上一次填充的数据还未发出或仍在等待应答，本次不覆盖
            return WorkingState_e::Busy;
        }

        // Fill接口只负责缓存待发帧，真正发送由AllUpdate统一调度
        _build_read_setting_frame(addr);
        return WorkingState_e::Normal;
    }

    // 写设置
    WorkingState_e Dfm113d_c::Fill_WriteSetting(uint16_t addr, float setting)
    {
        CheckLost();
        if (_is_busy())
        {
            // 上一次填充的数据还未发出或仍在等待应答，本次不覆盖
            return WorkingState_e::Busy;
        }

        // 写配置先缓存目标写帧，解锁动作由AllUpdate在发送阶段自动补齐
        _build_write_setting_frame(addr, setting);
        return WorkingState_e::Normal;
    }

    void AllUpdate()
    {
        static Dfm113d_c *temp = dfm113d_instance_head;
        if (dfm113d_instance_head == nullptr)
        {
            temp = nullptr;
            return;
        }

        if (temp == nullptr)
        {
            temp = dfm113d_instance_head;
        }

        Dfm113d_c* current = temp;
        // 每次调用只推进链表中的一个实例，避免单次轮询发送过多数据
        temp = (temp->next_device_ != nullptr) ? temp->next_device_ : dfm113d_instance_head;

        current->CheckLost();
        if (current->_is_wait_ack())
        {
            return;
        }

        if (current->_has_pending_frame() == 0)
        {
            return;
        }

        if (_is_usart_send_ready(current->usart_handle_) == 0)
        {
            return;
        }

        if (current->pending_cmd_ == PendingCmd_e::WRITE_SETTING && current->is_lock_ != 0)
        {
            uint16_t unlock_size = 0;
            // 写配置前若设备仍处于锁定态，则优先发送密码解锁帧
            current->_build_unlock_frame(current->tx_buff_, &unlock_size);
            current->is_lock_ = 1;
            current->_send_frame(current->tx_buff_, unlock_size, TxStage_e::WAIT_UNLOCK_ACK);
            return;
        }

        // 读命令或已解锁的写命令都从待发缓存拷贝到发送缓冲区后正式发出
        memcpy(current->tx_buff_, current->pending_tx_buff_, current->pending_tx_size_);
        current->_send_frame(current->tx_buff_, current->pending_tx_size_, TxStage_e::WAIT_CMD_ACK);
        if (current->pending_cmd_ != PendingCmd_e::WRITE_SETTING)
        {
            // 读命令发出后只等待回包，不再需要保留待发缓存
            current->pending_cmd_ = PendingCmd_e::NONE;
            current->pending_tx_size_ = 0;
        }
    }
}
