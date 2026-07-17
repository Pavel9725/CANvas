#ifndef LCD_H_
#define LCD_H_

#include "stm32f1xx_hal.h"

void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_String(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Update(void);

void DWT_Delay_Init(void);
void DWT_Delay_us(uint32_t us);

#endif
