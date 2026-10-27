#ifndef DR16_HPP
#define DR16_HPP

#include "bsp_usart.hpp"
#include "safe_task.hpp"
#include "dr16_config.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "string.h"
#include "ecf_configList.h"

#ifdef __cplusplus
}

namespace Dr16_n
{
    class dr16_c
    {
    public:
        BSP_n::USART_c *dr16;
        SafeTask_c *dt7_safe;
        uint8_t is_online_;
        uint8_t sbus_rx_buffer_[2][DT7_FRAME_LENGTH] = {0};
        DT7Ctrl_t dt7_data_;
        uint16_t deadline_limit_[5] = {DT7_DEAD_LINE, DT7_DEAD_LINE, DT7_DEAD_LINE, DT7_DEAD_LINE, DT7_DEAD_LINE}; // 遥控数据死区限制

        void Dr16Init();
        void Dt7DataProcess(const uint8_t *pData, uint16_t psize);
        void Dt7Clear();

        static dr16_c *GetInstance()
        {
            return &dr16_instance_; // 返回静态成员变量 instance 的地址
        }

    private:
        dr16_c() = default; // 私有构造函数
        static dr16_c dr16_instance_;//唯一实例指针
    };
    void DT7callback(const uint8_t *pdata, uint16_t psize);
}


#endif

#endif
