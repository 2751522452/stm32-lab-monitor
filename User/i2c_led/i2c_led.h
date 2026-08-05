#ifndef __I2C_LED_H
#define __I2C_LED_H

#include "main.h"
#include "i2c/i2c.h"

#define OLED_ADDR  (0x3C << 1)

void OLED_WriteCmd(uint8_t cmd);
void OLED_WriteData(uint8_t data);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Fill(uint8_t data);
void OLED_SetCursor(uint8_t page, uint8_t column);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch);
void OLED_ShowString(uint8_t x, uint8_t page, const char *str);
void OLED_UpdateLine(uint8_t page, char *str);
	
#endif
