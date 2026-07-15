/*
 * menu.c
 *
 *  Created on: Jul 15, 2026
 *      Author: Pavel
 */

#include "menu.h"
#include "settings.h"
#include "encoder.h"
#include "lcd.h"
#include <stdio.h>


#define MENU_ITEMS_COUNT 10
#define MENU_SAVE_INDEX  9
#define MENU_TOTAL_ITEMS 10

typedef enum
{
	MENU_VIEW,
	MENU_EDIT,
	MENU_SAVE

} MenuMode_t;


static MenuMode_t menu_mode = MENU_VIEW;

static Settings_t menu_settings;

static uint8_t menu_index = 0;

static uint8_t save_select = 0;

static uint8_t menu_active = 0;
static uint8_t edit_mode = 0;


static uint8_t last_draw_index = 255;

typedef struct
{
	char name[16];

	uint8_t *value;
	uint8_t min;
	uint8_t max;
} MenuItem_t;

static MenuItem_t menu_items[MENU_ITEMS_COUNT];

void Menu_Init(void)
{
    menu_mode = MENU_VIEW;

    menu_settings = settings;


    menu_items[0] = (MenuItem_t)
    {
        "ENGINE WARN ON",
        &menu_settings.engine_overheat_temp_up, 50, 150
    };


    menu_items[1] = (MenuItem_t)
    {
        "ENGINE WARN OFF", &menu_settings.engine_overheat_temp_low, 40, 140
    };


    menu_items[2] = (MenuItem_t)
    {
        "BAT WARN ON",
        &menu_settings.battery_overheat_temp_up, 20, 80
    };


    menu_items[3] = (MenuItem_t)
    {
        "BAT WARN OFF",
        &menu_settings.battery_overheat_temp_low, 20, 80
    };


    menu_items[4] = (MenuItem_t)
    {
        "FAN OFF TEMP",
        &menu_settings.temp_bat_off, 0, 80
    };


    menu_items[5] = (MenuItem_t)
    {
        "FAN SPEED5",
        &menu_settings.temp_bat_speed5, 0, 40
    };


    menu_items[6] = (MenuItem_t)
    {
        "FAN SPEED6",
        &menu_settings.temp_bat_speed6, 0, 45
    };


    menu_items[7] = (MenuItem_t)
    {
        "BALANCE MAX",
        &menu_settings.upped_diff, 1, 30
    };


    menu_items[8] = (MenuItem_t)
    {
        "BALANCE MIN",
        &menu_settings.lower_diff, 0, 20
    };

}

uint8_t Menu_IsActive(void)
{
    //return menu_mode != MENU_VIEW;
	return menu_active;
}

static void Menu_ShowItem(void)
{
    char buf[17];


    LCD_Command(0x01);
    HAL_Delay(5);


    LCD_SetCursor(0,0);

    LCD_String(menu_items[menu_index].name);


    LCD_SetCursor(1,0);


    if(menu_mode == MENU_EDIT)
    {
        sprintf(buf,
                "<%3d>",
                *menu_items[menu_index].value);
    }
    else
    {
        sprintf(buf,
                " %3d ",
                *menu_items[menu_index].value);
    }


    LCD_String(buf);
}

static void Menu_ShowSave(void)
{
    LCD_Command(0x01);
    HAL_Delay(5);


    LCD_SetCursor(0,0);
    LCD_String("SAVE SETTINGS?");


    LCD_SetCursor(1,0);


    if(save_select == 0)
        LCD_String("<YES> NO");
    else
        LCD_String("YES <NO>");

}

static void Menu_Enter(void)
{
    menu_settings = settings;

    menu_index = 0;

    edit_mode = 0;

    menu_active = 1;

    LCD_Command(0x01);
    HAL_Delay(5);

    Menu_ShowItem();
}

static void Menu_Exit(void)
{
    menu_active = 0;
    edit_mode = 0;

    LCD_Command(0x01);
    HAL_Delay(5);
}

static void Menu_Save(void)
{
    settings = menu_settings;

    Settings_Save();

    LCD_Command(0x01);
    HAL_Delay(5);

    LCD_String("SAVED");

    HAL_Delay(1000);

    Menu_Exit();
}

void Menu_Process(void)
{
    int8_t dir;


    /*
       Если меню закрыто
    */

    if(!menu_active)
    {
        if(Encoder_Button_Held(5000))
        {
            Menu_Enter();
        }

        return;
    }



    /*
       Если меню открыто
    */

    dir = Encoder_Read();

    if(menu_index == MENU_SAVE_INDEX)
    {
        if(dir > 0 || dir < 0)
        {
            save_select ^= 1;

            Menu_ShowSave();
        }


        if(Encoder_Button_Pressed())
        {
            if(save_select == 0)
            {
                Menu_Save();
            }
            else
            {
                Menu_Exit();
            }
        }


        return;
    }



    /*
       РЕДАКТИРОВАНИЕ ЗНАЧЕНИЯ
    */

    if(edit_mode)
    {

        if(dir > 0)
        {
            if(*menu_items[menu_index].value <
               menu_items[menu_index].max)
            {
                (*menu_items[menu_index].value)++;
            }

            Menu_ShowItem();
        }


        if(dir < 0)
        {
            if(*menu_items[menu_index].value >
               menu_items[menu_index].min)
            {
                (*menu_items[menu_index].value)--;
            }

            Menu_ShowItem();
        }



        if(Encoder_Button_Pressed())
        {
            edit_mode = 0;

            Menu_ShowItem();
        }


        return;
    }



    /*
       ПРОСМОТР ПАРАМЕТРОВ
    */


    if(dir > 0)
    {
        menu_index++;

        if(menu_index >= MENU_TOTAL_ITEMS)
        {
            menu_index = 0;
        }

        Menu_Show();
    }


    if(dir < 0)
    {
        if(menu_index == 0)
            menu_index = MENU_TOTAL_ITEMS - 1;
        else
            menu_index--;

        Menu_Show();
    }



    /*
       Вход в редактирование
    */

    if(Encoder_Button_Pressed())
    {

        if(menu_index == MENU_SAVE_INDEX)
        {
            if(save_select == 0)
            {
                Menu_Save();
            }
            else
            {
                Menu_Exit();
            }

            return;
        }


        edit_mode = 1;

        Menu_ShowItem();
    }


}

static void Menu_Show(void)
{
    if(menu_index == MENU_SAVE_INDEX)
    {
        Menu_ShowSave();
    }
    else
    {
        Menu_ShowItem();
    }
}

