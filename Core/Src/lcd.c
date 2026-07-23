#include "lcd.h"
#include "can.h"
#include "fan.h"
#include "warnings.h"
#include "main.h"
#include "menu.h"
#include <stdio.h>

#define LCD_PORT GPIOA

#define LCD_RS_PIN GPIO_PIN_0
#define LCD_E_PIN  GPIO_PIN_1

#define LCD_D4_PIN GPIO_PIN_2
#define LCD_D5_PIN GPIO_PIN_3
#define LCD_D6_PIN GPIO_PIN_4
#define LCD_D7_PIN GPIO_PIN_5


extern uint8_t warning_count;
extern uint8_t manual_fan_6;


uint32_t last_lcd_update = 0;
const uint32_t lcd_update_period = 100;
static char lcd_buffer[32]; // bufer


void DWT_Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DWT_Delay_us(uint32_t us)
{
    uint32_t cycles = (SystemCoreClock / 1000000L) * us;
    DWT->CYCCNT = 0; //reset counter
    while (DWT->CYCCNT < cycles);
}


static void LCD_Write4Bits(uint8_t data)
{
    // 4-bit mode
    HAL_GPIO_WritePin(LCD_PORT, LCD_D4_PIN, (data & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_PORT, LCD_D5_PIN, (data & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_PORT, LCD_D6_PIN, (data & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_PORT, LCD_D7_PIN, (data & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // wait for stabilization
    DWT_Delay_us(10);

    // strob E On
    HAL_GPIO_WritePin(LCD_PORT, LCD_E_PIN, GPIO_PIN_SET);

    // wait for stabilization
    DWT_Delay_us(50);

    // strob E off
    HAL_GPIO_WritePin(LCD_PORT, LCD_E_PIN, GPIO_PIN_RESET);

    // wait for stabilization
    DWT_Delay_us(50);
}

void LCD_Command(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
    DWT_Delay_us(10); // wait for update RS

    LCD_Write4Bits(cmd >> 4);
    DWT_Delay_us(100);

    LCD_Write4Bits(cmd & 0x0F);

    if (cmd == 0x01 || cmd == 0x02)
        HAL_Delay(20);  // wait clear display
    else if (cmd == 0x2A || cmd == 0x28 || cmd == 0x09)
	{
		HAL_Delay(10);
	}
	else
	{
		DWT_Delay_us(600);
	}
}

void LCD_Data(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_PORT, LCD_RS_PIN, GPIO_PIN_SET);
    DWT_Delay_us(5); // wait for update RS

    LCD_Write4Bits(data >> 4);
    LCD_Write4Bits(data & 0x0F);

    DWT_Delay_us(200);
}

void LCD_Init(void)
{
    DWT_Delay_Init();

    HAL_Delay(500); //wait for wakeup display


    HAL_GPIO_WritePin(LCD_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_PORT, LCD_E_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    LCD_Write4Bits(0x03); HAL_Delay(15);
    LCD_Write4Bits(0x03); HAL_Delay(15);
    LCD_Write4Bits(0x03); HAL_Delay(15);

    LCD_Write4Bits(0x02); HAL_Delay(15);


    LCD_Command(0x2A); // ON RS
	LCD_Command(0x09); // Set table rus
	LCD_Command(0x28); // return in normal mode


    LCD_Command(0x08); // reset driver (off display)
    LCD_Command(0x01); // clear RAM display     // wait clear
    LCD_Command(0x06);
    LCD_Command(0x0C); // on display
    HAL_Delay(10);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_Command(addr);
}

void LCD_String(char *str)
{
    while(*str)
    {
        LCD_Data((uint8_t)*str++);
    }
}

void LCD_Update(void)
{
    uint32_t current_time = HAL_GetTick();

    if(current_time - last_lcd_update < lcd_update_period)
        return;

    last_lcd_update = current_time;

    if(Menu_IsActive())
	{
		LCD_MenuUpdate();
		return;
	}


    if(warning_count > 0)
    {
    	Warnings_ShowLCD();
        return;
    }


    sprintf(lcd_buffer, "t:%3d%cC Bmin:%d ", temp_engine, 0xDF, temp_bat_min);

    LCD_SetCursor(0,0);
    LCD_String(lcd_buffer);

    if(manual_fan_6 == 1)
    	sprintf(lcd_buffer, "Fan_F:%d Bmax:%2d ", fan_speed, temp_bat_max);
    else if(balance_mode == 1)
    	sprintf(lcd_buffer, "Fan_B:%d Bmax:%2d ", fan_speed, temp_bat_max);
    else
    	sprintf(lcd_buffer, "Fan:%d   Bmax:%2d ", fan_speed, temp_bat_max);
    LCD_SetCursor(1,0);
    LCD_String(lcd_buffer);
}

void LCD_MenuUpdate(void)
{
    static uint8_t last_item = 255;
    static uint8_t last_value = 255;
    static uint8_t last_edit = 255;

    static uint8_t last_save_select = 255;
    static uint8_t last_save_mode = 255;

    static uint8_t last_defaults_select = 255;
    static uint8_t last_defaults_mode = 255;


    uint8_t value = Menu_GetValue();
    uint8_t edit = Menu_IsEdit();

    if(Menu_IsSaveMode())
    {
        uint8_t select = Menu_GetSaveSelect();

        if(last_save_mode == 1 && last_save_select == select)
            return;


        last_save_mode = 1;
        last_save_select = select;


        LCD_SetCursor(0,0);
        LCD_String("                ");

        LCD_SetCursor(1,0);
        LCD_String("                ");


        LCD_SetCursor(0,0);
        LCD_String("Save settings?");


        LCD_SetCursor(1,0);

        if(select)
        {
            LCD_String("<YES>   NO");
        }
        else
        {
            LCD_String(" YES   <NO>");
        }

        return;
    }

    last_save_mode = 0;

    if(Menu_IsDefaultsMode())
        {
            uint8_t select = Menu_GetDefaultsSelect();

            if(last_defaults_mode == 1 && last_defaults_select == select)
                return;


            last_defaults_mode = 1;
            last_defaults_select = select;


            LCD_SetCursor(0,0);
            LCD_String("                ");

            LCD_SetCursor(1,0);
            LCD_String("                ");


            LCD_SetCursor(0,0);
            LCD_String("Defaults settings?");


            LCD_SetCursor(1,0);

            if(select)
            {
                LCD_String("<YES>   NO");
            }
            else
            {
                LCD_String(" YES   <NO>");
            }

            return;
        }

        last_defaults_mode = 0;


    if(last_item != Menu_GetItem())
    {
        last_item = Menu_GetItem();


        LCD_SetCursor(0,0);
        LCD_String("                ");


        LCD_SetCursor(0,0);
        LCD_String(Menu_GetName());


        last_value = 255;
        last_edit = 255;
    }



    if(last_value != value || last_edit != edit)
    {
        last_value = value;
        last_edit = edit;


        LCD_SetCursor(1,0);
        LCD_String("                ");


        LCD_SetCursor(1,0);


       if(Menu_IsSave() || Menu_IsDefaults())
		   LCD_String("Press button");
	   else
	   {
		   if(edit)
		   {
			   sprintf(lcd_buffer, "< %d >", value);
		   }
		   else
		   {
			   sprintf(lcd_buffer, "  %d", value);
		   }

		   LCD_String(lcd_buffer);
	   }
    }
}
