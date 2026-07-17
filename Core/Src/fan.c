/*
 * fan.c
 *
 *  Created on: Jul 17, 2026
 *      Author: Pavel
 */
#include "fan.h"
#include "settings.h"
#include "can.h"
#include "main.h"
#include <stdio.h>


/* ========================================================================== */
/*                     FAN CONDITION AND CONTROL                              */
/* ========================================================================== */

uint8_t fan_speed = 0;							//current speed fan
uint8_t balance_mode = 0;   					// 0 - normal mode, 1 - balance mode

static uint8_t fan_mode = 0; 							//current state fun 0 - one transmit, 1 - period transmit
static uint32_t last_fan_time = 0;						//timer sent command fan
static const uint32_t timeTransiveFanSpeed = 500;		//ms


void Fan_UpdateBalanceMode(void)
{

	int16_t temp_diff = (temp_bat_max >= temp_bat_min) ? (temp_bat_max - temp_bat_min) : (temp_bat_min - temp_bat_max);

	if(temp_diff >= settings.upped_diff)
	        balance_mode = 1;
	    else if (balance_mode && (temp_diff <= settings.lower_diff))
	        balance_mode = 0;
}


uint8_t Fan_GetTargetSpeed(void)
{
	extern uint8_t temp_bat_max;


	if(balance_mode)
		return FAN_6;

	if(temp_bat_max <= settings.temp_bat_off)
		return FAN_OFF;

	if(temp_bat_max >= settings.temp_bat_speed6)
		return FAN_6;

	if(temp_bat_max >= settings.temp_bat_speed5)
		return FAN_5;

	return fan_speed;
}


static void Fan_SendCommand(uint8_t speed)
{
	uint8_t data_fan[8] = { 0x04, 0x30, 0x81, 0x00, speed, 0x00, 0x00, 0x00 };

	CAN_SendRequest(0x7E3, 8, data_fan);

	fan_speed = speed;
	fan_mode = (speed == FAN_OFF) ? 0 : 1;

	#if DEBUG_MODE
	if(balance_mode)
		DEBUG_PRINT("Balance mode on 6 speed\r\n");
	else
		DEBUG_PRINT("FAN SPEED CHANDEG TO: %d\r\n", speed);
	#endif
}

void Fan_Control(void)
{
	uint8_t target_speed = Fan_GetTargetSpeed();

	if(target_speed == FAN_OFF && fan_mode != 0)
	{
		Fan_SendCommand(FAN_OFF);
		return;
	}

	if (HAL_GetTick() - last_fan_time >= timeTransiveFanSpeed)
	{
		last_fan_time = HAL_GetTick();
		Fan_SendCommand(target_speed);
	}

}

