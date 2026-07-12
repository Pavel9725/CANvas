#include "lcd.h"

#define LCD_PORT GPIOA

#define LCD_RS_PIN GPIO_PIN_0
#define LCD_E_PIN  GPIO_PIN_1

#define LCD_D4_PIN GPIO_PIN_2
#define LCD_D5_PIN GPIO_PIN_3
#define LCD_D6_PIN GPIO_PIN_4
#define LCD_D7_PIN GPIO_PIN_5

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
/*void LCD_Init(void)
{
    DWT_Delay_Init();
    HAL_Delay(500); // Ожидание стабилизации встроенного чардж-пампа OLED

    // Сбрасываем управляющие линии
    HAL_GPIO_WritePin(LCD_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_PORT, LCD_E_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    // =================================================================
    // АППАРАТНЫЙ СБРОС ФАЗЫ ШИНЫ WS0010 (Строго по даташиту Winstar)
    // =================================================================
    // Каждый шаг 0x03 приводит внутренний счетчик полубайтов в исходное состояние
    LCD_Write4Bits(0x03); HAL_Delay(10);
    LCD_Write4Bits(0x03); HAL_Delay(10);
    LCD_Write4Bits(0x03); HAL_Delay(10);

    // Включаем 4-битный режим интерфейса
    LCD_Write4Bits(0x02); HAL_Delay(10);

    // Теперь контроллер гарантированно ждет ровно по ДВА полубайта на команду.

    // =================================================================
    // ПРЯМОЙ ВВОД КОМАНДЫ ПЕРЕКЛЮЧЕНИЯ НА КИРИЛЛИЦУ
    // =================================================================
    // Отправляем команду 0x2A (RE=1) вручную
    HAL_GPIO_WritePin(LCD_PORT, LCD_RS_PIN, GPIO_PIN_RESET); // Режим команд
    DWT_Delay_us(10);
    LCD_Write4Bits(0x02); // Старший полубайт от 0x2A
    DWT_Delay_us(50);
    LCD_Write4Bits(0x0A); // Младший полубайт от 0x2A
    HAL_Delay(10);        // Пауза для фиксации режима RE=1

    // Отправляем команду 0x09 (Выбор шрифта FT1 - Кириллица)
    LCD_Write4Bits(0x00); // Старший полубайт от 0x09
    DWT_Delay_us(50);
    LCD_Write4Bits(0x09); // Младший полубайт от 0x09
    HAL_Delay(10);        // Пауза для активации таблицы русского языка

    // Отправляем команду 0x28 (Выход из расширенного режима в RE=0)
    LCD_Write4Bits(0x02); // Старший полубайт от 0x28
    DWT_Delay_us(50);
    LCD_Write4Bits(0x08); // Младший полубайт от 0x28
    HAL_Delay(10);

    // =================================================================
    // СТАНДАРТНАЯ НАСТРОЙКА ОТОБРАЖЕНИЯ (Теперь через базовую LCD_Command)
    // =================================================================
    LCD_Command(0x08); // Полное выключение экрана перед чисткой RAM
    HAL_Delay(5);

    LCD_Command(0x01); // Очистка памяти дисплея (Внутри функции сработает задержка)
    HAL_Delay(5);

    LCD_Command(0x06); // Направление движения курсора (вправо)
    HAL_Delay(5);

    LCD_Command(0x0C); // Включение экрана (без курсора и мигания)
    HAL_Delay(15);
}*/


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
