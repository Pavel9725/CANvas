#ifndef INC_SETTINGS_H_
#define INC_SETTINGS_H_

#include "stm32f1xx_hal.h"


/* last page Flash for STM32F103C8 (64 КБ) */
#define SETTINGS_FLASH_ADDR   0x0800FC00UL

#pragma pack(push,1)

typedef struct
{

	uint32_t signature;
	uint16_t version;

/* ===================== CRITICAL THRESHOLDS ========================= */

	uint8_t engine_overheat_temp_up;
    uint8_t engine_overheat_temp_low;

    uint8_t battery_overheat_temp_up;
    uint8_t battery_overheat_temp_low;

/* ===================== BATTERY TEMPERATURE THRESHOLDS FOR THE FAN ========================= */

    uint8_t temp_bat_off;
    uint8_t temp_bat_speed5;
    uint8_t temp_bat_speed6;

/* ===================== DELTAS FOR BALANCING ========================= */

    uint8_t upped_diff;
    uint8_t lower_diff;


    uint16_t crc;

} Settings_t;

#pragma pack(pop)

_Static_assert(sizeof(Settings_t) == 17, "Settings_t size changed!");

extern Settings_t settings;
extern Settings_t menu_settings;

void Settings_LoadDefaults(void);
void Settings_Load(void);
void Settings_Save(void);
void Settings_Reset(void);
uint8_t Settings_IsValid(void);



#endif
