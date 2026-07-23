#ifndef ENCODER_H
#define ENCODER_H

#include "stm32f1xx_hal.h"


#define ENC_A_Pin			GPIO_PIN_4
#define ENC_A_GPIO_Port 	GPIOB
#define ENC_B_Pin 			GPIO_PIN_5
#define ENC_B_GPIO_Port 	GPIOB
#define ENC_BTN_Pin 		GPIO_PIN_6
#define ENC_BTN_GPIO_Port 	GPIOB

// EVENT BUTTON
typedef enum
{
	BTN_NONE  = 0,
	BTN_SHORT,
	BTN_LONG,
	BTN_RESET

}ButtonEvent_t;


typedef struct
{
	uint8_t state; 				//0 - non press, 1 - press
	uint8_t last_state;			//last state press

	uint32_t debounce_time;

	uint32_t press_time;

	uint8_t pressed;

	uint8_t long3_triggered;
	uint8_t long10_triggered;

	ButtonEvent_t event;

}Encoder_t;

extern Encoder_t Encoder;


void Encoder_Init(void);
void Encoder_Button_Update(void);
void Encoder_Reset(void);
void Encoder_Update(void);

int8_t Encoder_Read(void);
uint8_t Encoder_Button_Debounce(void);
uint8_t Encoder_Button_Pressed(void);
uint8_t Encoder_Button_Held(uint32_t hold_time_ms);

int16_t Encoder_GetValue(void);

ButtonEvent_t Encoder_GetEvent(void);

#endif
