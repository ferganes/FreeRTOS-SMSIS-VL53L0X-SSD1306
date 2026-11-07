#include <stdio.h>  
#include <stdint.h>

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "tasks.h"
#include "framebuffer.h"
#include "i2c.h"

#define VL53L0X_ADDR 0x52

TaskHandle_t BlinkLedHandle = NULL;
TaskHandle_t DisplayUpdateTaskHandle = NULL;
TaskHandle_t LidarTaskHandle = NULL; 

QueueHandle_t DisplayQueueHandle = NULL;

SemaphoreHandle_t I2C1_MutexHandle = NULL; 

void vLidarTask(void *argument);

void Tasks_Create(void){
    // Хендлер очереди на 5 элементов, размер каждого элемента — размер указателя (4 байта)  
    DisplayQueueHandle = xQueueCreate(5, sizeof(char *));

    I2C1_MutexHandle = xSemaphoreCreateMutex();

    xTaskCreate(BlinkLed, "BlinkTask", 128, NULL,
                tskIDLE_PRIORITY + 1, &BlinkLedHandle);

    xTaskCreate(vDisplayUpdateTask, "OLEDDMA", 128, NULL,
                tskIDLE_PRIORITY + 1, &DisplayUpdateTaskHandle);
    
    xTaskCreate(vLidarTask, "LidarTask", 256, NULL,
            tskIDLE_PRIORITY + 1, &LidarTaskHandle);
}

// Таска тестового светодиода, удалить потом
void BlinkLed(void *argument){

    //const char *text_to_send;    

    while (1) {

        GPIOC->BSRR = GPIO_BSRR_BR13;    // Светодиод горит
        
        //text_to_send = "BOOOM";
        //xQueueSend(DisplayQueueHandle, &text_to_send, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(500));

        GPIOC->BSRR = GPIO_BSRR_BS13;    // Светодиод погас

        //text_to_send = "GO GO GO";

        //xQueueSend(DisplayQueueHandle, &text_to_send, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(500));
    }    
}

// Таска обновления OLED-дисплея
void vDisplayUpdateTask(void *argument){

    FrameBuffer_Clear();
    
    while(1){       
        
        char *msg_ptr = NULL;

        // xQueueReceive вернет pdPASS, если сообщение успешно извлечено
        if (xQueueReceive(DisplayQueueHandle, &msg_ptr, portMAX_DELAY) == pdPASS) {
            
            FrameBuffer_Clear();
            
            FrameBuffer_WriteString(FrameBuffer_GetBuffer(), msg_ptr, 0, 0);

            xSemaphoreTake(I2C1_MutexHandle, portMAX_DELAY);

            // Открываем I2C трансфер для данных
            I2Cx_StartTransfer(I2C1, 0x78, 0x40);

            // Сброс флаги прерываний DMA перед новым запуском
            DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;

            // Задаем размер DISPLAY_BUFFER_SIZE и включаем поток DMA
            DMA1_Stream6->NDTR = FrameBuffer_GetSize();

            I2C1->CR2 |= I2C_CR2_DMAEN;    // Разрешаем DMA
            DMA1_Stream6->CR |= DMA_SxCR_EN;

            // Таска засыпает, пробудится по прерыванию DMA1_Stream6_IRQHandler
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            I2C1->CR2 &= ~I2C_CR2_DMAEN;

            I2Cx_StopTransfer(I2C1);    
            
            xSemaphoreGive(I2C1_MutexHandle);
        }
    }
}

void vLidarTask(void *argument) {
    // Временно создаем указатель на обычную жестко зашитую строку в Flash-памяти
    // Она гарантированно не может переполнить буфер или испортить память
    const char *test_text = "!!!!!!!!!HELLO LIDAR";

    while(1) {
        // Просто раз в 500 мс отправляем строку на дисплей
        vTaskDelay(pdMS_TO_TICKS(500)); 
        
        xQueueSend(DisplayQueueHandle, &test_text, portMAX_DELAY);
    }
}
