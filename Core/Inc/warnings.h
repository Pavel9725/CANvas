/*
 * warnings.h
 *
 *  Created on: Jul 17, 2026
 *      Author: Pavel
 */

#ifndef WARNINGS_H
#define WARNINGS_H

#include "stm32f1xx_hal.h"

#define WARNING_CAN      0
#define WARNING_ENGINE   1
#define WARNING_BATTERY  2


extern uint8_t warning_count;


void Warnings_Init(void);
void Warnings_Check(void);
void Warnings_ShowLCD(void);

#endif
