
#ifndef _BSP_USART_F4_H
#define _BSP_USART_F4_H

#define DEVICE_USART_CNT 3u     // 使用串口个数 ：暂时没用，可以写个类数量限制

#ifdef __cplusplus
extern "C"
{
#include "usart.h"
#include <stdint.h>

#endif
#ifdef __cplusplus
}
namespace USART_N
{
    // 回调
    using usart_tx_callback_p = void (*)();                    // 自定义接收回调指针
    using usart_rx_callback_p = void (*)(uint8_t *, uint16_t); // 自定义接收回调指针，参数1:接收数组，参数2：数组长度
    /* USART状态枚举 */

    /* 发送模式枚举 */
    enum USART_tx_type_e
    {
        USART_TX_DMA,   // DMA发送
        USART_TX_IT,    // 中断式发送
        USART_TX_BLOCK, // 阻塞式发送
    };
    /* 接收模式枚举 */
    enum USART_rx_type_e
    {
        USART_RX_BLOCK_NUM,  // 阻塞接收,注意无法使用回调
        USART_RX_DMA_NUM,    // DMA接收一定字节后进入回调
        USART_RX_IT_NUM,     // IT接收一定字节后进入回调
        USART_RX_BLOCK_IDLE, // 阻塞+空闲,接收一定字节或者触发空闲后返回,无法使用回调
        USART_RX_DMA_IDLE,   // DMA+Idle,接收一定字节或者触发空闲时进入回调
        USART_RX_IT_IDLE,    // IT+Idle,接收一定字节或者触发空闲时进入回调
        USART_RX_DMA_IDLE_D, // DMA+空闲+双缓冲，必须将DMA设置为循环(circuar)模式
    };
    // 初始化结构体
    struct usart_init_t
    {
        UART_HandleTypeDef *usart_handle_;                  // 对应句柄指针
        uint16_t rxbuf_size_ = 0;                            // 最大接收缓冲数据量,默认为0
        USART_rx_type_e rx_type_ = USART_RX_DMA_IDLE;       // 接收方式,默认为DMA空闲中断
        USART_tx_type_e tx_type_ = USART_TX_BLOCK;          // 发送方式,默认为阻断发送
        usart_tx_callback_p usart_tx_callback_ptr_ = nullptr; // 发送回调,初始化都为nullptr
        usart_rx_callback_p usart_rx_callback_ptr_ = nullptr; // 接收回调
        uint8_t usart_data_length_;                            // 数据包应有长度
        uint8_t *rx_buff_ptr_ = nullptr;                     // 第一缓冲区指针，一般只使用这个
        uint8_t *secondebuf_ptr_ = nullptr;                 // 第二地址，使用DMA双缓冲时可用
    };
    class usart_c
    {
        // friend void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
    public:
        friend void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
        friend void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
        friend void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
        usart_c();
        usart_c(usart_init_t &usart_config);
        ~usart_c();
        UART_HandleTypeDef *usart_handle;                                     // 对应句柄指针
     //   uint8_t rx_buff[USART_RXBUFF_LIMIT];                                // 原接收缓冲区，现改为地址
        USART_rx_type_e rx_type;                                              // 接收方式
        USART_tx_type_e tx_type;                                              // 发送方式,
        usart_c *nest_instance = nullptr;                                     // 链式结构指向下一个实例，方便轮询
        void USART_send(uint8_t *data, uint16_t data_size, uint32_t timeout); // 发送函数
        void USART_rx_start();                                                // 中断/DMA接收开启
        uint16_t USART_block_rx(uint32_t timeout);                            // 阻断式接收
        void USART_stop_recv();
        void usart_RxDMA_double_buf_init(UART_HandleTypeDef *huart, uint32_t *DstAddress, uint32_t *SecondMemAddress, uint32_t DataLength);
        void USART_init(usart_init_t &usart_config); // 和构造函数相同，只是将外部接口调用出来
        static void USART_RX_callback(UART_HandleTypeDef *huart);                //将hal库的回调函数引出
        static void USART_RX_callback(UART_HandleTypeDef *huart, uint16_t Size); // 重载函数，将hal库的回调函数引出
        static void USART_TX_callback(UART_HandleTypeDef *huart);                // 将hal库的回调函数引出
        static void USART_ERORR_callback(UART_HandleTypeDef *huart);             // 将hal库的回调函数引出

    protected:
        uint8_t usart_data_length;               // 数据包应有长度
        usart_tx_callback_p usart_tx_callback_ptr; // 发送回调
        usart_rx_callback_p usart_rx_callback_ptr; // 接收回调
        uint8_t *rx_buff_ptr = nullptr;          // 第一缓冲区指针，一般只使用这个
        uint8_t *sec_rxbuff_ptr = nullptr;       // 第二地址，使用DMA双缓冲时可用，必须将DMA设置为循环(circuar)模式
        uint16_t rxbuf_size;                        // 最大接收缓冲数据量
    };
}

#endif
#endif /*_BSP_USART_F4_H*/
