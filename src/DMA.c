#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dma.h"
#include "framebuffer.h"
#include "tasks.h" 

// Настройка DMA1 Stream 6
void DMA_init(void){

    // Отключаем стрим перед настройкой
    DMA1_Stream6->CR &= ~DMA_SxCR_EN; 

    // Ждем подтверждения отключения стрима
    while(DMA1_Stream6->CR & DMA_SxCR_EN);

    // Полный сброс регистра управления CR
    DMA1_Stream6->CR = 0;

    // Куда посылаем : регистр данных I2C
    DMA1_Stream6->PAR  = (uint32_t)&(I2C1->DR);       
    // Откуда посылаем: frame_buffer
    DMA1_Stream6->M0AR = (uint32_t)FrameBuffer_GetBuffer(); 

    // Сдвиги выбраны специально, можно и через дефайны, но зачем
    DMA1_Stream6->CR |= (1U << 25); // CHSEL: Channel 1
    DMA1_Stream6->CR |= (1U << 6);  // DIR: Memory-to-peripheral
    DMA1_Stream6->CR |= (1U << 10); // MINC: инкремент адреса памяти
    DMA1_Stream6->CR |= (1U << 4);  // TCIE: Прерывание по завершению (Transfer Complete)

    // Включаем прерывания
    NVIC_SetPriority(DMA1_Stream6_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}


//------------------// Хендлер прерывания DMA //------------------//

void DMA1_Stream6_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Проверяем флаг завершения передачи (Transfer Complete) для Stream 6
    if (DMA1->HISR & DMA_HISR_TCIF6)
    {
        DMA1->HIFCR = DMA_HIFCR_CTCIF6; // Сбрасываем установленный флаг

        // Будим задачу OLEDDMA через сохраненный хэндл
        if (DisplayUpdateTaskHandle != NULL)
        {
            vTaskNotifyGiveFromISR(DisplayUpdateTaskHandle, &xHigherPriorityTaskWoken);
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Переключаем контекст
    }
}