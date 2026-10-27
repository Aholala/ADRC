#include "bsp_dma_irq_router.hpp"

namespace BSP_n
{

alignas(1024) uint32_t DmaIrqRouter::ram_vector_table_[DmaIrqRouter::kVectorTableSize] = {};
DMA_HandleTypeDef* DmaIrqRouter::dma_handles_[DmaIrqRouter::kMaxExternalIrqCount] = {};
bool DmaIrqRouter::vector_table_initialized_ = false;

extern "C" void BSP_DMA_Common_IRQHandler(void)
{
    DmaIrqRouter::Dispatch();
}

void DmaIrqRouter::InitializeVectorTable()
{
    if (vector_table_initialized_)
        return;

    const auto* source_vectors = reinterpret_cast<const uint32_t*>(SCB->VTOR);
    const uint32_t external_irq_count =
        32U * ((SCnSCB->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk) + 1U);
    uint32_t vector_count = kCoreVectorCount + external_irq_count;
    if (vector_count > kVectorTableSize)
        vector_count = kVectorTableSize;

    for (uint32_t i = 0; i < vector_count; ++i)
        ram_vector_table_[i] = source_vectors[i];

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U)
        SCB_CleanDCache_by_Addr(ram_vector_table_, sizeof(ram_vector_table_));
#endif

    SCB->VTOR = reinterpret_cast<uint32_t>(ram_vector_table_);
    __DSB();
    __ISB();
    vector_table_initialized_ = true;
}

bool DmaIrqRouter::Register(DMA_HandleTypeDef* hdma,
                            uint32_t preempt_priority,
                            uint32_t sub_priority)
{
    const IRQn_Type irqn = GetIrq(hdma);
    const int32_t irq_index = static_cast<int32_t>(irqn);
    if (hdma == nullptr || irq_index < 0 ||
        static_cast<uint32_t>(irq_index) >= kMaxExternalIrqCount)
        return false;

    // DMA发送可能高频调用Register；同一IRQ已注册时不能反复清除pending标志。
    if (vector_table_initialized_ && dma_handles_[irq_index] == hdma)
        return true;

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    HAL_NVIC_DisableIRQ(irqn);

    InitializeVectorTable();
    dma_handles_[irq_index] = hdma;
    ram_vector_table_[kCoreVectorCount + static_cast<uint32_t>(irq_index)] =
        reinterpret_cast<uint32_t>(&BSP_DMA_Common_IRQHandler);

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U)
        SCB_CleanDCache_by_Addr(ram_vector_table_, sizeof(ram_vector_table_));
#endif
    __DSB();
    __ISB();

    HAL_NVIC_ClearPendingIRQ(irqn);
    HAL_NVIC_SetPriority(irqn, preempt_priority, sub_priority);
    HAL_NVIC_EnableIRQ(irqn);

    if (primask == 0U)
        __enable_irq();
    return true;
}

void DmaIrqRouter::Dispatch()
{
    const uint32_t exception_number = __get_IPSR();
    if (exception_number < kCoreVectorCount)
        return;

    const uint32_t irq_index = exception_number - kCoreVectorCount;
    if (irq_index >= kMaxExternalIrqCount)
        return;

    DMA_HandleTypeDef* hdma = dma_handles_[irq_index];
    if (hdma != nullptr)
        HAL_DMA_IRQHandler(hdma);
}

IRQn_Type DmaIrqRouter::GetIrq(DMA_HandleTypeDef* hdma)
{
    if (hdma == nullptr)
        return static_cast<IRQn_Type>(-1);

#if defined(DMA1_Stream0)
    if (hdma->Instance == DMA1_Stream0) return DMA1_Stream0_IRQn;
    if (hdma->Instance == DMA1_Stream1) return DMA1_Stream1_IRQn;
    if (hdma->Instance == DMA1_Stream2) return DMA1_Stream2_IRQn;
    if (hdma->Instance == DMA1_Stream3) return DMA1_Stream3_IRQn;
    if (hdma->Instance == DMA1_Stream4) return DMA1_Stream4_IRQn;
    if (hdma->Instance == DMA1_Stream5) return DMA1_Stream5_IRQn;
    if (hdma->Instance == DMA1_Stream6) return DMA1_Stream6_IRQn;
    if (hdma->Instance == DMA1_Stream7) return DMA1_Stream7_IRQn;
#endif
#if defined(DMA2_Stream0)
    if (hdma->Instance == DMA2_Stream0) return DMA2_Stream0_IRQn;
    if (hdma->Instance == DMA2_Stream1) return DMA2_Stream1_IRQn;
    if (hdma->Instance == DMA2_Stream2) return DMA2_Stream2_IRQn;
    if (hdma->Instance == DMA2_Stream3) return DMA2_Stream3_IRQn;
    if (hdma->Instance == DMA2_Stream4) return DMA2_Stream4_IRQn;
    if (hdma->Instance == DMA2_Stream5) return DMA2_Stream5_IRQn;
    if (hdma->Instance == DMA2_Stream6) return DMA2_Stream6_IRQn;
    if (hdma->Instance == DMA2_Stream7) return DMA2_Stream7_IRQn;
#endif
#if defined(BDMA_Channel0)
    if (hdma->Instance == BDMA_Channel0) return BDMA_Channel0_IRQn;
    if (hdma->Instance == BDMA_Channel1) return BDMA_Channel1_IRQn;
    if (hdma->Instance == BDMA_Channel2) return BDMA_Channel2_IRQn;
    if (hdma->Instance == BDMA_Channel3) return BDMA_Channel3_IRQn;
    if (hdma->Instance == BDMA_Channel4) return BDMA_Channel4_IRQn;
    if (hdma->Instance == BDMA_Channel5) return BDMA_Channel5_IRQn;
    if (hdma->Instance == BDMA_Channel6) return BDMA_Channel6_IRQn;
    if (hdma->Instance == BDMA_Channel7) return BDMA_Channel7_IRQn;
#endif

    return static_cast<IRQn_Type>(-1);
}

} // namespace BSP_n
