#include <stdio.h>  
#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "tasks.h"
#include "framebuffer.h"
#include "I2C.h"

TaskHandle_t HeartBeatHandle = NULL;
TaskHandle_t DisplayUpdateTaskHandle = NULL;
TaskHandle_t TempTaskHandle = NULL;
TaskHandle_t HumTaskHandle = NULL;

QueueHandle_t DisplayQueueHandle = NULL;
QueueHandle_t I2C_ErrorQueueHandle = NULL;

SemaphoreHandle_t I2C1_MutexHandle = NULL;

void Tasks_Create(void){

    // Хендлер очереди на 5 элементов, размер каждого элемента — размер указателя (4 байта)  
    DisplayQueueHandle = xQueueCreate(5, sizeof(display_msg_t));

    // Хендлер очереди ошибок на 1 элемент
    I2C_ErrorQueueHandle = xQueueCreate(1, sizeof(i2c_status_t));

    I2C1_MutexHandle = xSemaphoreCreateMutex();

    xTaskCreate(HeartBeatTask, "HeartBeatTask", 128, NULL,
                tskIDLE_PRIORITY + 1, &HeartBeatHandle);

    xTaskCreate(vDisplayUpdateTask, "DisplayUpdateTask", 128, NULL,
                tskIDLE_PRIORITY + 1, &DisplayUpdateTaskHandle);
    
    xTaskCreate(vTempTask, "TempTask", 256, NULL,
            tskIDLE_PRIORITY + 2, &TempTaskHandle);

    xTaskCreate(vHumTask, "HumTask", 256, NULL,
        tskIDLE_PRIORITY + 2, &HumTaskHandle);
}

// Таска HeartBeat паттерна контроля работы МК и состояния шины I2C
void HeartBeatTask(void *argument){
    
    i2c_status_t error_code = I2C_OK;

    // Текущее состояние шины I2C
    i2c_status_t last_error = I2C_OK;

    // Выключаем встроенный светодиод PC13
    GPIOC->BSRR = GPIO_BSRR_BS13;

    while (1) {
        // Проверяем очередь ошибок.
        // Если прилетела новая ошибка, обновляем last_error
        if (xQueueReceive(I2C_ErrorQueueHandle, &error_code, 0) == pdPASS) {
            last_error = error_code;
        }
        
        // Циклический показ ошибок шины I2C
        if (last_error > I2C_OK && last_error < I2C_ERR_MAX_ERR_NUM) {
            
            // Бесконечно повторяем серии вспышек, пока last_error не изменится
            for (uint8_t i = 0; i < last_error; i++) {
                GPIOC->BSRR = GPIO_BSRR_BR13;
                // Длительность вспышки
                vTaskDelay(pdMS_TO_TICKS(200));
                GPIOC->BSRR = GPIO_BSRR_BS13;
                // Пауза между сериями вспышек
                vTaskDelay(pdMS_TO_TICKS(700)); 
            }

            // Пауза между сериями вспышек при ошибке
            vTaskDelay(pdMS_TO_TICKS(3000));
            
        } else {            
            // Ошибок нет, работает Heartbeat каждые 10мс
            GPIOC->BSRR = GPIO_BSRR_BR13;   
            vTaskDelay(pdMS_TO_TICKS(10));  
            GPIOC->BSRR = GPIO_BSRR_BS13;   
            
            // Пауза Heartbeat в 2000мс
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }     
}

// Таска обновления дисплея с автоматическим аппаратным восстановлением шины
void vDisplayUpdateTask(void *argument){
    
    FrameBuffer_Clear();

    display_msg_t msg;
    
    while(1){       

        if (xQueueReceive(DisplayQueueHandle, &msg, portMAX_DELAY) == pdPASS) {

            do {
                FrameBuffer_WriteString(FrameBuffer_GetBuffer(), msg.text, msg.x, msg.page);
            } while (xQueueReceive(DisplayQueueHandle, &msg, 0) == pdPASS);

            xSemaphoreTake(I2C1_MutexHandle, portMAX_DELAY);

            uint8_t status = I2Cx_StartTransfer(I2C1, 0x78, 0x40); 

            // Успешная пересылка данных по DMA
            if (status == I2C_OK){

                DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
                DMA1_Stream6->NDTR = FrameBuffer_GetSize();
                I2C1->CR2 |= I2C_CR2_DMAEN;   
                DMA1_Stream6->CR |= DMA_SxCR_EN;

                bool is_BTF_received = (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500)) > 0);

                if (is_BTF_received) {
                    // Прерывание DMA пришло штатно, останавливаем шину I2C
                    status = I2Cx_StopTransfer(I2C1);
                } else {
                    // Error code I2C_ERR_BTF_TIMEOUT, не пришел BTF
                    status = I2C_ERR_BTF_TIMEOUT; 
                }

                // Отключаем стрим DMA
                I2C1->CR2 &= ~I2C_CR2_DMAEN;
                DMA1_Stream6->CR &= ~DMA_SxCR_EN;
            }

            // Ошибка при передаче данных DMA
            if (status != I2C_OK){                    
                
                xQueueOverwrite(I2C_ErrorQueueHandle, &status);  
                
                // Сброс и переинициализация I2C
                I2C1->CR1 |= I2C_CR1_SWRST;   
                I2C1->CR1 &= ~I2C_CR1_SWRST;  
                I2Cx_init(I2C1);              
                
                xSemaphoreGive(I2C1_MutexHandle);

                // Принудительно переводим в Blocked, чтобы освободить планировщик, и не стучаться в упавшую шину I2C
                vTaskDelay(pdMS_TO_TICKS(500));

                continue;         
                              
                } else {

                // Сброс ошибки при восстановлении шины I2C, пишем 0 в очередь ошибок. Таска на следующей итерации, увидит статус 0 и перезапишет last_error.
                // Контроллер переключается в успешный HeartBeat
                uint8_t clear_err = I2C_OK; //0
                xQueueOverwrite(I2C_ErrorQueueHandle, &clear_err); 
            }

            xSemaphoreGive(I2C1_MutexHandle);
        }
    }
}

// Заглушка произвольного девайса
void vTempTask(void *argument) {

    display_msg_t msg = { .x = 0, .page = 2 };
    int temp = 29;

    while (1) {
        snprintf(msg.text, sizeof(msg.text), "Temp: %d C   ", temp);
        xQueueSend(DisplayQueueHandle, &msg, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Заглушка произвольного девайса
void vHumTask(void *argument) {

    display_msg_t msg = { .x = 0, .page = 0 };
    int hum = 52;

    while (1) {        
        snprintf(msg.text, sizeof(msg.text), "Hum: %d prc   ", hum);
        xQueueSend(DisplayQueueHandle, &msg, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}