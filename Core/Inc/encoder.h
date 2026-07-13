/*
 * encoder.h
 *
 *  Created on: Jul 13, 2026
 *      Author: Pavel
 */

#ifndef INC_ENCODER_H_
#define INC_ENCODER_H_


/* ========================================================================== */
/*                           PIN ENCODER                                      */
/* ========================================================================== */
#define ENC_PORT GPIOB
#define ENC_A_PIN GPIO_PIN_0
#define ENC_B_PIN GPIO_PIN_1
#define ENC_BTN_PIN GPIO_PIN_2


typedef enum
{
	ENC_HOME,

	ENC_LEFT,
	ENC_RIGHT,

	ENC_CLICK,
	ENC_LONG_CLICK

}Encoder_Event_t;


void Encoder_Init(void);
void Encoder_Update(void);
Encoder_Event_t Encoder_GetEvent(void);


#endif /* INC_ENCODER_H_ */
