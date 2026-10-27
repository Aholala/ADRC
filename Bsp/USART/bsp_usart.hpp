extern "C" {
#include "main.h"
}
#if defined(HAL_UART_MODULE_ENABLED)
#    ifndef __BSP_USART_HPP
#        define __BSP_USART_HPP

#        include "usart.h"
#        include "bsp_dma_irq_router.hpp"
#        include <cstdint>
#        include <cstring>
#        include <functional>
extern "C" {
}

namespace BSP_n
{
// 自定义接收回调指针
using TxCallback = std::function<void()>;
// 自定义接收回调指针，参数1:接收数组，参数2：数组长度
using RxCallback = std::function<void(const uint8_t* data, uint16_t size)>;

/* USART状态枚举 */

class USART_c
{
private:
    // 添加链表管理函数
    static void RegisterInstance(USART_c* instance);
    static void UnregisterInstance(USART_c* instance);
    static USART_c* FindInstanceByHandle(UART_HandleTypeDef* huart);
    static USART_c* FindInstanceByDmaHandle(DMA_HandleTypeDef* hdma);
    static void handleDmaRxComplete(DMA_HandleTypeDef* hdma);

    static USART_c* header;           // 链表头指针
    static USART_c* tail;             // 链表尾指针，用于快速插入
    USART_c* next_instance = nullptr; // 指向下一个实例

public:
    /* 发送模式枚举 */
    enum class TxType_t {
        DMA,  // DMA发送
        IT,   // 中断式发送
        BLOCK // 阻塞式发送
    };

    enum class RxType_t {
        BLOCK_NUM,      // 阻塞接收
        DMA_NUM,        // DMA接收一定字节
        IT_NUM,         // IT接收一定字节
        BLOCK_IDLE,     // 阻塞+空闲
        DMA_IDLE,       // DMA+Idle
        IT_IDLE,        // IT+Idle
        DMA_IDLE_DOUBLE // DMA+空闲+双缓冲
    };

    // 初始化结构体
    struct Config {
        UART_HandleTypeDef* Handle;       // 对应句柄指针
        uint16_t RxBuffSize = 0;          // 最大接收缓冲数据量,默认为0
        RxType_t RxType = RxType_t::DMA_IDLE; // 接收方式,默认为DMA空闲中断
        TxType_t TxType = TxType_t::BLOCK;    // 发送方式,默认为阻断发送
        TxCallback pTxCallback = nullptr; // 发送回调,初始化都为nullptr
        RxCallback pRxCallback = nullptr; // 接收回调
        uint8_t expected_data_length;     // 数据包应有长度
        uint8_t* pRxBuffer = nullptr;     // 第一缓冲区指针，一般只使用这个
        uint8_t* pSecRxBuffer = nullptr;  // 第二地址，使用DMA双缓冲时可用
    };
    // friend void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
public:
    USART_c() {}
    USART_c(UART_HandleTypeDef* handle_,
          uint16_t rxbuf_size_,
          uint8_t data_length_,
          TxType_t tx_type_,
          RxType_t rx_type_,
          uint8_t* rx_buff_ptr_,
          TxCallback tx_callback_,
          RxCallback rx_callback_) :
        rx_type(rx_type_),
        tx_type(tx_type_),
        rxbuf_size(rxbuf_size_),
        expected_data_length(data_length_),
        handle(handle_),
        rx_callback(rx_callback_),
        tx_callback(tx_callback_),
        rx_buffer(rx_buff_ptr_)
    {
        init();
    }

    USART_c(Config& config_)
    {
        handle = config_.Handle;
        rxbuf_size = config_.RxBuffSize;
        expected_data_length = config_.expected_data_length;
        rx_type = config_.RxType;
        tx_type = config_.TxType;
        rx_callback = config_.pRxCallback;
        tx_callback = config_.pTxCallback;
        rx_buffer = config_.pRxBuffer;
        sec_rx_buffer = config_.pSecRxBuffer;

        init();
    }
    ~USART_c() { UnregisterInstance(this); }

    void send(uint8_t* data, uint16_t data_size, uint32_t timeout = 0); // 发送函数
    void start_receive();                                               // 中断/DMA接收开启
    uint16_t block_receive(uint32_t timeout);                           // 阻断式接收
    void stop_receive();

    // 回调处理
    static void handleRxCallback(UART_HandleTypeDef* huart,
                                 uint16_t Size = 0);            // 重载函数，将hal库的回调函数引出
    static void handleTxCallback(UART_HandleTypeDef* huart);    // 将hal库的回调函数引出
    static void handleErrorCallback(UART_HandleTypeDef* huart); // 将hal库的回调函数引出
private:
    // 初始化实例
    void init()
    {
        if (rx_type == RxType_t::DMA_IDLE_DOUBLE && sec_rx_buffer == nullptr)
            sec_rx_buffer = rx_buffer + get_single_buffer_size();
        RegisterInstance(this);
        start_receive();
    }
    // 和构造函数相同，只是将外部接口调用出来
    void config_init(Config& config_)
    {
        rxbuf_size = config_.RxBuffSize;
        rx_type = config_.RxType;
        tx_type = config_.TxType;
        rx_callback = config_.pRxCallback;
        tx_callback = config_.pTxCallback;
        handle = config_.Handle;
        expected_data_length = config_.expected_data_length;
        rx_buffer = config_.pRxBuffer;
        sec_rx_buffer = config_.pSecRxBuffer;

        UnregisterInstance(this);
        RegisterInstance(this);
        start_receive();
    }
    // dma双缓冲初始化
    void initialize_dma_double_buffer()
    {
        const uint16_t single_buffer_size = get_single_buffer_size();

        // UART可能早于上层模块初始化。若此时线路上已有数据，RDR会累积FE/NE/ORE，
        // 必须在开启DMA前清除旧错误并刷新接收寄存器，否则DMA不会产生有效请求。
        CLEAR_BIT(handle->Instance->CR3, USART_CR3_DMAR);
        __HAL_UART_CLEAR_FLAG(handle,
                              UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF |
                                  UART_CLEAR_OREF | UART_CLEAR_IDLEF);
        __HAL_UART_SEND_REQ(handle, UART_RXDATA_FLUSH_REQUEST);

        handle->ReceptionType = HAL_UART_RECEPTION_TOIDLE; // 改变接收类型为持续接收至完成或空闲

        handle->RxEventType = HAL_UART_RXEVENT_IDLE; // 接收事件为空闲中断

        handle->RxXferSize = single_buffer_size; // DMA的每个缓冲区只接收一帧
        handle->RxXferCount = single_buffer_size;
        handle->pRxBuffPtr = rx_buffer;
        handle->RxState = HAL_UART_STATE_BUSY_RX;
        handle->ErrorCode = HAL_UART_ERROR_NONE;

        // HAL对DMA1/2双缓冲完成回调的命名与CT位语义容易混淆，
        // 两个完成入口统一读取CT位来判断刚完成的缓冲区。
        handle->hdmarx->XferCpltCallback = handleDmaRxComplete;
        handle->hdmarx->XferM1CpltCallback = handleDmaRxComplete;
        handle->hdmarx->XferHalfCpltCallback = nullptr;
        handle->hdmarx->XferM1HalfCpltCallback = nullptr;

        // 设置双缓冲数组，开始接收 :这里H7和F4不一样，Insatnce为RDR寄存器
        HAL_DMAEx_MultiBufferStart_IT(handle->hdmarx, get_data_register_address(), (uint32_t)rx_buffer,
                                      (uint32_t)sec_rx_buffer, single_buffer_size);

        if (handle->Init.Parity != UART_PARITY_NONE)
            ATOMIC_SET_BIT(handle->Instance->CR1, USART_CR1_PEIE);
        ATOMIC_SET_BIT(handle->Instance->CR3, USART_CR3_EIE | USART_CR3_DMAR);
        __HAL_UART_ENABLE_IT(handle, UART_IT_IDLE); // 使能空闲中断
    }
    // DMA TC发生后，CT已经切换到正在接收的缓冲区，另一块才是刚完成的缓冲区。
    void handle_dma_double_buffer_complete()
    {
        const uint16_t size = get_single_buffer_size();
        const bool memory1_is_active =
            ((((DMA_Stream_TypeDef*)handle->hdmarx->Instance)->CR & DMA_SxCR_CT) != RESET);
        uint8_t* completed_buffer = memory1_is_active ? rx_buffer : sec_rx_buffer;

        invalidate_Dcache(completed_buffer, size);
        rx_callback(completed_buffer, size);
        memset(completed_buffer, 0, size);
    }

    // IDLE只用于丢弃并重新对齐不足一帧的数据。完整帧由DMA TC回调处理。
    void handle_dma_double_buffer_idle(uint16_t size)
    {
        const uint16_t single_buffer_size = get_single_buffer_size();
        if (size == 0 || size >= single_buffer_size)
            return;

        __HAL_DMA_DISABLE(handle->hdmarx);
        uint8_t* active_buffer =
            ((((DMA_Stream_TypeDef*)handle->hdmarx->Instance)->CR & DMA_SxCR_CT) != RESET)
                ? sec_rx_buffer
                : rx_buffer;
        memset(active_buffer, 0, single_buffer_size);
        __HAL_DMA_SET_COUNTER(handle->hdmarx, single_buffer_size);
        __HAL_DMA_ENABLE(handle->hdmarx);
    }

    uint32_t get_data_register_address()
    {
#        if defined(STM32H7)
        return (uint32_t)&handle->Instance->RDR;
#        elif defined(STM32F4)
        return (uint32_t)&handle->Instance->DR;
#        endif
    }

    // 获取接收模式状态
    bool is_double_buffer_mode() const { return rx_type == RxType_t::DMA_IDLE_DOUBLE; }
    // 获取单缓冲长度
    uint16_t get_single_buffer_size() const
    {
        return is_double_buffer_mode() ? rxbuf_size / 2 : rxbuf_size;
    }

    // 缓存管理
    void invalidate_Dcache(uint8_t* buffer, uint16_t size) const
    {
#        if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        // SCB_InvalidateDCache_by_Addr((uint32_t*)buffer, size);
#        endif
    }
    void clean_invalidate_Dcache(uint8_t* buffer, uint16_t size) const
    {
#        if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        // SCB_CleanInvalidateDCache_by_Addr((uint32_t*)buffer, size);
#        endif
    }
    void clean_Dcache(uint8_t* buffer, uint16_t size) const
    {
#        if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        // SCB_CleanDCache_by_Addr((uint32_t*)buffer, size);
#        endif
    }

private:
    UART_HandleTypeDef* handle;       // 对应句柄指针
    uint8_t expected_data_length;     // 数据包应有长度
    RxType_t rx_type;                   // 接收方式
    TxType_t tx_type;                   // 发送方式,
    TxCallback tx_callback;           // 发送回调
    RxCallback rx_callback;           // 接收回调
    uint8_t* rx_buffer = nullptr;     // 第一缓冲区指针，一般只使用这个
    uint8_t* sec_rx_buffer = nullptr; // 第二地址，使用DMA双缓冲时可用
    uint16_t rxbuf_size;              // 最大接收缓冲数据量
};

} // namespace BSP

#    endif /* __BSP_USART_HPP */
#endif     /* HAL_UART_MODULE_ENABLED */
