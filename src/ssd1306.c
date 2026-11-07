#include "stm32f4xx.h"
#include "ssd1306.h"
#include "I2C.h"
#include "framebuffer.h"

// Последовательность инициализации дисплея SSD1306
const uint8_t Display_InitSequence[] = {
    0xAE,       //Выключить дисплей
    
    // Адресация памяти
    0x20, 0x00, //Установка Horizontal Addressing Mode (Горизонтальный режим)

    0x21, 0, 127, //Границы колонок: от 0 до 127
    0x22, 0, 7,   //Границы страниц (строк): от 0 до 7

    0xC8,       //Разворот строк (COM Output Scan Direction) - верх/низ
    0xA1,       //Разворот колонок (Segment Remap) - лево/право
    
    0xA6,       //Нормальное отображение (не инверсное)
    0xA8, 0x3F, //Мультиплексор (высота экрана 64 пикселя: 63)
    
    0xD3, 0x00, //Смещение дисплея (Display Offset: 0)
    0x40,       //Стартовая строка (Display Start Line: 0)
    
    0xD5, 0x80, //Частота осциллятора (Display Clock Divide Ratio)
    0xD9, 0x22, //Период предзаряда (Pre-charge Period)
    0xDA, 0x12, //Конфигурация COM-выводов (Alternative COM Pin Config)
    0xDB, 0x20, //Уровень VCOMH (VCOMH Deselect Level)
    
    0x81, 0x7F, //Яркость (Contrast: 127)
    0xA4,       //Выводить данные из GDDRAM (Resume to RAM content)
    
    0x8D, 0x14, //Включить Charge Pump — без этого экран будет темным
    0xAF        //Включить дисплей
};

void Display_Init(void){

    uint8_t arr_len = sizeof(Display_InitSequence) / sizeof(Display_InitSequence[0]);

    for( uint8_t i = 0; i < arr_len; i++){
        I2Cx_SendCmd(I2C1, 0x78, 0x00, Display_InitSequence[i]);        
    }

    FrameBuffer_Clear();
}