#ifndef ENCODER_H
#define ENCODER_H

#include "stm32f1xx_hal.h"


#define ENC_A_Pin			GPIO_PIN_4
#define ENC_A_GPIO_Port 	GPIOB
#define ENC_B_Pin 			GPIO_PIN_5
#define ENC_B_GPIO_Port 	GPIOB
#define ENC_BTN_Pin 		GPIO_PIN_6
#define ENC_BTN_GPIO_Port 	GPIOB



void Encoder_Init(void);

int8_t Encoder_Read(void);


void Encoder_Reset(void);


int16_t Encoder_GetValue(void);


uint8_t Encoder_Button_Pressed(void);

uint8_t Encoder_Button_Held(uint32_t hold_time_ms);

void Encoder_Update(void);

#endif
