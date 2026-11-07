#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Собственные 
#include "framebuffer.h"
#include "tasks.h"
#include "clock.h"
#include "I2C.h"
#include "DMA.h"
#include "ssd1306.h"
#include "tasks.h"


// Инициализация пина светодиода (PC13)
void led_init(void) {
    GPIOC->MODER &= ~GPIO_MODER_MODE13;  
    GPIOC->MODER |= GPIO_MODER_MODE13_0;
}



int main(void) {

    // Обновляем системную частоту
    SystemCoreClockUpdate();

    // Грязный Power On Reset
    volatile uint32_t delay_counter = 500000;
    while (delay_counter--) { __NOP(); }

    // Включаем тактирование 
    clock_init();
    
    // Настройка GPIO пинов для I2C1 (PB6 - SCL, PB7 - SDA)
    GPIO_init_I2C_pin(GPIOB, 6);
    GPIO_init_I2C_pin(GPIOB, 7);
    
    // Тестовый светодиод, убрать потом
    led_init();
    
    // Настройка периферии
    I2Cx_init(I2C1);
    DMA_init();
    Display_Init();



    Tasks_Create();

    // Запуск планировщика FreeRTOS
    vTaskStartScheduler();

    // Сюда программа никогда не должна дойти
    while (1) {}
}
















