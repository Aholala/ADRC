/*************************** Dongguan-University of Technology -ACE**************************
 * @file    tfmini_plus.cpp
 * @author  OrangeHan
 * @version v26.0.0.0
 * @date    2026-04-26
 * @brief   Benewake TFmini Plus ToF module
 * @bug 设置类用户自定义发送相关接口不可用
 ******************************************************************************
 * @attention
 *      Depends:
 *          USART
 *          safe_task
 *          tfmini_plus_config.hpp
 ************************** Dongguan-University of Technology -ACE***************************/
#include "tfmini_plus.hpp"
#if defined(USE_JLINK_RTT)
#include "bsp_jlink_rtt.hpp"
#endif

namespace Tof_n
{

TFminiPlus_c TFminiPlus_c::tfmini_plus_instance_ = TFminiPlus_c();

#if defined(USE_JLINK_RTT) && defined(ECF_RTT_LOG_ENABLE_TOF) && (ECF_RTT_LOG_ENABLE_TOF)
#define TOF_LOG_I(fmt, ...) LOG_RTT_I("[TOF][LINK] " fmt __VA_OPT__(,) __VA_ARGS__)
#define TOF_LOG_W(fmt, ...) LOG_RTT_W("[TOF][FRAME] " fmt __VA_OPT__(,) __VA_ARGS__)
#define TOF_LOG_E(fmt, ...) LOG_RTT_E("[TOF][LINK] " fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define TOF_LOG_I(fmt, ...) ((void)0)
#define TOF_LOG_W(fmt, ...) ((void)0)
#define TOF_LOG_E(fmt, ...) ((void)0)
#endif

void TFminiPlus_c::TFminiPlusInit(UART_HandleTypeDef *handle, const char *name)
{
    if (handle == nullptr || tof_usart != nullptr)
    {
        return;
    }

    if (name != nullptr)
    {
        strncpy(name_, name, sizeof(name_) - 1U);
        name_[sizeof(name_) - 1U] = '\0';
    }

    if (handle->hdmarx == nullptr)
    {
        TOF_LOG_E("%s init failed: uart rx dma is null", name_);
        return;
    }

    BSP_n::USART_c::Config tof_uart_param = {
            .Handle = handle,
            .RxBuffSize = TFMINI_PLUS_RX_BUFFER_NUM,
            .RxType = BSP_n::USART_c::RxType_t::DMA_IDLE,
            .TxType = BSP_n::USART_c::TxType_t::BLOCK,
            .pTxCallback = nullptr,
            .pRxCallback = [this](const uint8_t *pdata, uint16_t psize) {
                if (pdata != nullptr && psize != 0U)
                {
                    this->DataProcess(pdata, psize);
                }
            },
            .expected_data_length = TFMINI_PLUS_FRAME_LENGTH,
            .pRxBuffer = rx_buffer_,
            .pSecRxBuffer = nullptr};
    tof_usart = new BSP_n::USART_c(tof_uart_param);
    tof_safe = new SafeTask_c(name_, TFMINI_PLUS_DISCONNECT_TIMEOUT_MS,
                              [this]() { this->Disonline(); },
                              [this]() { this->Online(); });
    tof_usart->start_receive();
    TOF_LOG_I("%s init ok rx_buf=%u timeout_ms=%u",
              name_,
              static_cast<unsigned>(TFMINI_PLUS_RX_BUFFER_NUM),
              static_cast<unsigned>(TFMINI_PLUS_DISCONNECT_TIMEOUT_MS));
}

void TFminiPlus_c::Online()
{
    if (disconnected_latched_)
    {
        TOF_LOG_I("%s recovered", name_);
        disconnected_latched_ = false;
    }
    is_online_ = 1U;
}

void TFminiPlus_c::Disonline()
{
    if (!disconnected_latched_)
    {
        TOF_LOG_E("%s disconnected timeout=%ums",
                  name_,
                  static_cast<unsigned>(TFMINI_PLUS_DISCONNECT_TIMEOUT_MS));
        disconnected_latched_ = true;
    }
    Clear();
}

void TFminiPlus_c::Clear()
{
    is_online_ = 0U;
    frame_index_ = 0U;
    frame_rate_window_tick_ = 0U;
    frame_rate_window_count_ = 0U;
    frame_rate_window_started_ = 0U;
    distance_lpf_started_ = 0U;
    memset(frame_buffer_, 0, sizeof(frame_buffer_));
    memset(&data_, 0, sizeof(data_));
}

void TFminiPlus_c::DataProcess(const uint8_t *pdata, uint16_t psize)
{
    for (uint16_t i = 0; i < psize; i++)
    {
        if (PushByte(pdata[i]) && tof_safe != nullptr)
        {
            tof_safe->Online();
        }
    }
}

bool TFminiPlus_c::PushByte(uint8_t data)
{
    if (frame_index_ == 0U)
    {
        if (data != TFMINI_PLUS_FRAME_HEADER)
        {
            data_.frame_error_count++;
            return false;
        }
        frame_buffer_[frame_index_++] = data;
        return false;
    }

    if (frame_index_ == 1U)
    {
        if (data != TFMINI_PLUS_FRAME_HEADER)
        {
            frame_index_ = (data == TFMINI_PLUS_FRAME_HEADER) ? 1U : 0U;
            data_.frame_error_count++;
            return false;
        }
        frame_buffer_[frame_index_++] = data;
        return false;
    }

    frame_buffer_[frame_index_++] = data;
    if (frame_index_ < TFMINI_PLUS_FRAME_LENGTH)
    {
        return false;
    }

    frame_index_ = 0U;
    return ParseFrame(frame_buffer_);
}

bool TFminiPlus_c::ParseFrame(const uint8_t *frame)
{
    if (frame == nullptr)
    {
        return false;
    }

    if (BuildChecksum(frame, TFMINI_PLUS_FRAME_LENGTH - 1U) != frame[8])
    {
        data_.checksum_error_count++;
        data_.frame_valid = 0U;
        if (!frame_fault_latched_)
        {
            TOF_LOG_W("%s checksum failed cnt=%lu",
                      name_,
                      static_cast<unsigned long>(data_.checksum_error_count));
            frame_fault_latched_ = true;
        }
        return false;
    }

    const uint16_t distance = static_cast<uint16_t>(frame[2]) |
                              (static_cast<uint16_t>(frame[3]) << 8);
    const uint16_t strength = static_cast<uint16_t>(frame[4]) |
                              (static_cast<uint16_t>(frame[5]) << 8);
    const int16_t temp_raw = static_cast<int16_t>(static_cast<uint16_t>(frame[6]) |
                                                 (static_cast<uint16_t>(frame[7]) << 8));

    data_.raw_distance_cm = distance;
    data_.strength = strength;
    data_.temp_raw = temp_raw;
    data_.temperature_c = static_cast<float>(temp_raw) / 8.0f - 256.0f;
    data_.frame_valid = 1U;
    data_.distance_valid = (distance != 0U &&
                            strength > TFMINI_PLUS_STRENGTH_LOW_LIMIT &&
                            strength != TFMINI_PLUS_INVALID_STRENGTH) ? 1U : 0U;
    if (data_.distance_valid != 0U)
    {
        UpdateDistanceLowPass(distance);
    }
    data_.update_count++;
    UpdateActualFrameRate();

    if (frame_fault_latched_)
    {
        TOF_LOG_I("%s frame recovered", name_);
        frame_fault_latched_ = false;
    }
    return true;
}

void TFminiPlus_c::UpdateDistanceLowPass(uint16_t raw_distance_cm)
{
    const float raw = static_cast<float>(raw_distance_cm);
    if (distance_lpf_started_ == 0U)
    {
        distance_lpf_started_ = 1U;
        data_.filtered_distance_cm = raw;
    }
    else
    {
        data_.filtered_distance_cm += distance_lpf_alpha_ * (raw - data_.filtered_distance_cm);
    }

    if (data_.filtered_distance_cm <= 0.0f)
    {
        data_.distance_cm = 0U;
    }
    else if (data_.filtered_distance_cm >= 65535.0f)
    {
        data_.distance_cm = 65535U;
    }
    else
    {
        data_.distance_cm = static_cast<uint16_t>(data_.filtered_distance_cm + 0.5f);
    }
}

void TFminiPlus_c::SetDistanceLowPassAlpha(float alpha)
{
    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }
    else if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }
    distance_lpf_alpha_ = alpha;
}

void TFminiPlus_c::UpdateActualFrameRate()
{
    const uint32_t now = HAL_GetTick();

    if (frame_rate_window_started_ == 0U)
    {
        frame_rate_window_started_ = 1U;
        frame_rate_window_tick_ = now;
        frame_rate_window_count_ = 1U;
        data_.actual_frame_rate_hz = 0.0f;
        return;
    }

    frame_rate_window_count_++;
    const uint32_t elapsed_ms = now - frame_rate_window_tick_;
    if (elapsed_ms >= 1000U)
    {
        data_.actual_frame_rate_hz =
                static_cast<float>(frame_rate_window_count_) * 1000.0f /
                static_cast<float>(elapsed_ms);
        frame_rate_window_tick_ = now;
        frame_rate_window_count_ = 0U;
    }
}

uint8_t TFminiPlus_c::BuildChecksum(const uint8_t *data, uint16_t len)
{
    uint16_t sum = 0U;
    for (uint16_t i = 0; i < len; i++)
    {
        sum = static_cast<uint16_t>(sum + data[i]);
    }
    return static_cast<uint8_t>(sum & 0xFFU);
}

void TFminiPlus_c::SendCommand(const uint8_t *cmd, uint16_t len)
{
    if (tof_usart == nullptr || cmd == nullptr || len == 0U)
    {
        return;
    }
    if (len >= 2U && cmd[0] == 0x5AU && cmd[1] != static_cast<uint8_t>(len))
    {
        TOF_LOG_E("%s command len mismatch frame_len=%u tx_len=%u",
                  name_,
                  static_cast<unsigned>(cmd[1]),
                  static_cast<unsigned>(len));
        return;
    }
    tof_usart->send(const_cast<uint8_t *>(cmd), len, 10U);
}

void TFminiPlus_c::GetFirmwareVersion()
{
    static const uint8_t cmd[4] = {0x5A, 0x04, 0x01, 0x5F};
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SystemReset()
{
    static const uint8_t cmd[4] = {0x5A, 0x04, 0x02, 0x60};
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::TriggerOnce()
{
    static const uint8_t cmd[4] = {0x5A, 0x04, 0x04, 0x62};
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetOutputMode(TFminiPlusOutputMode_e mode)
{
    uint8_t cmd[5] = {0x5A, 0x05, 0x05, static_cast<uint8_t>(mode), 0x00};
    cmd[4] = BuildChecksum(cmd, 4U);
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetOutputEnable(bool enable)
{
    uint8_t cmd[5] = {0x5A, 0x05, 0x07, static_cast<uint8_t>(enable ? 0x01U : 0x00U), 0x00};
    cmd[4] = BuildChecksum(cmd, 4U);
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetOutputRate(uint16_t hz)
{
    uint8_t cmd[6] = {0x5A, 0x06, 0x03, static_cast<uint8_t>(hz & 0xFFU),
                      static_cast<uint8_t>((hz >> 8) & 0xFFU), 0x00};
    cmd[5] = BuildChecksum(cmd, 5U);
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetBaudRate(uint32_t baudrate)
{
    uint8_t cmd[8] = {0x5A, 0x08, 0x06,
                      static_cast<uint8_t>(baudrate & 0xFFU),
                      static_cast<uint8_t>((baudrate >> 8) & 0xFFU),
                      static_cast<uint8_t>((baudrate >> 16) & 0xFFU),
                      static_cast<uint8_t>((baudrate >> 24) & 0xFFU),
                      0x00};
    cmd[7] = BuildChecksum(cmd, 7U);
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetCommunicationInterface(TFminiPlusInterface_e interface)
{
    uint8_t cmd[5] = {0x5A, 0x05, 0x0A, static_cast<uint8_t>(interface), 0x00};
    cmd[4] = BuildChecksum(cmd, 4U);
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetI2CAddress(uint8_t address)
{
    uint8_t cmd[5] = {0x5A, 0x05, 0x0B, address, 0x00};
    cmd[4] = BuildChecksum(cmd, 4U);
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::RequestDistanceCm()
{
    static const uint8_t cmd[5] = {0x5A, 0x05, 0x00, 0x01, 0x60};
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::RequestDistanceMm()
{
    static const uint8_t cmd[5] = {0x5A, 0x05, 0x00, 0x06, 0x65};
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetIOMode(TFminiPlusIOMode_e mode, uint16_t dist_cm, uint16_t zone_cm)
{
    uint8_t cmd[9] = {0x5A, 0x09, 0x3B,
                      static_cast<uint8_t>(mode),
                      static_cast<uint8_t>(dist_cm & 0xFFU),
                      static_cast<uint8_t>((dist_cm >> 8) & 0xFFU),
                      static_cast<uint8_t>(zone_cm & 0xFFU),
                      static_cast<uint8_t>((zone_cm >> 8) & 0xFFU),
                      0x00};
    cmd[8] = BuildChecksum(cmd, 8U);
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetStrengthLowThreshold(uint16_t strength_threshold, uint16_t low_strength_output_cm)
{
    uint8_t threshold = static_cast<uint8_t>(strength_threshold / 10U);
    uint8_t cmd[7] = {0x5A, 0x07, 0x22,
                      threshold,
                      static_cast<uint8_t>(low_strength_output_cm & 0xFFU),
                      static_cast<uint8_t>((low_strength_output_cm >> 8) & 0xFFU),
                      0x00};
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetLowPowerMode(uint8_t level)
{
    if (level > 0x0AU)
    {
        level = 0x0AU;
    }
    uint8_t cmd[6] = {0x5A, 0x06, 0x35, level, 0x00, 0x00};
    cmd[5] = BuildChecksum(cmd, 5U);
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SetOutputModeStandardCm()
{
    SetOutputMode(TFminiPlusOutputMode_e::StandardCm);
}

void TFminiPlus_c::SetOutputModeStringM()
{
    SetOutputMode(TFminiPlusOutputMode_e::StringM);
}

void TFminiPlus_c::SetOutputModeStandardMm()
{
    SetOutputMode(TFminiPlusOutputMode_e::StandardMm);
}

void TFminiPlus_c::FactoryReset()
{
    static const uint8_t cmd[4] = {0x5A, 0x04, 0x10, 0x6E};
    SendCommand(cmd, sizeof(cmd));
}

void TFminiPlus_c::SaveSettings()
{
    static const uint8_t cmd[4] = {0x5A, 0x04, 0x11, 0x6F};
    SendCommand(cmd, sizeof(cmd));
}

} // namespace Tof_n
