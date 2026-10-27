extern "C" {
#include "main.h"
}
#if defined(HAL_UART_MODULE_ENABLED)
#    ifndef __BSP_USART_H__
#        define __BSP_USART_H__

#        include <cstdint>
#        include <functional>
#        include <map>

extern "C" {
#        include <stdint.h>
#        include <string.h>
}

/** H7宏定义 */
#        ifdef STM32H7xx_HAL_H
/** F4宏定义 */
#        elif STM32F4xx_HAL_H
#        endif

namespace BSP
{
using TxCallback = std::function<void(void)>;
using RxCallback = std::function<void(uint8_t* data, uint16_t size)>;

class USART
{
public:
    uint8_t* sbhw = 0;
    // 发送模式枚举
    enum class TxMode {
        DMA,       // DMA模式
        INTERRUPT, // 中断模式
        BLOCK      // 阻塞模式
    };

    // 接收模式枚举
    enum class RxMode {
        BLOCK_SIZE,             // 固定长度阻塞模式
        INTERRUPT_SIZE,         // 固定长度中断模式
        DMA_SIZE,               // 固定长度DMA模式
        BLOCK_IDLE,             // 空闲中断阻塞模式
        INTERRUPT_IDLE,         // 空闲中断中断模式
        DMA_IDLE,               // 空闲中断DMA模式
        DMA_IDLE_DOUBLE_BUFFER, // DMA+空闲中断+双缓冲
        DMA_IDLE_CIRCLE_BUFFER  // 带环缓冲的DMA模式
    };

    // ==================== 方法一：建造者模式 ====================
    class Builder
    {
    public:
        Builder() = default;

        Builder& set_handle(UART_HandleTypeDef* handle_)
        {
            handle = handle_;
            return *this;
        }

        Builder& set_rxbuf_size(uint16_t rxbuf_size_)
        {
            rxbuf_size = rxbuf_size_;
            return *this;
        }

        Builder& set_data_length(uint16_t data_length_)
        {
            data_length = data_length_;
            return *this;
        }

        Builder& set_tx_mode(TxMode tx_mode_)
        {
            tx_mode = tx_mode_;
            return *this;
        }

        Builder& set_rx_mode(RxMode rx_mode_)
        {
            rx_mode = rx_mode_;
            return *this;
        }

        Builder& set_rx_buffer(uint8_t* rx_buffer_)
        {
            rx_buffer = rx_buffer_;
            return *this;
        }

        Builder& set_tx_callback(TxCallback tx_callback_)
        {
            tx_callback = tx_callback_;
            return *this;
        }

        Builder& set_rx_callback(RxCallback rx_callback_)
        {
            rx_callback = rx_callback_;
            return *this;
        }

        USART build() const
        {
            return USART(handle, rxbuf_size, data_length, tx_mode, rx_mode, rx_buffer, tx_callback,
                         rx_callback);
        }

        USART* build_ptr() const
        {
            return new USART(handle, rxbuf_size, data_length, tx_mode, rx_mode, rx_buffer,
                             tx_callback, rx_callback);
        }

    private:
        UART_HandleTypeDef* handle = nullptr;
        uint16_t rxbuf_size = 0;
        uint16_t data_length = 256;
        TxMode tx_mode = TxMode::BLOCK;
        RxMode rx_mode = RxMode::DMA_IDLE;
        uint8_t* rx_buffer = nullptr;
        TxCallback tx_callback = nullptr;
        RxCallback rx_callback = nullptr;
    };

    // ==================== 方法二：配置结构体 ====================
    struct Config {
        UART_HandleTypeDef* Handle = nullptr;
        uint16_t data_length = 256;
        TxMode tx_mode = TxMode::BLOCK;
        RxMode rx_mode = RxMode::DMA_IDLE;
        uint8_t* rx_buffer = nullptr;
        TxCallback pTxCallback = nullptr;
        RxCallback pRxCallback = nullptr;
    };

    // ==================== 构造方法 ====================

    // 方法一：建造者模式构造
    static Builder create_builder() { return Builder(); }

    // 方法二：配置结构体构造
    USART(const Config& config) :
        handle(config.Handle),
        data_length(config.data_length),
        tx_mode(config.tx_mode),
        rx_mode(config.rx_mode),
        rx_buffer(config.rx_buffer),
        tx_callback(config.pTxCallback),
        rx_callback(config.pRxCallback)
    {
        initialize();
    }

    // 传统参数构造（保持兼容性）
    USART(UART_HandleTypeDef* handle_ = nullptr,
          uint16_t rxbuf_size_ = 0,
          uint16_t data_length_ = 0,
          TxMode tx_mode_ = TxMode::BLOCK,
          RxMode rx_mode_ = RxMode::DMA_IDLE_DOUBLE_BUFFER,
          uint8_t* rx_buffer_ = nullptr,
          TxCallback tx_callback_ = nullptr,
          RxCallback rx_callback_ = nullptr) :
        handle(handle_),
        rxbuf_size(rxbuf_size_),
        data_length(data_length_),
        tx_mode(tx_mode_),
        rx_mode(rx_mode_),
        rx_buffer(rx_buffer_),
        tx_callback(tx_callback_),
        rx_callback(rx_callback_)
    {
        initialize();
    }

    // 默认构造
    USART() {}
    ~USART();

    // 删除拷贝构造和赋值操作
    USART(const USART&) = delete;
    USART& operator=(const USART&) = delete;

    // 设置回调函数
    void set_tx_callback(TxCallback callback) { tx_callback = callback; }
    void set_rx_callback(RxCallback callback) { rx_callback = callback; }

    // 回调函数方法

    inline static void handle_tx_callback(UART_HandleTypeDef* huart)
    {
        USART* instance = find_instance_by_handle(huart);
        if (instance != nullptr && instance->tx_callback != nullptr) {
            instance->tx_callback();
        }
    }

    inline static void handle_error_callback(UART_HandleTypeDef* huart)
    {
        USART* instance = find_instance_by_handle(huart);
        instance->stop_receive();
        instance->start_receive();
    }

    // 公共接口
    void send(const uint8_t* data, uint16_t size, uint32_t timeout = 100);
    void start_receive();
    void stop_receive();
    uint16_t block_receive(uint32_t timeout);

    // 获取状态信息
    UART_HandleTypeDef* get_handle() const { return handle; }
    bool is_initialized() const { return handle != nullptr && rx_buffer != nullptr; }
    bool is_double_buffer_mode() const { return rx_mode == RxMode::DMA_IDLE_DOUBLE_BUFFER; }

    friend void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart);
    friend void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size);
    friend void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart);

public:
    // 成员变量
    UART_HandleTypeDef* handle = nullptr; // 对应句柄指针
    uint16_t rxbuf_size = 0;           // 最大接收缓冲数据量,默认为0，注意双缓冲区应该为单缓冲的长度
    uint8_t data_length = 0;           // 数据包应有长度
    TxMode tx_mode = TxMode::BLOCK;    // 发送方式,默认为阻断发送
    RxMode rx_mode = RxMode::DMA_IDLE; // 接收方式,默认为DMA空闲中断接收
    uint8_t* rx_buffer = nullptr;      // 第一缓冲区指针，一般只使用这个
    TxCallback tx_callback = nullptr;  // 发送回调,初始化都为nullptr
    RxCallback rx_callback = nullptr;  // 接收回调
    uint8_t real_data[256] = {0}; // 实际接收数据长度

    USART* next_instance = nullptr; // 链表下一个实例指针

    // 静态链表头指针
    static USART* instance_list_head;

    // 内部方法
    void initialize()
    {
        if (handle == nullptr) {
            // 句柄未初始化，无法注册
            return;
        }
        register_instance();
        start_receive();
    }
    void register_instance();
    void unregister_instance();
    // 通过句柄查找实例
    static USART* find_instance_by_handle(UART_HandleTypeDef* huart);
    // 回调函数处理
    static void handle_rx_callback(UART_HandleTypeDef* huart, uint16_t size = 0);
    void handle_double_buffer_interrupt(UART_HandleTypeDef* huart, uint16_t size)
    { // 该标志位就是表示当接收数据的缓冲区是哪一个，如果等于0就是第一个缓冲区
        // __HAL_UART_CLEAR_PEFLAG(huart);
        __HAL_DMA_DISABLE(huart->hdmarx); // 失效DMA   disable DMA
        __HAL_DMA_SET_COUNTER(huart->hdmarx, get_total_buffer_size());
        // 等于1就是第二个缓冲区
        if (((((DMA_Stream_TypeDef*)huart->hdmarx->Instance)->CR) & DMA_SxCR_CT) == RESET) {
            // 将当前目标缓冲区从0改到1，即开启DMA后接收数据的缓冲区为第二个
            ((DMA_Stream_TypeDef*)huart->hdmarx->Instance)->CR |= DMA_SxCR_CT;
            invalidate_Dcache(get_primary_buffer_ptr(), rxbuf_size);
            memcpy(&real_data, get_primary_buffer_ptr(), size);
            // 使能DMA
            __HAL_DMA_ENABLE(huart->hdmarx);
            uint8_t* primary_buffer_ptr = get_primary_buffer_ptr();
            rx_callback(primary_buffer_ptr, size);
        } else {
            // 将当前目标缓冲区从1改到0，即开启DMA后接收数据的缓冲区为第一个
            ((DMA_Stream_TypeDef*)huart->hdmarx->Instance)->CR &= ~(DMA_SxCR_CT);
            invalidate_Dcache(get_secondary_buffer_ptr(), rxbuf_size);
            memcpy(&real_data, get_primary_buffer_ptr(), size);
            // 使能DMA
            __HAL_DMA_ENABLE(huart->hdmarx);
            uint8_t* secondary_buffer_ptr = get_secondary_buffer_ptr();
            rx_callback(secondary_buffer_ptr, size);
        }
    }

    // 初始化DMA双缓冲
    void init_dma_double_buffer()
    {
        if (handle == nullptr || handle->hdmarx == nullptr) return;

        handle->ReceptionType = HAL_UART_RECEPTION_TOIDLE; // 改变接收类型为持续接收至完成或空闲
        handle->RxEventType = HAL_UART_RXEVENT_IDLE;       // 接收事件为空闲中断
        handle->RxXferSize = get_total_buffer_size();      // 设置接收长度

        SET_BIT(handle->Instance->CR3, USART_CR3_DMAR); // 将对应串口的DMA打开
        __HAL_UART_ENABLE_IT(handle, UART_IT_IDLE);     // 使能空闲中断

        // 设置双缓冲数组，开始接收:这里H7和F4不一样，Insatnce为RDR寄存器
        HAL_DMAEx_MultiBufferStart(
            handle->hdmarx,
            reinterpret_cast<uint32_t>(get_data_register(handle)),  // 数据内存缓冲区寄存器
            reinterpret_cast<uint32_t>(get_primary_buffer_ptr()),   // 第一缓冲区地址
            reinterpret_cast<uint32_t>(get_secondary_buffer_ptr()), // 第二缓冲区地址
            get_total_buffer_size());
    }

    // 获取不同芯片的数据寄存器
    uint32_t get_data_register(UART_HandleTypeDef* handle)
    {
#        ifdef STM32H7xx_HAL_H
        return reinterpret_cast<uint32_t>(handle->Instance->RDR);
#        elif STM32F4xx_HAL_H
        return reinterpret_cast<uint32_t>(handle->Instance->DR);
#        endif
    }

    // 缓冲区管理
    uint8_t* get_primary_buffer_ptr() const { return rx_buffer; }
    // 获取第二缓冲区的地址
    uint8_t* get_secondary_buffer_ptr() const { return rx_buffer + rxbuf_size; }
    uint32_t get_total_buffer_size() const
    {
        return is_double_buffer_mode() ? rxbuf_size * 2 : rxbuf_size;
    }
    // 清除缓冲区
    void clear_buffer()
    {
        if (rx_buffer != nullptr) {
            memset(rx_buffer, 0, get_total_buffer_size());
        }
    }

    // 缓存维护 - 条件编译
    void invalidate_Dcache(uint8_t* buffer, uint16_t size) const
    {
#        ifdef __DCACHE_PRESENT == 1U
        if (buffer != nullptr && size > 0) {
            uint32_t aligned_addr = reinterpret_cast<uint32_t>(buffer) & ~(0x1F);
            uint32_t aligned_size = ((size + 31) / 32) * 32;
            SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(aligned_addr), aligned_size);
        }
#        else
        (void)buffer;
        (void)size;
#        endif
    }
    void clean_Dcache(const uint8_t* buffer, uint16_t size) const
    {
#        ifdef __DCACHE_PRESENT == 1U
        if (buffer != nullptr && size > 0) {
            uint32_t aligned_addr = reinterpret_cast<uint32_t>(buffer) & ~(0x1F);
            uint32_t aligned_size = ((size + 31) / 32) * 32;
            SCB_CleanDCache_by_Addr(
                const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(aligned_addr)),
                aligned_size);
        }
#        else
        (void)buffer;
        (void)size;
#        endif
    }
    void clean_invalidate_Dcache(uint8_t* buffer, uint16_t size) const
    {
#        ifdef __DCACHE_PRESENT == 1U
        if (buffer != nullptr && size > 0) {
            uint32_t aligned_addr = reinterpret_cast<uint32_t>(buffer) & ~(0x1F);
            uint32_t aligned_size = ((size + 31) / 32) * 32;
            SCB_CleanInvalidateDCache_by_Addr(
                const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(aligned_addr)),
                aligned_size);
        }
#        else
        (void)buffer;
        (void)size;
#        endif
    }
};
} // namespace BSP

#    endif // __BSP_USART_H__

#endif // HAL_UART_MODULE_ENABLED