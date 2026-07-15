#include "settings.h"
#include <string.h>


#define SETTINGS_SIGNATURE    0x12345678      // "SET1"
#define SETTINGS_VERSION      1				  // for future




Settings_t settings;
static uint16_t Settings_CalculateCRC(void);

void Settings_LoadDefaults(void)
{
	settings.signature = SETTINGS_SIGNATURE;
	settings.version = SETTINGS_VERSION;

/* ===================== CRITICAL THRESHOLDS ========================= */

	settings.engine_overheat_temp_up   = 100;
	settings.engine_overheat_temp_low = 98;

	settings.battery_overheat_temp_up = 45;
	settings.battery_overheat_temp_low = 40;


/* ===================== DELTAS FOR BALANCING ========================= */

	settings.upped_diff = 8;
	settings.lower_diff = 3;



/* ===================== BATTERY TEMPERATURE THRESHOLDS FOR THE FAN ========================= */

	settings.temp_bat_off = 34;
	settings.temp_bat_speed5 = 35;
	settings.temp_bat_speed6 = 36;

	settings.crc = 0;
	settings.crc = Settings_CalculateCRC();
}

void Settings_Load(void)
{
	memcpy(&settings, (void*)SETTINGS_FLASH_ADDR, sizeof(Settings_t));

	if(!Settings_IsValid())
		{
			Settings_LoadDefaults();
		}
}

void Settings_Save(void)
{
	HAL_StatusTypeDef status = HAL_OK;

	settings.crc = Settings_CalculateCRC();

	/* 		unlock flash 	*/
	HAL_FLASH_Unlock();

	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

	FLASH_EraseInitTypeDef erase;

	uint32_t page_error = 0;

	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	erase.PageAddress = SETTINGS_FLASH_ADDR;
	erase.NbPages = 1;

	status = HAL_FLASHEx_Erase(&erase, &page_error);

	if(status != HAL_OK)
	{
		goto exit;
	}


	uint16_t *data = (uint16_t *)&settings;
	uint32_t size = (sizeof(Settings_t) + 1) / 2;

	for(uint32_t i = 0; i < size; i++)
	{
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, SETTINGS_FLASH_ADDR + i * 2, data[i]);

		if(status != HAL_OK)
		{
			goto exit;
		}
	}

	if(memcmp((void *)SETTINGS_FLASH_ADDR, &settings, sizeof(Settings_t)) != 0)
	        status = HAL_ERROR;
exit:

	HAL_FLASH_Lock();

	if(status != HAL_OK)
		printf("Settings save error\r\n");

}

void Settings_Reset(void)
{
	Settings_LoadDefaults();
	Settings_Save();
}

uint8_t Settings_IsValid(void)
{
	 if(settings.signature != SETTINGS_SIGNATURE)
			return 0;

	if(settings.version != SETTINGS_VERSION)
		return 0;

	if(settings.crc != Settings_CalculateCRC())
		return 0;

	if(settings.temp_bat_off > settings.temp_bat_speed5)
		return 0;

	if(settings.temp_bat_speed5 > settings.temp_bat_speed6)
		return 0;

	if(settings.engine_overheat_temp_low >= settings.engine_overheat_temp_up)
		return 0;

	if(settings.battery_overheat_temp_low >= settings.battery_overheat_temp_up)
		return 0;

	return 1;
}

static uint16_t Settings_CalculateCRC(void)
{
    uint16_t crc = 0xFFFF;

    uint8_t *data = (uint8_t *)&settings;

    for(uint32_t i = 0; i < sizeof(Settings_t) - 2; i++)
    {
        crc ^= data[i];

        for(uint8_t j = 0; j < 8; j++)
        {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}



