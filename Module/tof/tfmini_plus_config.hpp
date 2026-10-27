#ifndef TFMINI_PLUS_CONFIG_HPP
#define TFMINI_PLUS_CONFIG_HPP

#include <cstdint>

namespace Tof_n
{

// TFmini Plus 标准数据帧帧头，连续两个 0x59 表示一帧开始。
#define TFMINI_PLUS_FRAME_HEADER 0x59U
// TFmini Plus 标准输出帧长度，单位：字节。
#define TFMINI_PLUS_FRAME_LENGTH 9U
// 串口 DMA 接收缓冲区长度，单位：字节。
#define TFMINI_PLUS_RX_BUFFER_NUM 64U
// 超过该时间未收到有效数据时判定模块离线，单位：ms。
#define TFMINI_PLUS_DISCONNECT_TIMEOUT_MS 200U
// 信号强度有效下限，低于该值认为测距结果不可靠。
#define TFMINI_PLUS_STRENGTH_LOW_LIMIT 100U
// TFmini Plus 输出的无效强度标记。
#define TFMINI_PLUS_INVALID_STRENGTH 65535U
// 单个 TFmini Plus 距离低通滤波系数，范围建议为 0.0f ~ 1.0f。
#define TFMINI_PLUS_DISTANCE_LPF_ALPHA 0.2f
// 多个 ToF 数据融合后的距离低通滤波系数。
#define TOF_FUSION_DISTANCE_LPF_ALPHA 0.2f

// TFmini Plus 解码后的测距数据和运行统计信息。
typedef struct
{
    // 原始距离，直接由传感器帧解析得到，单位：cm。
    uint16_t raw_distance_cm;
    // 低通滤波后的距离浮点值，单位：cm。
    float filtered_distance_cm;
    // 对滤波距离四舍五入后的输出距离，单位：cm。
    uint16_t distance_cm;
    // 回波信号强度，数值越大通常表示回波越可靠。
    uint16_t strength;
    // 传感器原始温度值，需按 TFmini Plus 协议换算。
    int16_t temp_raw;
    // 换算后的模块温度，单位：摄氏度。
    float temperature_c;
    // 当前帧校验是否通过，1 表示有效，0 表示无效。
    uint8_t frame_valid;
    // 当前距离是否可信，1 表示可信，0 表示距离或强度无效。
    uint8_t distance_valid;
    // 实测输出帧率，单位：Hz。
    float actual_frame_rate_hz;
    // 成功解析的有效帧累计次数。
    uint32_t update_count;
    // 校验和错误累计次数。
    uint32_t checksum_error_count;
    // 帧头或帧结构错误累计次数。
    uint32_t frame_error_count;
} TFminiPlusData_t;

} // namespace Tof_n

#endif // TFMINI_PLUS_CONFIG_HPP
