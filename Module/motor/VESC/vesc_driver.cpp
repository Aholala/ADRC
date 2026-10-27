/************************** Dongguan-University of Technology -ACE**************************
 * @file  vesc_driver.cpp
 * @brief VESC类电调CAN通讯库，目前使用在N630电调
 * @author L_Zero
 * ==============================================================================
 * @verbatim
 * * 配合vesc_tool助手使用
 * @attention
 * * 此类电调大部分用于无感FOC场景，请勿在电调未校准电机内参时尝试驱动电机
 * * 关于反馈数据，需要在vesc_tool打开对应状态发送功能
 * * 关于电机转速，VESC电调使用的都是电气转速ERPM，请确保你知道手中电机ERPM和RPM的比例关系
 *
 * @version
 * * v26.0.1.0 2026/6/12
 *      新增反馈数据读取，因为一个can实例只能设一个rxid所以接收开了6个can实例，
 *      不过如果能改bsp_can底层的话可以修改FIFO过滤器的ID Mask做到单过滤器接收6种ID，不过目前不打算底层
 * * v26.0.1.1 2026/6/17
 *      区分erpm和rpm，补充文档
 ************************** Dongguan-University of Technology -ACE***************************/

#include <stdint.h>
#include <string.h>
#include <cstdio>
#include "vesc_driver.hpp"
#include "ecf_configList.h"
#include "safe_task.hpp"

namespace Motor_n
{
    /* ----------------------- 静态成员初始化 -------------------------- */
    Vesc_c *Vesc_c::vesc_instance_[VESC_DRIVER_NUM] = {nullptr};
    uint8_t Vesc_c::vesc_idx_ = 0;

    /* ----------------------- 位索引 ↔ 状态类型 查表 -------------------------- */
    constexpr uint8_t Vesc_c::StatusTypeToIndex(uint32_t status_type)
    {
        switch (status_type)
        {
            case Vesc_RxID_e::STATUES_1: return 0;
            case Vesc_RxID_e::STATUES_2: return 1;
            case Vesc_RxID_e::STATUES_3: return 2;
            case Vesc_RxID_e::STATUES_4: return 3;
            case Vesc_RxID_e::STATUES_5: return 4;
            case Vesc_RxID_e::STATUES_6: return 5;
            default:                return 0xFF;
        }
    }

    constexpr uint32_t Vesc_c::IndexToStatusType(uint8_t index)
    {
        constexpr uint32_t table[kRxStatusCount] = {
                Vesc_RxID_e::STATUES_1,
                Vesc_RxID_e::STATUES_2,
                Vesc_RxID_e::STATUES_3,
                Vesc_RxID_e::STATUES_4,
                Vesc_RxID_e::STATUES_5,
                Vesc_RxID_e::STATUES_6,
        };
        return (index < kRxStatusCount) ? table[index] : 0;
    }

    /* ----------------------- 辅助发送函数 -------------------------- */

#ifdef STM32H723xx
    void can_transmit_eid(BSP_n::Fdcan_c *can, uint32_t id, const uint8_t *data, uint8_t len)
#else
    void can_transmit_eid(BSP_n::Can_c *can, uint32_t id, const uint8_t *data, uint8_t len)
#endif // USE_H7_if_or_not
    {
        if (data == NULL || len == 0 || len > 8) return;

        can->SetDLC(len);
        can->SetExtTxId(id);
        memcpy(can->tx_buff_, (uint8_t *)data, len);
        can->Transmit(0.01f);
    }

    void buffer_append_int16(uint8_t *buffer, int16_t number, int32_t *index)
    {
        buffer[(*index)++] = number >> 8;
        buffer[(*index)++] = number;
    }

    void buffer_append_int32(uint8_t *buffer, int32_t number, int32_t *index)
    {
        buffer[(*index)++] = number >> 24;
        buffer[(*index)++] = number >> 16;
        buffer[(*index)++] = number >> 8;
        buffer[(*index)++] = number;
    }

    void buffer_append_float16(uint8_t *buffer, float number, float scale, int32_t *index)
    {
        buffer_append_int16(buffer, (int16_t)(number * scale), index);
    }

    void buffer_append_float32(uint8_t *buffer, float number, float scale, int32_t *index)
    {
        buffer_append_int32(buffer, (int32_t)(number * scale), index);
    }

    /* ----------------------- 统一接收回调（由 CAN 分发器调用） -------------------------- */

#ifdef STM32H723xx
    void Vesc_c::RxCallback(BSP_n::Fdcan_c *can)
#else
    void Vesc_c::RxCallback(BSP_n::Can_c *can)
#endif // USE_H7_if_or_not
    {
        // 从扩展帧ID中提取 driver_id（低8位）和 status_type（高8位以上）
        uint8_t  driver_id   = can->rx_id_ & 0xFF;
        uint32_t status_type = (can->rx_id_ >> 8) & 0xFF;

        // 在静态注册表中查找对应 driver_id 的 Vesc_c 实例
        Vesc_c *self = nullptr;
        for (uint8_t i = 0; i < vesc_idx_; i++)
        {
            if (vesc_instance_[i] != nullptr &&
                vesc_instance_[i]->data_.id == driver_id)
            {
                self = vesc_instance_[i];
                break;
            }
        }
        if (!self) return;

        // 刷新在线标志并喂狗 SafeTask
        self->data_.is_online = true;
        self->vesc_safe_task_.Online();

        // 根据状态类型解析数据
        switch (status_type)
        {
            case Vesc_RxID_e::STATUES_1:
                self->data_.erpm =
                    (int32_t)(can->rx_buff_[0] << 24 | can->rx_buff_[1] << 16 |
                              can->rx_buff_[2] << 8 | can->rx_buff_[3]);
                self->data_.current =
                    (int16_t)(can->rx_buff_[4] << 8 | can->rx_buff_[5]);
                self->data_.duty =
                    (int16_t)(can->rx_buff_[6] << 8 | can->rx_buff_[7]);
                break;

            case Vesc_RxID_e::STATUES_2:
                self->data_.amp_hours =
                    (uint32_t)(can->rx_buff_[0] << 24 | can->rx_buff_[1] << 16 |
                               can->rx_buff_[2] << 8 | can->rx_buff_[3]);
                self->data_.amp_hours_charged =
                    (uint32_t)(can->rx_buff_[4] << 24 | can->rx_buff_[5] << 16 |
                               can->rx_buff_[6] << 8 | can->rx_buff_[7]);
                break;

            case Vesc_RxID_e::STATUES_3:
                self->data_.watt_hours =
                    (uint32_t)(can->rx_buff_[0] << 24 | can->rx_buff_[1] << 16 |
                               can->rx_buff_[2] << 8 | can->rx_buff_[3]);
                self->data_.watt_hours_charged =
                    (uint32_t)(can->rx_buff_[4] << 24 | can->rx_buff_[5] << 16 |
                               can->rx_buff_[6] << 8 | can->rx_buff_[7]);
                break;

            case Vesc_RxID_e::STATUES_4:
                self->data_.temp_fet =
                    (uint16_t)(can->rx_buff_[0] << 8 | can->rx_buff_[1]);
                self->data_.temp_motor =
                    (uint16_t)(can->rx_buff_[2] << 8 | can->rx_buff_[3]);
                self->data_.current_in =
                    (uint16_t)(can->rx_buff_[4] << 8 | can->rx_buff_[5]);
                self->data_.pid_pos_now =
                    (uint16_t)(can->rx_buff_[6] << 8 | can->rx_buff_[7]);
                break;

            case Vesc_RxID_e::STATUES_5:
                self->data_.tacho_value =
                    (uint32_t)(can->rx_buff_[0] << 24 | can->rx_buff_[1] << 16 |
                               can->rx_buff_[2] << 8 | can->rx_buff_[3]);
                self->data_.v_in =
                    (uint16_t)(can->rx_buff_[4] << 8 | can->rx_buff_[5]);
                self->data_.rec_0 =
                    (uint16_t)(can->rx_buff_[6] << 8 | can->rx_buff_[7]);
                break;

            case Vesc_RxID_e::STATUES_6:
                self->data_.adc1 =
                    (uint16_t)(can->rx_buff_[0] << 8 | can->rx_buff_[1]);
                self->data_.adc2 =
                    (uint16_t)(can->rx_buff_[2] << 8 | can->rx_buff_[3]);
                self->data_.adc3 =
                    (uint16_t)(can->rx_buff_[4] << 8 | can->rx_buff_[5]);
                self->data_.ppm =
                    (uint16_t)(can->rx_buff_[6] << 8 | can->rx_buff_[7]);
                break;

            default:
                break;
        }
    }

    /* ----------------------- 构造函数 -------------------------- */

#ifdef STM32H723xx
    Vesc_c::Vesc_c(FDCAN_HandleTypeDef *use_can, uint8_t driver_id,
                   uint8_t rx_enable, uint64_t disconnect_ms)
        : driver_id_(driver_id), rx_enable_(rx_enable), disconnect_ms_(disconnect_ms),
          vesc_safe_task_("VescSafe", disconnect_ms,
                          [this]() { uint8_t id = this->data_.id; memset(&this->data_, 0, sizeof(this->data_)); this->data_.id = id; }, nullptr)
    {
        data_.id = driver_id;

        // 1. 创建仅用于发送的实例（不注册回调）
        {
            BSP_n::CanInitConfig_s tx_cfg = {
                .fdcan_handle          = use_can,
                .tx_id                 = 0,
                .rx_id                 = 0,
                .fdcan_module_callback = nullptr,
                .SAND_IDE              = FDCAN_EXTENDED_ID,
            };
            tx_can_ = new BSP_n::Fdcan_c(tx_cfg);
        }

        // 2. 遍历6个状态位，为启用的状态创建接收实例
        for (uint8_t i = 0; i < kRxStatusCount; i++)
        {
            if (rx_enable_ & (1 << i))
            {
                uint32_t full_rx_id = (IndexToStatusType(i) << 8) | driver_id_;

                BSP_n::CanInitConfig_s rx_cfg = {
                    .fdcan_handle          = use_can,
                    .tx_id                 = 0,  // 只收不发
                    .rx_id                 = full_rx_id,
                    .fdcan_module_callback = Vesc_c::RxCallback,
                    .SAND_IDE              = FDCAN_EXTENDED_ID,
                };
                rx_can_[i] = new BSP_n::Fdcan_c(rx_cfg);
            }
        }

        // 3. 注册到静态实例表
        if (vesc_idx_ < VESC_DRIVER_NUM)
        {
            vesc_instance_[vesc_idx_++] = this;
        }
        else
        {
            while (1) {} // 超过最大VESC实例数
        }
    }
#else
    Vesc_c::Vesc_c(CAN_HandleTypeDef *use_can, uint8_t driver_id,
                   uint8_t rx_enable, uint64_t disconnect_ms)
        : driver_id_(driver_id), rx_enable_(rx_enable), disconnect_ms_(disconnect_ms),
          vesc_safe_task_("VescSafe", disconnect_ms,
                          [this]() { uint8_t id = this->data_.id; memset(&this->data_, 0, sizeof(this->data_)); this->data_.id = id; }, nullptr)
    {
        data_.id = driver_id;

        // 1. 创建仅用于发送的实例（不注册回调）
        {
            BSP_n::CanInitConfig_s tx_cfg = {
                .can_handle          = use_can,
                .tx_id               = 0,
                .rx_id               = 0,
                .can_module_callback = nullptr,
                .SAND_IDE            = CAN_ID_EXT,
            };
            tx_can_ = new BSP_n::Can_c(tx_cfg);
        }

        // 2. 遍历6个状态位，为启用的状态创建接收实例
        for (uint8_t i = 0; i < kRxStatusCount; i++)
        {
            if (rx_enable_ & (1 << i))
            {
                uint32_t full_rx_id = (IndexToStatusType(i) << 8) | driver_id_;

                BSP_n::CanInitConfig_s rx_cfg = {
                    .can_handle          = use_can,
                    .tx_id               = 0,  // 只收不发
                    .rx_id               = full_rx_id,
                    .can_module_callback = Vesc_c::RxCallback,
                    .SAND_IDE            = CAN_ID_EXT,
                };
                rx_can_[i] = new BSP_n::Can_c(rx_cfg);
            }
        }

        // 3. 注册到静态实例表
        if (vesc_idx_ < VESC_DRIVER_NUM)
        {
            vesc_instance_[vesc_idx_++] = this;
        }
        else
        {
            while (1) {} // 超过最大VESC实例数
        }

        // 注册灯效（如果开灯效的话）
#ifdef USE_DEBUG_LIGHT // 注册灯效触发类
        Light_n::Effect_t a_light;
        Light_n::Effect_t lost_light = Light_n::CANLOST_B_EFFECT;
        lost_light.SetBlink(driver_id, lost_light.blink_cycle);
        std::snprintf(lost_light.name, sizeof(lost_light.name), "canL_%u", driver_id);
#ifdef USE_CAN1
        if(use_can == &hcan1)
        {
            a_light = Light_n::CAN1_A_EFFECT;
        }
#endif
#ifdef USE_CAN2
        if(use_can == &hcan2)
        {
            a_light = Light_n::CAN2_A_EFFECT;
        }
#endif
#ifdef USE_FDCAN1
        if(use_can == &hfdcan1)
                {
                    a_light = Light_n::CAN1_A_EFFECT;
                }
#endif
#ifdef USE_FDCAN2
        if(use_can == &hfdcan2)
                {
                    a_light = Light_n::CAN2_A_EFFECT;
                }
#endif
#ifdef USE_FDCAN3
        if(use_can == &hfdcan3)
                {
                    a_light = Light_n::CAN3_A_EFFECT;
                }
#endif
        // 同一组A/B灯效持续时间保持一致，避免两颗灯播放不同步
        a_light.duration = lost_light.duration;
        Light_n::Effect_t lost_effect[2] = {
                a_light,
                lost_light
        };
        online_light_ = new Light_n::Debug_c(lost_effect, &data_.is_online);
        online_light_->Enable();
#endif
    }
#endif // USE_H7_if_or_not

    /* ----------------------- 发送接口 -------------------------- */

    void Vesc_c::Set_Duty(float duty)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_int32(buffer, (int32_t)(duty * 100000.0), &send_index);
        can_transmit_eid(tx_can_, data_.id | ((uint32_t)Vesc_TxID_e::SET_DUTY << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_Current(float current)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_int32(buffer, (int32_t)(current * 1000.0), &send_index);
        can_transmit_eid(tx_can_, data_.id | ((uint32_t)Vesc_TxID_e::SET_CURRENT << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_Current_OffDelay(float current, float off_delay)
    {
        int32_t send_index = 0;
        uint8_t buffer[6];
        buffer_append_int32(buffer, (int32_t)(current * 1000.0), &send_index);
        buffer_append_float16(buffer, off_delay, 1e3, &send_index);
        can_transmit_eid(tx_can_, data_.id | ((uint32_t)Vesc_TxID_e::SET_CURRENT << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_Current_Brake(float current)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_int32(buffer, (int32_t)(current * 1000.0), &send_index);
        can_transmit_eid(tx_can_,
                         data_.id | ((uint32_t)Vesc_TxID_e::SET_CURRENT_BRAKE << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_ERPM(float erpm)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_int32(buffer, (int32_t)erpm, &send_index);
        can_transmit_eid(tx_can_, data_.id | ((uint32_t)Vesc_TxID_e::SET_RPM << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_Pos(float pos)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_int32(buffer, (int32_t)(pos * 1000000.0), &send_index);
        can_transmit_eid(tx_can_, data_.id | ((uint32_t)Vesc_TxID_e::SET_POS << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_Current_Rel(float current_rel)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_float32(buffer, current_rel, 1e5, &send_index);
        can_transmit_eid(tx_can_,
                         data_.id | ((uint32_t)Vesc_TxID_e::SET_CURRENT_REL << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_Current_Rel_OffDelay(float current_rel, float off_delay)
    {
        int32_t send_index = 0;
        uint8_t buffer[6];
        buffer_append_float32(buffer, current_rel, 1e5, &send_index);
        buffer_append_float16(buffer, off_delay, 1e3, &send_index);
        can_transmit_eid(tx_can_,
                         data_.id | ((uint32_t)Vesc_TxID_e::SET_CURRENT_REL << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_Current_Brake_Rel(float current_rel)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_float32(buffer, current_rel, 1e5, &send_index);
        can_transmit_eid(tx_can_,
                         data_.id | ((uint32_t)Vesc_TxID_e::SET_CURRENT_BRAKE_REL << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_HandBrake(float current)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_float32(buffer, current, 1e3, &send_index);
        can_transmit_eid(tx_can_,
                         data_.id | ((uint32_t)Vesc_TxID_e::SET_CURRENT_HANDBRAKE << 8),
                         buffer, send_index);
    }

    void Vesc_c::Set_HandBrake_Rel(float current_rel)
    {
        int32_t send_index = 0;
        uint8_t buffer[4];
        buffer_append_float32(buffer, current_rel, 1e5, &send_index);
        can_transmit_eid(tx_can_,
                         data_.id | ((uint32_t)Vesc_TxID_e::SET_CURRENT_HANDBRAKE_REL << 8),
                         buffer, send_index);
    }
}
