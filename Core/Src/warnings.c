/*
 * warnings.c
 *
 *  Created on: Jul 17, 2026
 *      Author: Pavel
 */

#include "warnings.h"
#include "can.h"
#include "lcd.h"
#include <settings.h>
#include <stdio.h>


extern Settings_t settings;


/* ================= On-screen alert carousel manager ======================== */
typedef struct
{
	uint8_t active;
	const char *line1;
} Warning_t;

Warning_t warnings[] =
{
	{0, "CAN BUS ERROR"},
	{0, "ENGINE HOT!!!"},
	{0, "BATTERY HOT!!!"}
};


uint8_t warning_count = 0;				//active warning
static uint8_t warning_index = 0;				//the displayed warning is currently being displayed
static uint32_t last_warning_time = 0;			//on-screen alert toggle timer
static uint16_t time_update_warning = 3000;
static char warning_buffer[32];


void Warnings_Init(void)
{
	warning_count = 0;
	warning_index = 0;
	last_warning_time = HAL_GetTick();

	for(int i = 0; i < 3; i++)
		warnings[i].active = 0;
}

void Warnings_Check(void)
{
    warnings[WARNING_CAN].active = can_bus_error;

    if(!warnings[WARNING_ENGINE].active)
	{
    	if(temp_engine >= settings.engine_overheat_temp_up)
    		warnings[WARNING_ENGINE].active = 1;
	}
    else
    {
    	if(temp_engine <= settings.engine_overheat_temp_low)
    	    		warnings[WARNING_ENGINE].active = 0;
    }

    if(!warnings[WARNING_BATTERY].active)
    	{
    	if(temp_bat_max >= settings.battery_overheat_temp_up)
    	            warnings[WARNING_BATTERY].active = 1;
    	}
		else
		{
			if(temp_bat_max <= settings.battery_overheat_temp_low)
				warnings[WARNING_BATTERY].active = 0;
		}

    warning_count = 0;

    for(uint8_t i = 0; i < 3; i++)
    {
        if(warnings[i].active)
            warning_count++;
    }
}


void Warnings_ShowLCD(void)
{
    static uint8_t blink = 1;
    static uint32_t last_blink = 0;

    uint32_t current_time = HAL_GetTick();

    if(current_time - last_warning_time >= time_update_warning)
    {
        last_warning_time = current_time;

        do
        {
            warning_index++;

            if(warning_index >= 3)
                warning_index = 0;

        }while(!warnings[warning_index].active);
    }

    if(current_time - last_blink >= 500)
    {
        last_blink = current_time;
        blink ^= 1;
    }


    LCD_SetCursor(0,0);
    if(blink)
        LCD_String((char*)warnings[warning_index].line1);
    else
    	LCD_String("                ");

    LCD_SetCursor(1,0);

    	switch(warning_index)
    	{
    	case WARNING_ENGINE:
    		sprintf(warning_buffer, "TEMP ENG: %3d%cC        ", temp_engine, 0xDF);
    			break;

    	case WARNING_BATTERY:
    		sprintf(warning_buffer, "TEMP BAT: %3d%cC        ", temp_bat_max, 0xDF);
    			break;

    	case WARNING_CAN:
    		sprintf(warning_buffer, "CHECK CAN BUS    ");
    			break;
    	}

    LCD_String(warning_buffer);
}



