#ifndef TASKS_H
#define TASKS_H
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern TaskHandle_t BlinkLedHandle;
extern TaskHandle_t DisplayUpdateTaskHandle;
extern QueueHandle_t DisplayQueueHandle;

uint16_t VL53L0X_SimpleRead(void);
void VL53L0X_SimpleInit(void);

void Tasks_Create(void);
void BlinkLed(void *argument);
void vDisplayUpdateTask(void *argument);

void vLidarTask(void *argument);


#endif