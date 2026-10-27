#ifndef TFMINI_PLUS_HPP
#define TFMINI_PLUS_HPP

#include "bsp_usart.hpp"
#include "safe_task.hpp"
#include "tfmini_plus_config.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
}

namespace Tof_n
{

// TFmini Plus 输出数据格式选择。
enum class TFminiPlusOutputMode_e : uint8_t
{
    // 标准二进制输出，距离单位为 cm。
    StandardCm = 0x01,
    // 字符串输出，距离单位为 m。
    StringM = 0x02,
    // 标准二进制输出，距离单位为 mm。
    StandardMm = 0x06,
};

// TFmini Plus 通信接口选择。
enum class TFminiPlusInterface_e : uint8_t
{
    // UART 串口通信。
    UART = 0x00,
    // I2C 通信。
    I2C = 0x01,
};

// TFmini Plus IO 引脚输出模式。
enum class TFminiPlusIOMode_e : uint8_t
{
    // IO 输出标准数据模式。
    StandardData = 0x00,
    // 近距离输出高电平，远距离输出低电平。
    NearHighFarLow = 0x01,
    // 近距离输出低电平，远距离输出高电平。
    NearLowFarHigh = 0x02,
};

// TFmini Plus 激光 ToF 模块驱动类，负责串口接收、帧解析、在线检测和参数配置。
class TFminiPlus_c
{
public:
    // 模块在线标志，1 表示在线，0 表示离线。
    uint8_t is_online_ = 0;
    // 最近一次解析到的测距数据和统计信息。
    TFminiPlusData_t data_ = {0};

    TFminiPlus_c() = default;
    // 使用指定 UART 句柄构造并初始化 TFmini Plus。
    explicit TFminiPlus_c(UART_HandleTypeDef *handle, const char *name = "TOF")
    {
        TFminiPlusInit(handle, name);
    }

    // 初始化 TFmini Plus 串口接收、DMA 空闲中断接收和离线检测任务。
    void TFminiPlusInit(UART_HandleTypeDef *handle, const char *name = "TOF");
    // 处理串口收到的一段原始字节流，内部会自动寻找帧头并解析完整帧。
    void DataProcess(const uint8_t *pdata, uint16_t psize);
    // 清空在线状态、帧缓存、滤波状态和测距数据。
    void Clear();

    // 获取滤波并取整后的距离，单位：cm。
    uint16_t GetDistanceCm() const { return data_.distance_cm; }
    // 获取原始距离，单位：cm。
    uint16_t GetRawDistanceCm() const { return data_.raw_distance_cm; }
    // 获取低通滤波后的距离，单位：cm。
    float GetFilteredDistanceCm() const { return data_.filtered_distance_cm; }
    // 获取滤波并取整后的距离，单位：m。
    float GetDistanceM() const { return static_cast<float>(data_.distance_cm) * 0.01f; }
    // 获取原始距离，单位：m。
    float GetRawDistanceM() const { return static_cast<float>(data_.raw_distance_cm) * 0.01f; }
    // 获取低通滤波后的距离，单位：m。
    float GetFilteredDistanceM() const { return data_.filtered_distance_cm * 0.01f; }
    // 获取回波信号强度。
    uint16_t GetStrength() const { return data_.strength; }
    // 获取模块温度，单位：摄氏度。
    float GetTemperatureC() const { return data_.temperature_c; }
    // 获取实测输出帧率，单位：Hz。
    float GetActualFrameRateHz() const { return data_.actual_frame_rate_hz; }
    // 判断当前距离数据是否可信。
    bool IsDistanceValid() const { return data_.distance_valid != 0U; }
    // 判断模块是否在线。
    bool IsOnline() const { return is_online_ != 0U; }
    // 设置距离低通滤波系数，输入会被限制到 0.0f ~ 1.0f。
    void SetDistanceLowPassAlpha(float alpha);

    // 发送 TFmini Plus 原始配置命令。
    void SendCommand(const uint8_t *cmd, uint16_t len);

    // 查询固件版本。
    void GetFirmwareVersion();
    // 软复位模块。
    void SystemReset();
    // 触发单次测距，适用于非连续输出模式。
    void TriggerOnce();
    // 设置输出数据格式。
    void SetOutputMode(TFminiPlusOutputMode_e mode);
    // 使能或关闭连续输出。
    void SetOutputEnable(bool enable);
    // 设置输出帧率，单位：Hz。
    void SetOutputRate(uint16_t hz);
    // 设置模块串口波特率。
    void SetBaudRate(uint32_t baudrate);
    // 设置通信接口为 UART 或 I2C。
    void SetCommunicationInterface(TFminiPlusInterface_e interface);
    // 设置 I2C 地址。
    void SetI2CAddress(uint8_t address);
    // 主动请求一帧 cm 单位距离数据。
    void RequestDistanceCm();
    // 主动请求一帧 mm 单位距离数据。
    void RequestDistanceMm();
    // 设置 IO 输出模式、触发距离和回差区间，单位：cm。
    void SetIOMode(TFminiPlusIOMode_e mode, uint16_t dist_cm, uint16_t zone_cm);
    // 设置低强度阈值，以及低强度时输出的替代距离，单位：cm。
    void SetStrengthLowThreshold(uint16_t strength_threshold, uint16_t low_strength_output_cm);
    // 设置低功耗等级，范围 0 ~ 10。
    void SetLowPowerMode(uint8_t level);
    // 快捷设置为标准 cm 输出模式。
    void SetOutputModeStandardCm();
    // 快捷设置为字符串 m 输出模式。
    void SetOutputModeStringM();
    // 快捷设置为标准 mm 输出模式。
    void SetOutputModeStandardMm();
    // 恢复出厂设置。
    void FactoryReset();
    // 保存当前配置到模块非易失存储。
    void SaveSettings();

    // 获取全局单例实例指针。
    static TFminiPlus_c *GetInstance()
    {
        return &tfmini_plus_instance_;
    }

private:
    // TFmini Plus 使用的 USART 驱动对象。
    BSP_n::USART_c *tof_usart = nullptr;
    // 用于在线/离线监测的安全任务对象。
    SafeTask_c *tof_safe = nullptr;
    // 串口 DMA 接收缓冲区。
    uint8_t rx_buffer_[TFMINI_PLUS_RX_BUFFER_NUM] = {0};
    // 标记模块上线，并在恢复时输出日志。
    void Online();
    // 标记模块离线，并清空相关数据。
    void Disonline();
    // 向帧状态机压入 1 字节数据，完整且有效时返回 true。
    bool PushByte(uint8_t data);
    // 解析一帧 9 字节标准 TFmini Plus 数据。
    bool ParseFrame(const uint8_t *frame);
    // 更新距离低通滤波结果。
    void UpdateDistanceLowPass(uint16_t raw_distance_cm);
    // 统计并更新实际输出帧率。
    void UpdateActualFrameRate();
    // 根据 TFmini Plus 协议计算低 8 位累加校验和。
    static uint8_t BuildChecksum(const uint8_t *data, uint16_t len);
    // TFmini Plus 全局单例对象。
    static TFminiPlus_c tfmini_plus_instance_;

private:
    // 模块名称，用于日志输出。
    char name_[16] = "TOF";
    // 离线日志锁存标志，避免离线期间重复刷日志。
    bool disconnected_latched_ = false;
    // 帧错误日志锁存标志，避免连续错误时重复刷日志。
    bool frame_fault_latched_ = false;
    // 当前正在组包的标准帧缓存。
    uint8_t frame_buffer_[TFMINI_PLUS_FRAME_LENGTH] = {0};
    // 当前帧缓存写入位置。
    uint8_t frame_index_ = 0;
    // 帧率统计窗口起始 tick，单位：ms。
    uint32_t frame_rate_window_tick_ = 0U;
    // 帧率统计窗口内的有效帧数量。
    uint16_t frame_rate_window_count_ = 0U;
    // 帧率统计窗口是否已经启动。
    uint8_t frame_rate_window_started_ = 0U;
    // 距离低通滤波系数。
    float distance_lpf_alpha_ = TFMINI_PLUS_DISTANCE_LPF_ALPHA;
    // 距离低通滤波器是否已经用首个有效值初始化。
    uint8_t distance_lpf_started_ = 0U;
};

} // namespace Tof_n

#endif

#endif // TFMINI_PLUS_HPP
