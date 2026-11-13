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
    //I2Cx->CCR |= I2C_CCR_DUTY;     // Включаем DUTY = 1, соотношение 16:9 для стабильности на 400 кГц
    I2Cx->CCR |= 36;               // Делитель для 400 кГц

    I2Cx->TRISE = 13;              // Время подъема сигнала

    I2Cx->CR1 |= I2C_CR1_PE;       // Включаем модуль
}

// Подготовка I2C к передаче данных, при ошибке вернется статус из i2c_status_t
i2c_status_t I2Cx_StartTransfer(I2C_TypeDef *I2Cx, uint8_t device_addr, uint8_t reg_byte){
    
    // Ожидание освобождения шины
    I2C_WAIT_OR_RETURN((I2Cx->SR2 & I2C_SR2_BUSY), I2C_ERR_BUSY_TIMEOUT);
    
    // Формирование START-условия
    I2Cx->CR1 |= I2C_CR1_START;
    I2C_WAIT_OR_RETURN(!(I2Cx->SR1 & I2C_SR1_SB), I2C_ERR_START_TIMEOUT);
    
    // Отправка адреса устройства
    I2Cx->DR = device_addr;

    // Ожидание флага AF в ADDR с перехватом NACK
    volatile uint32_t timeout = I2C_WAIT_TIMEOUT;
    while (!(I2Cx->SR1 & I2C_SR1_ADDR)) {
        if (I2Cx->SR1 & I2C_SR1_AF) {
            I2Cx->SR1 &= ~I2C_SR1_AF; // Сбрасываем аппаратный флаг ошибки
            I2Cx->CR1 |= I2C_CR1_STOP; // Освобождаем шину через STOP
            return I2C_ERR_NACK;
        }
        if (--timeout == 0) {
            return I2C_ERR_ADDR_TIMEOUT;
        }
    }

    // Очистка флага ADDR чтением SR1 и SR2
    (void)I2Cx->SR1; 
    (void)I2Cx->SR2;

    // Ожидание очистки регистра данных и отправка управляющего байта
    I2C_WAIT_OR_RETURN(!(I2Cx->SR1 & I2C_SR1_TXE), I2C_ERR_TXE_REG_TIMEOUT);
    I2Cx->DR = reg_byte;
    I2C_WAIT_OR_RETURN(!(I2Cx->SR1 & I2C_SR1_TXE), I2C_ERR_TXE_DATA_TIMEOUT);

    return I2C_OK;
}

// Останов I2C по BTF
i2c_status_t I2Cx_StopTransfer(I2C_TypeDef *I2Cx){

    I2C_WAIT_OR_RETURN(!(I2Cx->SR1 & I2C_SR1_TXE), I2C_ERR_TXE_STOP_TIMEOUT);
    I2C_WAIT_OR_RETURN(!(I2Cx->SR1 & I2C_SR1_BTF), I2C_ERR_BTF_TIMEOUT);
    I2Cx->CR1 |= I2C_CR1_STOP;

    return I2C_OK;
}

// Отправка команды в I2C периферию
i2c_status_t I2Cx_SendCmd(I2C_TypeDef *I2Cx, uint8_t device_addr, uint8_t reg_byte, uint8_t cmd){    
    i2c_status_t status = I2Cx_StartTransfer(I2Cx, device_addr, reg_byte);    
    if (status != I2C_OK) {
        return status;
    } 
    I2Cx->DR = cmd;

    return I2Cx_StopTransfer(I2Cx);
}