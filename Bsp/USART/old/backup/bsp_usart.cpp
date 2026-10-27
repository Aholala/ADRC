/**
 * @file bsp_usart.cpp
 * @author Lann (you@domain.com)
 * @brief BSP层串口库，融合H7和F4，使用了多个重载构造函数优化初始化流程
 * @version 0.1
 * @date 2025-10-10
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "bsp_usart.hpp"
#if defined(HAL_UART_MODULE_ENABLED)
using namespace BSP;

// 链表头指针
USART* USART::instance_list_head = nullptr;

void USART::register_instance()
{
    if (handle == nullptr) {
        // 串口句柄为空，无法注册
        return;
    }
    
    // 如果链表为空，直接设置为头节点
    if (instance_list_head == nullptr) {
        instance_list_head = this;
        next_instance = nullptr;
        return;
    }
    
    // 遍历到链表末尾
    USART* current = instance_list_head;
    while (current->next_instance != nullptr) {
        current = current->next_instance;
    }
    
    // 添加到链表末尾
    current->next_instance = this;
    next_instance = nullptr;
}

void USART::unregister_instance()
{
    if (handle == nullptr || instance_list_head == nullptr) {
        return;
    }
    
    // 如果要移除的是头节点
    if (instance_list_head == this) {
        instance_list_head = next_instance;
        next_instance = nullptr;
        return;
    }
    
    // 查找并移除节点
    USART* current = instance_list_head;
    while (current != nullptr && current->next_instance != this) {
        current = current->next_instance;
    }
    
    if (current != nullptr) {
        current->next_instance = next_instance;
        next_instance = nullptr;
    }
}

USART* USART::find_instance_by_handle(UART_HandleTypeDef* huart)
{
    USART* current = instance_list_head;
    while (current != nullptr) {
        if (current->handle == huart) {
            return current;
        }
        current = current->next_instance;
    }
    return nullptr;
}

/**
 * @brief 串口数据发送函数
 *
 * @param data
 * @param size
 * @param timeout
 * @details 理论上没输入timeout的默认是100ms，不行的话你来帮帮我
 */
void USART::send(const uint8_t* data, uint16_t size, uint32_t timeout)
{
    switch (tx_mode) {
        case TxMode::DMA:
            SCB_CleanInvalidateDCache_by_Addr(
                reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(data)), size);
            HAL_UART_Transmit_DMA(handle, const_cast<uint8_t*>(data), size);
            break;
        case TxMode::INTERRUPT:
            HAL_UART_Transmit_IT(handle, const_cast<uint8_t*>(data), size);
            break;
        case TxMode::BLOCK:
            HAL_UART_Transmit(handle, const_cast<uint8_t*>(data), size, timeout);
            break;
        default:
            // 未知发送模式，处理错误
            break;
    }
}

void USART::start_receive()
{
    switch (rx_mode) {
        case RxMode::DMA_IDLE:
            invalidate_Dcache(rx_buffer, rxbuf_size);
            HAL_UARTEx_ReceiveToIdle_DMA(handle, rx_buffer, rxbuf_size);
            clear_buffer(); // 清空接收缓冲区
            // 跃鹿战队的框架代码中失能了半接收中断，但经过实测并不会进入该中断，如后续测试到该中断再启用吧
            __HAL_DMA_DISABLE_IT(handle->hdmarx, DMA_IT_HT);
            break;
        case RxMode::INTERRUPT_IDLE:
            HAL_UARTEx_ReceiveToIdle_IT(handle, rx_buffer, rxbuf_size);
            clear_buffer(); // 清空接收缓冲区
            break;
        case RxMode::DMA_SIZE:
            SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(rx_buffer), rxbuf_size);
            HAL_UART_Receive_DMA(handle, rx_buffer, rxbuf_size);
            clear_buffer(); // 清空接收缓冲区
            __HAL_DMA_DISABLE_IT(handle->hdmarx, DMA_IT_HT);
            break;
        case RxMode::INTERRUPT_SIZE:
            HAL_UART_Receive_IT(handle, rx_buffer, rxbuf_size);
            clear_buffer(); // 清空接收缓冲区
            break;
        case RxMode::DMA_IDLE_DOUBLE_BUFFER:
            
            init_dma_double_buffer(); // 初始化双缓冲
            clear_buffer();           // 清空接收缓冲区
            __HAL_DMA_DISABLE_IT(handle->hdmarx, DMA_IT_HT);
            break;
        default:
            // 未知接收模式，返航！
            break;
    }
}

void USART::stop_receive()
{
    if (handle != nullptr) {
        HAL_UART_AbortReceive(handle);
        clear_buffer();
    }
}

uint16_t USART::block_receive(uint32_t timeout)
{
    if (rx_mode == RxMode::BLOCK_SIZE) {
        if (HAL_UART_Receive(handle, rx_buffer, rxbuf_size, timeout) == HAL_OK) {
            return rxbuf_size;
        }
    } else if (rx_mode == RxMode::BLOCK_IDLE) {
        uint16_t rx_len; // rx为最后接收的数据量
        if (HAL_UARTEx_ReceiveToIdle(handle, rx_buffer, rxbuf_size, &rx_len, timeout) == HAL_OK) {
            return rx_len;
        }
    } else {
        return 0; // recv failed
    }
    return 0;
}

void USART::handle_rx_callback(UART_HandleTypeDef* huart, uint16_t size)
    {
        USART* instance = find_instance_by_handle(huart);
        if (instance != nullptr && instance->rx_callback != nullptr) {
            if (instance->is_double_buffer_mode()) { // 如果是双缓冲区模式
                instance->handle_double_buffer_interrupt(instance->handle, size);
            } else {
                instance->rx_callback(instance->rx_buffer, size);
            }
            instance->start_receive();
        }
    }

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size)
{
    USART::handle_rx_callback(huart, Size);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    USART::handle_rx_callback(huart);
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef* huart) {}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    USART::handle_tx_callback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    USART::handle_error_callback(huart);
}

#endif // HAL_UART_MODULE_ENABLED