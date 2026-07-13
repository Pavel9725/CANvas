/*
 * encoder.h
 *
 *  Created on: Jul 13, 2026
 *      Author: Pavel
 */

#ifndef INC_ENCODER_H_
#define INC_ENCODER_H_

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
