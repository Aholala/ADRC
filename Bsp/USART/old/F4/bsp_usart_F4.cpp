/**
 * @file bsp_usart_F4.cpp
 * @brief 底层bsp，负责创建实例，触发中断回调函数，接收数据/发送数据
 *
 * @version 1.0
 * @date 2024-9-16
 * @example
 * 1.引用命名空间后，需要创建一个结构体 //usart_init_t i
 * 2.为其赋值// rxbuf_size,rx/tx_callback函数，接收发送发送等，具体看hpp,根据需求不做初始化也可以
 * 3.之后便可以创建类  //usart_c example(usart_init)
 * 4.若有使能中断，则在进入中断后会自动进入自定义的callback函数
 * 例程如下：
 *
    void tx_call()
    {
        int i = 0;
    }
    void rx_call(uint8_t *buff, uint16_t size)
    {
        int i = 0;
    }
    uint8_t rx1[286] = {0};
     usart_init_t usart_con =
        {
        .usart_handle_ = &huart2, //hal库usart句柄
        .rxbuf_size_ = 10,        //接收区缓冲大小
        .rx_type_ = USART_RX_DMA_IDLE, //接收类型
        .tx_type_ = USART_TX_DMA,      //发送类型

        .usart_data_length_ = usart_data_length

        .usart_tx_callback_ptr_ = callback_m, //发送回调函数指针
        .usart_rx_callback_ptr_ = rx_call,  //接收回调函数指针
        .rx_buff_ptr = rx1,                 //发送区指针
        .sec_rxbuff_ptr = rx2 //接收区指针--双地址时用
        };
    usart_c test(usart_con);
    uint8_t tx_b[10] = {10, 10, 2, 1};
    test.USART_rx_start();
    while (1)
    {
    test.USART_send(tx_b, 4, 500);
    HAL_Delay(500);
    }

   @attention 若需要双缓冲，需要在结构体添加另一指针,并将DMA设置为循环(circuar)模式
 *  @vision 1.0 基本完成
            2.0 将部分改为私有，引出回调函数
            3.0 增加双缓冲
            5.0 修改双缓冲bug和类的权限
 */

#include "bsp_usart_F4.hpp"
#include <cstring>
namespace USART_N
{
    usart_c *header = nullptr; // 指向类的指针，做轮询链表

    /**
     * @brief 有参构造函数
     * @param usart_config 需要传入初始化结构体
     */
    usart_c::usart_c(usart_init_t &usart_config)
    {
        rxbuf_size = usart_config.rxbuf_size_;
        rx_type = usart_config.rx_type_;
        tx_type = usart_config.tx_type_;
        usart_rx_callback_ptr = usart_config.usart_rx_callback_ptr_;
        usart_tx_callback_ptr = usart_config.usart_tx_callback_ptr_;
        usart_handle = usart_config.usart_handle_;
        usart_data_length = usart_config.usart_data_length_;
        rx_buff_ptr = usart_config.rx_buff_ptr_;
        sec_rxbuff_ptr = usart_config.secondebuf_ptr_;
        // 进行赋值
        if (header == nullptr)
        {
            header = this;
        }
        else
        {
            usart_c *last_ptr = header;
            while (last_ptr->nest_instance != nullptr)
            {
                last_ptr = last_ptr->nest_instance;
            }
            last_ptr->nest_instance = this; // 简易链表，主要用于callback轮询
        }
    }
    usart_c::usart_c()
    {
    }
    usart_c::~usart_c()
    {
    }
    // 和构造函数相同，只是将外部接口调用出来
    void usart_c::USART_init(usart_init_t &usart_config)
    {
        rxbuf_size = usart_config.rxbuf_size_;
        rx_type = usart_config.rx_type_;
        tx_type = usart_config.tx_type_;
        usart_rx_callback_ptr = usart_config.usart_rx_callback_ptr_;
        usart_tx_callback_ptr = usart_config.usart_tx_callback_ptr_;
        usart_handle = usart_config.usart_handle_;
        usart_data_length = usart_config.usart_data_length_;
        rx_buff_ptr = usart_config.rx_buff_ptr_;
        sec_rxbuff_ptr = usart_config.secondebuf_ptr_;
        // 进行赋值
        if (header == nullptr)
        {
            header = this;
        }
        else
        {
            usart_c *last_ptr = header;
            while (last_ptr->nest_instance != nullptr)
            {
                last_ptr = last_ptr->nest_instance;
            }
            last_ptr->nest_instance = this; // 简易链表，主要用于callback轮询
        }
    }
    /***
     * @brief UART发送函数
     *
     * @param data      数据缓冲区的首地址
     * @param data_size 接受的数据长度
     * @param timeout 超时时间(只在阻塞式使用)，单位是ms，如果超过设置的时间，则函数返回HAL_TIMEOUT，
     * 如果设置为HAL_MAX_DELAY，处理器就会一直等到接受到设置好的数据数量再执行下一条语句。
     * @todo 加入数据缓冲区
     */
    void usart_c::USART_send(uint8_t *data, uint16_t data_size, uint32_t timeout)
    {
        switch (tx_type)
        {
        case USART_TX_DMA:
            /* code */
            HAL_UART_Transmit_DMA(usart_handle, data, data_size);
            break;
        case USART_TX_IT:
            HAL_UART_Transmit_IT(usart_handle, data, data_size);
            break;
        case USART_TX_BLOCK:
            HAL_UART_Transmit(usart_handle, data, data_size, timeout);
            break;
        default:
            break;
        }
    }

    /**
     * @brief 中断/DMA接收开启,用于初始化以及回调函数的重启，可直接加进构造函数中
     * @version 0.1
     * @date 2024-09-16
     **/
    void usart_c::USART_rx_start()
    {
        if (rx_buff_ptr == nullptr || rxbuf_size == 0)
        {
            while (1)
            {
            }
        }
        switch (rx_type)
        {
        case USART_RX_DMA_IDLE:
            HAL_UARTEx_ReceiveToIdle_DMA(usart_handle, rx_buff_ptr, rxbuf_size);
            memset(rx_buff_ptr, 0, rxbuf_size); // 清空接收缓冲区
            // 跃鹿战队的框架代码中失能了半接收中断，但经过实测并不会进入该中断，如后续测试到该中断再启用吧
            //  __HAL_DMA_DISABLE_IT(usart_handle->hdmarx, DMA_IT_HT);
            break;
        case USART_RX_IT_IDLE:
            HAL_UARTEx_ReceiveToIdle_IT(usart_handle, rx_buff_ptr, rxbuf_size);
            memset(rx_buff_ptr, 0, rxbuf_size); // 清空接收缓冲区
            break;
        case USART_RX_DMA_NUM:
            HAL_UART_Receive_DMA(usart_handle, rx_buff_ptr, rxbuf_size);
            memset(rx_buff_ptr, 0, rxbuf_size); // 清空接收缓冲区
            // __HAL_DMA_DISABLE_IT(usart_handle->hdmarx, DMA_IT_HT);
            break;
        case USART_RX_IT_NUM:
            HAL_UART_Receive_IT(usart_handle, rx_buff_ptr, rxbuf_size);
            memset(rx_buff_ptr, 0, rxbuf_size); // 清空接收缓冲区
            break;
        case USART_RX_DMA_IDLE_D:
            usart_RxDMA_double_buf_init(usart_handle, (uint32_t *)rx_buff_ptr, (uint32_t *)sec_rxbuff_ptr, rxbuf_size);
            memset(rx_buff_ptr, 0, rxbuf_size);    // 清空接收缓冲区
            memset(sec_rxbuff_ptr, 0, rxbuf_size); // 清空接收缓冲区
            break;
        default:
            while (1)
            {
                // 请重新确认接收模式
            }
            break;
        }
    }

    /**
     * @brief uart中断所有接收,用于重启
     * @author your name (you@domain.com)
     * @version 0.1
     * @date 2024-09-16
     **/
    void usart_c::USART_stop_recv()
    {
        HAL_UART_AbortReceive(usart_handle);
        memset(rx_buff_ptr, 0, rxbuf_size); // 清空接收缓冲区
        if (sec_rxbuff_ptr != nullptr)
        {
            memset(sec_rxbuff_ptr, 0, rxbuf_size);
        }
    }

    /***
     * @brief 若收到指定大小数据，则立刻返回
     * @param timeout 超时时间(只在阻塞式使用)，单位是ms，如果超过设置的时间，则函数返回HAL_TIMEOUT，
     * 如果设置为HAL_MAX_DELAY，处理器就会一直等到接受到设置好的数据数量再执行下一条语句。
     * @return 返回值rx为最后接收的数据量
     */
    uint16_t usart_c::USART_block_rx(uint32_t timeout)
    {
        if (rx_type == USART_RX_BLOCK_NUM)
        {
            if (HAL_UART_Receive(usart_handle, rx_buff_ptr, rxbuf_size, timeout) == HAL_OK)
            {
                return rxbuf_size;
            }
        }
        else if (rx_type == USART_RX_BLOCK_IDLE)
        {
            uint16_t rx_len;                                                                                 // rx为最后接收的数据量
            if (HAL_UARTEx_ReceiveToIdle(usart_handle, rx_buff_ptr, rxbuf_size, &rx_len, timeout) == HAL_OK) //
            {
                return rx_len;
            }
        }
        else
        {
            return 0; // recv failed
        }
        return 0;
    }
    void usart_c::usart_RxDMA_double_buf_init(UART_HandleTypeDef *huart, uint32_t *DstAddress, uint32_t *SecondMemAddress, uint32_t DataLength)
    {

        huart->ReceptionType = HAL_UART_RECEPTION_TOIDLE; // 改变接收类型为持续接收至完成或空闲

        huart->RxEventType = HAL_UART_RXEVENT_IDLE; // 接收事件为空闲中断

        huart->RxXferSize = DataLength; // 设置接收长度

        SET_BIT(huart->Instance->CR3, USART_CR3_DMAR); // 将对应串口的DMA打开

        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE); // 使能空闲中断
        // 设置双缓冲数组，开始接收
        HAL_DMAEx_MultiBufferStart(huart->hdmarx, (uint32_t)&huart->Instance->DR, (uint32_t)DstAddress, (uint32_t)SecondMemAddress, DataLength);
    }
    void usart_c::USART_RX_callback(UART_HandleTypeDef *huart) // 接收完成中断
    {
        usart_c *temp_ptr = header;
        while (temp_ptr != nullptr) // 链表轮询
        {
            if (temp_ptr->usart_handle == huart && temp_ptr->usart_rx_callback_ptr != nullptr)
            {
                temp_ptr->usart_rx_callback_ptr(temp_ptr->rx_buff_ptr, huart->RxXferSize); // 进自定义回调处理数据
                temp_ptr->USART_rx_start();                                                // 回调完成时重启
                break;
            }
            temp_ptr = temp_ptr->nest_instance;
        }
    }
    void usart_c::USART_RX_callback(UART_HandleTypeDef *huart, uint16_t Size) // 重载函数：一般处理接收数据中断，如空闲中断
    {
        usart_c *temp_ptr = header;
        while (temp_ptr != nullptr) // 链表轮询
        {
            if (temp_ptr->usart_handle == huart && temp_ptr->usart_rx_callback_ptr != nullptr)
            {
                if (temp_ptr->rx_type == USART_RX_DMA_IDLE_D) // 如果为双缓冲中断
                {
                    // 该标志位就是表示当接收数据的缓冲区是哪一个，如果等于0就是第一个缓冲区 等于1就是第二个缓冲区
                    if ((huart->hdmarx->Instance->CR & DMA_SxCR_CT) == RESET)
                    {
                        /* Current memory buffer used is Memory 0 */
                        // 失效DMA
                        __HAL_DMA_DISABLE(huart->hdmarx); // disable DMA
                        // set memory buffer 1
                        huart->hdmarx->Instance->CR |= DMA_SxCR_CT; // 设定缓冲区1
                        //if (Size == temp_ptr->usart_data_length)
                        {
                            temp_ptr->usart_rx_callback_ptr(temp_ptr->rx_buff_ptr, Size); // 进自定义回调处理数据
                        }
                    }
                    else
                    {
                        /* Current memory buffer used is Memory 1 */
                        __HAL_DMA_DISABLE(huart->hdmarx); // 失效DMA   disable DMA
                        // set memory buffer 0
                        huart->hdmarx->Instance->CR &= ~(DMA_SxCR_CT); // 设定缓冲区0
                        //if (Size == temp_ptr->usart_data_length)
                        {
                            temp_ptr->usart_rx_callback_ptr(temp_ptr->sec_rxbuff_ptr, Size); // 进自定义回调处理数据
                        }
                    }
                    __HAL_DMA_ENABLE(huart->hdmarx); // 使能DMA                            //  enable DMA
                }
                else // 如果不是双缓冲
                {
                    temp_ptr->usart_rx_callback_ptr(temp_ptr->rx_buff_ptr, Size); 
                    // 进自定义回调处理数据
                                                                                  // if (Size == temp_ptr->usart_data_length)
                                                                                  // {
                                                                                  //     temp_ptr->usart_rx_callback_ptr(temp_ptr->rx_buff_ptr, Size); // 进自定义回调处理数据
                                                                                  // }
                }
                temp_ptr->USART_rx_start(); // 回调完成时重启
                break;
            }
            temp_ptr = temp_ptr->nest_instance;
        }
    }
    void usart_c::USART_TX_callback(UART_HandleTypeDef *huart)
    {
        usart_c *temp_ptr = header;
        while (temp_ptr != nullptr) // 链表轮询
        {
            if (temp_ptr->usart_handle == huart && temp_ptr->usart_tx_callback_ptr != nullptr)
            {
                temp_ptr->usart_tx_callback_ptr(); // 进自定义回调处理数据

                break;
            }
            temp_ptr = temp_ptr->nest_instance;
        }
    }
    void usart_c::USART_ERORR_callback(UART_HandleTypeDef *huart)
    {
        usart_c *temp_ptr = header;
        while (temp_ptr != nullptr) // 链表轮询
        {
            if (temp_ptr->usart_handle == huart && temp_ptr->usart_rx_callback_ptr != nullptr)
            {
                temp_ptr->USART_stop_recv(); // 中断所有接收
                temp_ptr->USART_rx_start();  // 重启接收

                /*  your code */
                break;
            }
            temp_ptr = temp_ptr->nest_instance;
        }
    }

}
/**
 * @brief 每次dma/idle(空闲)中断发生时，都会调用此函数.对于每个uart实例会调用对应的回调进行进一步的处理
 *
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    USART_N::usart_c::USART_RX_callback(huart, Size);
}

/**
 * @brief usart接收完成中断
 * @author your name (you@domain.com)
 * @version 0.1
 * @date 2024-09-16
 *
 **/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    USART_N::usart_c::USART_RX_callback(huart);
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    while (1)
    {
        /* code *测试是否会进入半完成中断*/
    }
}

/*
 *@brief 发送完成回调
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    USART_N::usart_c::USART_TX_callback(huart);
}
/**
 *  @brief 错误回调，主要用于重启usart接收，也可在此自定义
 *
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    USART_N::usart_c::USART_ERORR_callback(huart);
}
