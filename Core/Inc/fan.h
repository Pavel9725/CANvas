
#ifndef INC_FAN_H_
#define INC_FAN_H_

#include "stm32f1xx_hal.h"

/* ===================== CONTROL FAN ========================= */
#define FAN_OFF 0x00				//speed off fan
#define FAN_5 0x05					//speed 5 fan
#define FAN_6 0x06					//speed 6 fan

extern uint8_t fan_speed;
extern uint8_t balance_mode;


void Fan_Control(void);
void Fan_UpdateBalanceMode(void);
uint8_t Fan_GetTargetSpeed(void);


#endif
