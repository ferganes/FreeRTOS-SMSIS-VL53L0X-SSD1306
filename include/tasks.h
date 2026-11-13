#ifndef TASKS_H
#define TASKS_H
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern TaskHandle_t HeartBeatHandle;
extern TaskHandle_t DisplayUpdateTaskHandle;

extern QueueHandle_t DisplayQueueHandle;
extern QueueHandle_t I2C_ErrorQueueHandle;

typedef struct {
    uint8_t x;          // Позиция по горизонтали (0..127)
    uint8_t page;       // Номер строки дисплея (0..7)
    char text[21];      // Буфер текста (21 символ + '\0' для ширины 128px при шрифте 6px)
} display_msg_t;


void Tasks_Create(void);
void HeartBeatTask(void *argument);
void vDisplayUpdateTask(void *argument);

void vTempTask(void *argument);
void vHumTask(void *argument);


#endif