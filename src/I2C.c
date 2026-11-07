#include "I2C.h"


// GPIO для I2C
void GPIO_init_I2C_pin(GPIO_TypeDef *GPIOx, uint8_t pin){
    uint8_t afr_idx = pin / 8;
    uint8_t afr_shift;
    
    if (pin < 8 ){
        afr_shift = pin * 4;
    } else {
        afr_shift = (pin - 8) * 4;
    }

    // Alternate function (10)
    GPIOx->MODER &= ~(3U << (pin * 2));
    GPIOx->MODER |=  (2U << (pin * 2));

    // Привязка к AF4 (I2C1)
    GPIOx->AFR[afr_idx] &= ~(15U << afr_shift);
    GPIOx->AFR[afr_idx] |=  (4U  << afr_shift);

    // Open drain
    GPIOx->OTYPER |= (1U << pin);

    // Very high speed (11)
    GPIOx->OSPEEDR &= ~(3U << (pin * 2));
    GPIOx->OSPEEDR |=  (3U << (pin * 2));

    // Pull Up (01)
    GPIOx->PUPDR &= ~(3U << (pin * 2));
    GPIOx->PUPDR |=  (1U << (pin * 2));
}

// Инициализация I2C
void I2Cx_init(I2C_TypeDef *I2Cx){

    I2Cx->CR1 &=~ I2C_CR1_PE; // Отключаем модуль

    I2Cx->CR1 |=  I2C_CR1_SWRST;   // сброс
    I2Cx->CR1 &=~ I2C_CR1_SWRST;

    I2Cx->CR2 &=~ I2C_CR2_FREQ;
    I2Cx->CR2 |= 42;               // Частота APB1 = 42 МГц

    I2Cx->CCR &=~ I2C_CCR_CCR;
    I2Cx->CCR |= I2C_CCR_FS;       // FastMode
    I2Cx->CCR |= 36;               // Делитель для 400 кГц

    I2Cx->TRISE = 13;              // Время подъема сигнала

    I2Cx->CR1 |= I2C_CR1_PE;       // Включаем модуль
}

// Подготовка I2C к передаче данных
void I2Cx_StartTransfer(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t control_byte){
    
    // Ожидание освобождения шины
    while(I2Cx->SR2 & I2C_SR2_BUSY);
    
    // Формирование START-условия
    I2Cx->CR1 |= I2C_CR1_START;
    while(!(I2Cx->SR1 & I2C_SR1_SB));
    
    // Отправка адреса устройства
    I2Cx->DR = target_addr;
    while(!(I2Cx->SR1 & I2C_SR1_ADDR));  

    // Очистка флага ADDR чтением SR1 и SR2
    (void)I2Cx->SR1; 
    (void)I2Cx->SR2;

    // Ожидание очистки регистра данных и отправка управляющего байта
    while(!(I2Cx->SR1 & I2C_SR1_TXE));
    I2Cx->DR = control_byte;
    while(!(I2Cx->SR1 & I2C_SR1_TXE));
}

// Останов I2C по BTF
void I2Cx_StopTransfer(I2C_TypeDef *I2Cx){
    while(!(I2Cx->SR1 & I2C_SR1_TXE));
    while(!(I2Cx->SR1 & I2C_SR1_BTF));
    I2Cx->CR1 |= I2C_CR1_STOP;
}

// Отправка команды в I2C периферию
void I2Cx_SendCmd(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t control_byte, uint8_t cmd){
    I2Cx_StartTransfer(I2Cx, target_addr, control_byte);    
    I2Cx->DR = cmd;
    I2Cx_StopTransfer(I2Cx);
}

// Универсальная запись в 8-битный регистр устройства
void I2Cx_WriteReg(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t reg_addr, uint8_t value){
    while(I2Cx->SR2 & I2C_SR2_BUSY);
    
    I2Cx->CR1 |= I2C_CR1_START;
    while(!(I2Cx->SR1 & I2C_SR1_SB));
    
    I2Cx->DR = target_addr; // Для VL53L0X это 0x52
    while(!(I2Cx->SR1 & I2C_SR1_ADDR));  
    (void)I2Cx->SR1; (void)I2Cx->SR2;

    while(!(I2Cx->SR1 & I2C_SR1_TXE));
    I2Cx->DR = reg_addr;    // Адрес регистра
    
    while(!(I2Cx->SR1 & I2C_SR1_TXE));
    I2Cx->DR = value;       // Значение
    
    while(!(I2Cx->SR1 & I2C_SR1_TXE));
    while(!(I2Cx->SR1 & I2C_SR1_BTF));
    I2Cx->CR1 |= I2C_CR1_STOP;
}

// Универсальное чтение из 8-битного регистра устройства
uint8_t I2Cx_ReadReg(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t reg_addr){
    while(I2Cx->SR2 & I2C_SR2_BUSY);
    
    I2Cx->CR1 |= I2C_CR1_START;
    while(!(I2Cx->SR1 & I2C_SR1_SB));
    
    I2Cx->DR = target_addr; // Запись (0x52)
    while(!(I2Cx->SR1 & I2C_SR1_ADDR));  
    (void)I2Cx->SR1; (void)I2Cx->SR2;

    while(!(I2Cx->SR1 & I2C_SR1_TXE));
    I2Cx->DR = reg_addr;
    while(!(I2Cx->SR1 & I2C_SR1_BTF));

    // Повторный старт для чтения
    I2Cx->CR1 |= I2C_CR1_START;    
    while(!(I2Cx->SR1 & I2C_SR1_SB));

    I2Cx->DR = target_addr | 1; // Чтение (0x53)
    while(!(I2Cx->SR1 & I2C_SR1_ADDR));  
    
    I2Cx->CR1 &= ~I2C_CR1_ACK; // NACK для единственного байта
    (void)I2Cx->SR1; (void)I2Cx->SR2;

    I2Cx->CR1 |= I2C_CR1_STOP; // Стоп генерируем СРАЗУ после очистки ADDR

    while (!(I2Cx->SR1 & I2C_SR1_RXNE));     
    return I2Cx->DR;  
}

