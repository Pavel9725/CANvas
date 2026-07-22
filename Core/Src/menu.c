#include "menu.h"
#include "encoder.h"
#include "settings.h"
#include <string.h>

Menu_t Menu;


static Settings_t settings_backup;


static MenuList_t MenuItems[MENU_ITEMS_COUNT] =
{
    {
        "Engine Hot",
        &settings.engine_overheat_temp_up,
        80,
        120
    },

    {
        "Engine Cool",
        &settings.engine_overheat_temp_low,
        70,
        110
    },

    {
        "Battery Hot",
        &settings.battery_overheat_temp_up,
        40,
        90
    },

    {
        "Battery Cool",
        &settings.battery_overheat_temp_low,
        30,
        80
    },
	 {
		"Fan OFF",
		&settings.temp_bat_off,
		30,
		100
	},

    {
        "Fan Speed5",
        &settings.temp_bat_speed5,
        30,
        100
    },

    {
        "Fan Speed6",
        &settings.temp_bat_speed6,
        30,
        100
    },
	{
		"Temp diff up",
		&settings.upped_diff,
		0,
		10
	},
	{
		"Temp diff low",
		&settings.lower_diff,
		0,
		10
	},

    {
        "Save",
        NULL,
        0,
        0
    }
};


void Menu_Init(void)
{
	Menu.active = 0;
	Menu.edit = 0;
	Menu.item = MENU_ENGINE_HOT;

	Menu.save_mode = 0;
	Menu.save_select = 0;
}


void Menu_Open(void)
{
	Menu.active = 1;
	Menu.edit = 0;
	Menu.item = MENU_ENGINE_HOT;

	Menu.save_mode = 0;
	Menu.save_select = 0;


	memcpy(&settings_backup, &settings, sizeof(Settings_t));


	Encoder_Reset();
}

void Menu_Close(void)
{
	Menu.active = 0;
	Menu.edit = 0;

	Encoder_Reset();
}



static void Menu_Process_Button(void)
{
	ButtonEvent_t event = Encoder_GetEvent();

	if(event == BTN_RESET)
	    {
	        NVIC_SystemReset();
	    }

	if(event == BTN_NONE)
		return;

	if(Menu.active == 0)
	{
		switch(event)
		{
			case BTN_SHORT:
				break;

			case BTN_LONG:
				Menu_Open();
				break;

			case BTN_RESET:
				NVIC_SystemReset();
				break;

			default:
				break;
		}
		return;
	}

	if(event == BTN_SHORT)
	{
		if(Menu.save_mode)
		{
			if(Menu.save_select)
			{
				// YES
				Settings_Save();
			}
			else
			{
				// NO
				memcpy(&settings, &settings_backup, sizeof(Settings_t));
			}

			Menu.save_mode = 0;
			Menu_Close();

			return;
		}

		if(Menu.edit == 0)
		{
			if(Menu.item == MENU_SAVE)
			{
				Menu.save_mode = 1;
				Menu.save_select = 0;

				Encoder_Reset();

			}
			else
			{
				Menu.edit = 1;
			}
		}
		else
			Menu.edit = 0;
	}
}


static void Menu_ProcessEncoder(void)
{
	if(!Menu.active)
		return;


	if(Menu.save_mode)
	{
	    int8_t encoder = Encoder_Read();

	    if(encoder > 0)
	    {
	        Menu.save_select = 0;
	    }
	    else if(encoder < 0)
	    {
	        Menu.save_select = 1;
	    }

	    return;
	}

	int8_t encoder = Encoder_Read();

	if(encoder == 0)
		return;

	if(!Menu.edit)
	{
		if(encoder > 0)
		{
			Menu.item++;
			if(Menu.item >= MENU_ITEMS_COUNT)
				Menu.item = 0;
		}
		else
		{
			if(Menu.item == 0)
				Menu.item = MENU_ITEMS_COUNT - 1;
			else
				Menu.item--;
		}



		return;
	}

	MenuList_t *item = &MenuItems[Menu.item];

	if(item->value == NULL)
		return;


	if(encoder > 0)
	{
		if(*item->value < item->max)
			(*item->value)++;
	}
	else
	{
		if(*item->value > item->min)
			(*item->value)--;
	}

}

void Menu_Process(void)
{
	Menu_Process_Button();

	Menu_ProcessEncoder();
}

uint8_t Menu_IsEdit(void)
{
    return Menu.edit;
}

const char *Menu_GetName(void)
{
    return MenuItems[Menu.item].name;
}

uint8_t Menu_GetValue(void)
{
    if(MenuItems[Menu.item].value == NULL)
        return 0;

    return *MenuItems[Menu.item].value;
}


uint8_t Menu_IsActive(void)
{
    return Menu.active;
}


uint8_t Menu_IsSave(void)
{
    return (Menu.item == MENU_SAVE);
}


uint8_t Menu_GetItem(void)
{
    return Menu.item;
}

uint8_t Menu_IsSaveMode(void)
{
    return Menu.save_mode;
}


uint8_t Menu_GetSaveSelect(void)
{
    return Menu.save_select;
}
