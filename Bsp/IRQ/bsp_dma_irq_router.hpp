#ifndef BSP_DMA_IRQ_ROUTER_HPP
#define BSP_DMA_IRQ_ROUTER_HPP

extern "C" {
#include "main.h"
}

#include <cstdint>

namespace BSP_n
{

/**
 * @brief DMA中断动态路由器。
 *
 * 将当前Flash向量表复制到RAM，并把指定DMA IRQ的向量替换为框架统一入口。
 * 这样DMA驱动无需向CubeMX生成的stm32xx_it.c中添加HAL_DMA_IRQHandler。
 */
class DmaIrqRouter
{
public:
    static bool Register(DMA_HandleTypeDef* hdma,
                         uint32_t preempt_priority = 5U,
                         uint32_t sub_priority = 0U);

    static void Dispatch();

private:
    static IRQn_Type GetIrq(DMA_HandleTypeDef* hdma);
    static void InitializeVectorTable();

    static constexpr uint32_t kCoreVectorCount = 16U;
    static constexpr uint32_t kMaxExternalIrqCount = 240U;
    static constexpr uint32_t kVectorTableSize = 256U;

    alignas(1024) static uint32_t ram_vector_table_[kVectorTableSize];
    static DMA_HandleTypeDef* dma_handles_[kMaxExternalIrqCount];
    static bool vector_table_initialized_;
};

} // namespace BSP_n

#endif
