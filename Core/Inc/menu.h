#ifndef MENU_H
#define MENU_H

#include "stm32f1xx_hal.h"

/* ================= MENU ITEMS ================= */

typedef enum
{
    MENU_ENGINE_HOT = 0,
    MENU_ENGINE_COOL,

    MENU_BATTERY_HOT,
    MENU_BATTERY_COOL,

	MENU_FAN_OFF,
    MENU_FAN_SPEED5,
    MENU_FAN_SPEED6,

	MENU_TEMP_DIFF_UP,
	MENU_TEMP_DIFF_LOW,

    MENU_SAVE,
	MENU_DEFAULTS,

    MENU_ITEMS_COUNT

}MenuItem_t;


/* ================= MENU ================= */

typedef struct
{
    const char *name;

    int8_t *value;

    int8_t min;
    int8_t max;

} MenuList_t;


typedef struct
{
    uint8_t active;
    uint8_t edit;
    MenuItem_t item;

    uint8_t save_select;
    uint8_t save_mode;

    uint8_t defaults_select;
	uint8_t defaults_mode;

}Menu_t;


extern Menu_t Menu;


/* ================= FUNCTIONS ================= */

void Menu_Init(void);
void Menu_Open(void);
void Menu_Close(void);
void Menu_Process(void);
uint8_t Menu_IsSave(void);
uint8_t Menu_GetItem(void);
uint8_t Menu_IsSaveMode(void);
uint8_t Menu_GetSaveSelect(void);

const char *Menu_GetName(void);

uint8_t Menu_GetValue(void);
uint8_t Menu_IsEdit(void);
uint8_t Menu_IsActive(void);

static MenuList_t MenuItems[MENU_ITEMS_COUNT];

#endif
