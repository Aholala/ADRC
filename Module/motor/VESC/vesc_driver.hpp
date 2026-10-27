#ifndef __VESC_HPP
#define __VESC_HPP

#ifdef STM32H723xx
#include "bsp_fdcan.hpp"
#else
#include "bsp_can.hpp"
#endif // USE_H7_if_or_not
#include "ecf_configList.h"
#include "safe_task.hpp"

#ifndef VESC_DRIVER_NUM
#define VESC_DRIVER_NUM 4 // VESC设备数量
#endif // !VESC_DRIVER_NUM

#ifdef USE_DEBUG_LIGHT
#include "debug_light.hpp"
#endif // USE_DEBUG_LIGHT

namespace Motor_n
{
    // VESC发送数据包ID
    typedef enum class Vesc_TxID_e : uint32_t
    {
        SET_DUTY = 0,
        SET_CURRENT,
        SET_CURRENT_BRAKE,
        SET_RPM,
        SET_POS,
        SET_CURRENT_REL = 10,
        SET_CURRENT_BRAKE_REL,
        SET_CURRENT_HANDBRAKE,
        SET_CURRENT_HANDBRAKE_REL,
        MAKE_ENUM_32_BITS = 0xFFFFFFFF,
    } Vesc_TxID_e;

    // VESC的回传扩展帧HEADER包括：消息命令+控制器ID(类似于DJI电调的控制帧头+ID结构)，较低的8字节表示VESC的Controller_ID,其余高字节则表示不同消息类型的索引值
    typedef enum Vesc_RxID_e
    {
        STATUES_1 = 9,  // 0x09——rpm(4 bytes)、电流*10.0(2 bytes)、duty*1000.0(2 bytes)
        STATUES_2 = 14, // 0x0E——amp_hours*10000.0(4 bytes)、amp_hours_charged*10000.0(4 bytes)
        STATUES_3 = 15, // 0x0F——watt_hours*10000.0(4 bytes)、watt_hours_charged*10000.0(4 bytes)
        STATUES_4 = 16, // 0x10——temp_fet*10.0(2 bytes)、temp_motor*10.0(2 bytes)、current_in*10.0(2 bytes)、pid_pos_now*50.0(2 bytes)
        STATUES_5 = 27, // 0x1B——tacho_value(4 bytes)、v_in*10.0(2 bytes)、rec 0(2 bytes)
        STATUES_6 = 58  // 0x3A——ADC1(2 bytes)、ADC2(2 bytes)、ADC3(2 bytes)、PPM(2 bytes)
    } Vesc_RxID_e;

    // 用户配置启用哪些状态的接收，用位或组合
    typedef enum Vesc_RxEnable_e : uint8_t
    {
        RX_ENABLE_NONE    = 0b00000000,
        RX_ENABLE_STATUS1 = 0b00000001,  // erpm、电流、占空比
        RX_ENABLE_STATUS2 = 0b00000010,  // 电流累积、充电电流累积
        RX_ENABLE_STATUS3 = 0b00000100,  // 功率累积、充电功率累积
        RX_ENABLE_STATUS4 = 0b00001000,  // FET温度、电机温度、输入电流、PID位置
        RX_ENABLE_STATUS5 = 0b00010000,  // 转速计数值、输入电压
        RX_ENABLE_STATUS6 = 0b00100000,  // ADC1、ADC2、ADC3、PPM
        RX_ENABLE_ALL     = 0b00111111,
    } Vesc_RxEnable_e;

    typedef struct
    {
        uint8_t id;                 // VESC控制器ID
        bool is_online;             // 在线标志
        int32_t erpm;           // 电机转速,单位erpm
        int16_t current;        // 电机电流*10,单位0.1A
        int16_t duty;           // 占空比*1000,单位0.001
        uint32_t amp_hours;         // 电流累积*10000,单位0.1Ah
        uint32_t amp_hours_charged; // 充电电流累积*10000,单位0.1Ah
        uint32_t watt_hours;         // 功率累积*10000,单位0.1Wh
        uint32_t watt_hours_charged; // 充电功率累积*10000,单位0.1Wh
        uint16_t temp_fet;    // FET温度*10,单位0.1℃
        uint16_t temp_motor;  // 电机温度*10,单位0.1℃
        uint16_t current_in;  // 输入电流*10,单位0.1A
        uint16_t pid_pos_now; // 当前PID位置*50,单位0.02°
        uint32_t tacho_value; // 转速计数值
        uint16_t v_in;        // 输入电压*10,单位0.1V
        uint16_t rec_0;       // 保留字段
        uint16_t adc1; // ADC1值
        uint16_t adc2; // ADC2值
        uint16_t adc3; // ADC3值
        uint16_t ppm;  // PPM值
    } Vesc_DataTypeDef_t;

    class Vesc_c
    {
    public:
        /**
         * @brief VESC电调构造函数
         * @param use_can           CAN总线句柄（&hcan1 / &hcan2 或 &hfdcan1 / &hfdcan2）
         * @param driver_id         VESC控制器ID，范围0~255
         * @param rx_enable         启用的状态接收掩码，用 RxEnable_e 位或组合，默认全部开启
         * @param disconnect_ms     掉线检测阈值(ms)，收到任意启用状态的数据即刷新在线，
         *                          超过此时间未收到则判定掉线，默认 200ms
         * @note  会根据 rx_enable 为每个启用的状态创建一个仅接收的 Can_c 实例，
         *        并在全局 CAN 分发器中注册对应的回调；
         *        SafeTask 会自动注册到全局链表，由 FreeRTOS Safe_Task 任务统一巡检
         */
#ifdef STM32H723xx
        Vesc_c(FDCAN_HandleTypeDef *use_can, uint8_t driver_id = 0,
               uint8_t rx_enable = RX_ENABLE_ALL, uint64_t disconnect_ms = 500);
#else
        Vesc_c(CAN_HandleTypeDef *use_can, uint8_t driver_id = 0,
               uint8_t rx_enable = RX_ENABLE_ALL, uint64_t disconnect_ms = 500);
#endif // USE_H7_if_or_not

        Vesc_DataTypeDef_t data_ = {};
        [[nodiscard]] const Vesc_DataTypeDef_t& data() const { return data_; }
        [[nodiscard]] bool is_online() const { return data_.is_online; }

        void Online() { data_.is_online = true; vesc_safe_task_.Online(); }

        void Set_Duty(float duty);
        void Set_Current(float current);
        void Set_Current_OffDelay(float current, float off_delay);
        void Set_Current_Brake(float current);
        void Set_ERPM(float erpm);
        void Set_Pos(float pos);
        void Set_Current_Rel(float current_rel);
        void Set_Current_Rel_OffDelay(float current_rel, float off_delay);
        void Set_Current_Brake_Rel(float current_rel);
        void Set_HandBrake(float current);
        void Set_HandBrake_Rel(float current_rel);

    private:
#ifdef USE_DEBUG_LIGHT
        Light_n::Debug_c* online_light_ = nullptr;
#endif // USE_DEBUG_LIGHT

        static constexpr uint8_t kRxStatusCount = 6;

        uint8_t driver_id_ = 0;
        uint8_t rx_enable_ = 0;
        uint64_t disconnect_ms_ = 500;

        SafeTask_c vesc_safe_task_;

#ifdef STM32H723xx
        BSP_n::Fdcan_c *tx_can_ = nullptr;      // 仅发送
        BSP_n::Fdcan_c *rx_can_[kRxStatusCount] = {nullptr}; // 按需创建的接收实例
        static void RxCallback(BSP_n::Fdcan_c *can);
#else
        BSP_n::Can_c *tx_can_ = nullptr;
        BSP_n::Can_c *rx_can_[kRxStatusCount] = {nullptr};
        static void RxCallback(BSP_n::Can_c *can);
#endif // USE_H7_if_or_not

        static Vesc_c *vesc_instance_[VESC_DRIVER_NUM];
        static uint8_t vesc_idx_;

        // 查找表：位索引(0~5) ↔ Status类型码(9/14/15/16/27/58)
        static constexpr uint8_t StatusTypeToIndex(uint32_t status_type);
        static constexpr uint32_t IndexToStatusType(uint8_t index);
    };
}

#endif //! __VESC_HPP
