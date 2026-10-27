#ifndef __MODBUS_CRC_HPP
#define __MODBUS_CRC_HPP

#include <stdint.h>
#include <stddef.h>

namespace Modbus_n
{
    /**
     * @brief  MODBUS CRC-16 校验码计算
     * @param  data  数据缓冲区指针
     * @param  len   数据字节数
     * @return uint16_t CRC校验码（低字节在前，可直接放入发送缓冲区：tx_buff[n]=crc>>8, tx_buff[n+1]=crc）
     * @note   多项式: 0xA001 (反转的 0x8005), 初始值: 0xFFFF
     *
     * 测试样例1: data={0x02,0x04,0x00,0x00,0x00,0x02}, len=6 → 0x71F8
     * 测试样例2: Unlock_Setting() tx_buff_[0..10] (addr=0x01, password=1111) → 0x0EAC
     */
    uint16_t crc16(const uint8_t *data, size_t len);

} // namespace Modbus_n

#endif //! __MODBUS_CRC_HPP
