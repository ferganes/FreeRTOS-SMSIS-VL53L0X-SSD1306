#ifndef I2C_H
#define I2C_H
#include "stm32f4xx.h"

// Описание ошибок шины по возвращаемому значению status
typedef enum {
    I2C_OK                    = 0,
    I2C_ERR_BUSY_TIMEOUT      = 1,
    I2C_ERR_START_TIMEOUT     = 2,
    I2C_ERR_ADDR_TIMEOUT      = 3,
    I2C_ERR_TXE_REG_TIMEOUT   = 4,
    I2C_ERR_TXE_DATA_TIMEOUT  = 5,
    I2C_ERR_TXE_STOP_TIMEOUT  = 6,
    I2C_ERR_BTF_TIMEOUT       = 7,
    I2C_ERR_NACK              = 8,
    I2C_ERR_MAX_ERR_NUM
} i2c_status_t;

// Количество тактов до срабатывания таймаута
#define I2C_WAIT_TIMEOUT 10000 

// Враппер для циклов ожидания ответа
#define I2C_WAIT_OR_RETURN(condition, err_code) do { \
    volatile uint32_t __timeout = I2C_WAIT_TIMEOUT;  \
    while (condition) {                              \
        if (--__timeout == 0) return (err_code);     \
    }                                                \
} while(0)

// Настройка пинов
void GPIO_init_I2C_pin(GPIO_TypeDef *GPIOx, uint8_t pin);

//Настройка I2Cx
void I2Cx_init(I2C_TypeDef *I2Cx);

// Управление трансфером байтов по I2Cx
i2c_status_t I2Cx_StartTransfer(I2C_TypeDef *I2Cx, uint8_t device_addr, uint8_t reg_byte);
i2c_status_t I2Cx_StopTransfer(I2C_TypeDef *I2Cx);

// Отправка команд в I2C устройство (инициализация устройства)
i2c_status_t I2Cx_SendCmd(I2C_TypeDef *I2Cx, uint8_t device_addr, uint8_t reg_byte, uint8_t cmd);

#endif