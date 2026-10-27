#ifndef REFEREE_HPP
#define REFEREE_HPP

#include "bsp_usart.hpp"
#include "safe_task.hpp"
#include "referee_config.hpp"
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#include "ecf_configList.h"
#include <stdint.h>
#include <string.h>
#include "CRC.h"

#ifdef __cplusplus
}

namespace Referee_n {

    class referee_c {
    public:
    /************************** 变量 **************************/
        BSP_n::USART_c *referee = nullptr; // 裁判系统指针
        SafeTask_c *referee_safe = nullptr; // 裁判系统安全函数
        SafeTask_c *TC_Safe = nullptr; // VT13安全函数
        uint8_t is_online_re_ = 0; // 裁判系统是否在线
        uint8_t is_online_tc_ = 0; // VT13是否在线
        uint8_t referee_rx_buffer_[2][REFEREE_RX_BUFFER_NUM] = {0}; // 裁判系统双缓冲区
        RefereeData_t referee_data_; // 裁判系统数据
        VT13data_t vt13_data_; // VT13数据
    /************************** 函数 **************************/
        void RefereeInit(); // 裁判系统初始化
        void RefereeDataProcess(); // 裁判系统数据处理函数，包括VT13
        void RefereeClear(); // 掉线清理裁判系统数据
        void RefereeDataCRC16Deal(void *RefereeData, uint8_t *frame_header, uint16_t data_length); // CRC校验函数
        void VtmClear(); // 掉线清理VT13数据
    /************************** 单例 **************************/    
        static referee_c *GetInstance() // 饿汉式单例获取接口
        {
            static referee_c referee_instance_;
            return &referee_instance_;
        }
    private:
            referee_c() = default;
    };
// 裁判系统和VT13回调
void RefereeCallback(const uint8_t *pdata, uint16_t psize); 

} // namespace Referee_n

#endif

#endif // REFEREE_HPP
