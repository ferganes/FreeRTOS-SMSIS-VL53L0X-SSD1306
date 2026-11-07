#ifndef I2C_H
#define I2C_H
#include "stm32f4xx.h"

void GPIO_init_I2C_pin(GPIO_TypeDef *GPIOx, uint8_t pin);

void I2Cx_init(I2C_TypeDef *I2Cx);
void I2Cx_StartTransfer(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t control_byte);
void I2Cx_StartReading(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t reg_byte);
void I2Cx_StopTransfer(I2C_TypeDef *I2Cx);
void I2Cx_SendCmd(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t control_byte, uint8_t cmd);

uint8_t I2Cx_ReadReg(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t reg_addr);
void I2Cx_WriteReg(I2C_TypeDef *I2Cx, uint8_t target_addr, uint8_t reg_addr, uint8_t value);


#endif