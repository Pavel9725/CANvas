#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_can.h"
#include <stdio.h>

#define DEBUG_MODE 1 //////////////// 1 - on debug, 0 - off debug

#if DEBUG_MODE
    #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif


/* ========================================================================== */
/*                           PIN ENCODER                                      */
/* ========================================================================== */
#define ENC_PORT GPIOB
#define ENC_A_PIN GPIO_PIN_0
#define ENC_B_PIN GPIO_PIN_1
#define ENC_BTN_PIN GPIO_PIN_2


/* ========================================================================== */
/*                     CONST AND SETTINGS SYSTEM                              */
/* ========================================================================== */

/* ===================== CONTROL FAN ========================= */
#define FAN_OFF 0x00				//speed off fan
#define FAN_5 0x05					//speed 5 fan
#define FAN_6 0x06					//speed 6 fan



/* ===================== WARNING INDEXES IN THE ARRAY ========================= */
#define WARNING_CAN      0
#define WARNING_ENGINE   1
#define WARNING_BATTERY  2




/* ========================================================================== */
/*              CURRENT DATA FROM SENSORS (TEMPERATURE)                       */
/* ========================================================================== */

int16_t temp_engine = 0;
int8_t temp_bat_min = 0;
int8_t temp_bat_max = 0;





/* ========================================================================== */
/*                     FAN CONDITION AND CONTROL                              */
/* ========================================================================== */

uint8_t fan_mode = 0; 							//current state fun 0 - one transmit, 1 - period transmit
uint8_t fan_speed = 0;							//current speed fan
uint8_t balance_mode = 0;   					// 0 - normal mode, 1 - balance mode

uint32_t last_fan_time = 0;						//timer sent command fan
const uint32_t timeTransiveFanSpeed = 500;		//ms





/* ========================================================================== */
/*                   CAN Interface and Data Exchange                          */
/* ========================================================================== */

CAN_HandleTypeDef hcan;


volatile uint8_t can_activity_flag = 0;
volatile uint8_t can_rx_flag = 0;
volatile uint8_t can_bus_error = 0;
uint8_t old_can_error = 0;


volatile uint32_t can_last_time = 0;				// timer last recieve can packet
uint32_t time_check_status = 1000;					// period check status CAN in ms


/* ================ var request temp engine ============ */
uint32_t last_request_time = 0;
const uint32_t request_period = 1000;


/* ======================= data temp request transmit =================================== */
uint8_t data_temp_engine[8] = {0x02, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};





/* ========================================================================== */
/*                WARNING SYSTEM AND SCREEN (LCD / LED)                       */
/* ========================================================================== */


/* ======================= Emergency flags =================================== */
volatile uint8_t engine_hot = 0;
volatile uint8_t battery_hot = 0;


/* ================= On-screen alert carousel manager ======================== */
typedef struct
{
	uint8_t active;
	const char *line1;
	const char *line2;
} Warning_t;

Warning_t warnings[] =
{
	{0, "CAN BUS ERROR"},
	{0, "ENGINE HOT!!!"},
	{0, "BATTERY HOT!!!"}
};


uint8_t warning_count = 0;				//active warning
uint8_t warning_index = 0;				//the displayed warning is currently being displayed
uint32_t last_warning_time = 0;			//on-screen alert toggle timer
uint16_t time_update_warning = 3000;


/* =========================== Display rendering settings ===================== */
uint32_t last_lcd_update = 0;
const uint32_t lcd_update_period = 500;
char lcd_buffer[32]; // bufer


/* ================= LED TIMER ================= */
const uint32_t led_timer = 50;


/* ========================================================================== */
/*            			    	SETTINGS     		                  		  */
/* ========================================================================== */
typedef struct
{
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

} Settings_t;

Settings_t settings =
{
		.engine_overheat_temp_up = 105,
		.engine_overheat_temp_low = 100,
		.battery_overheat_temp_up = 45,
		.battery_overheat_temp_low = 40,
		.temp_bat_off = 34,
		.temp_bat_speed5 = 35,
		.temp_bat_speed6 = 36,
		.upped_diff = 8,
		.lower_diff = 3
};




/* ========================================================================== */
/*            			    FUNCTION PROTOTYPES     		                  */
/* ========================================================================== */


//Initialization and low-level work with LCD
void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_String(char *str);


// Logic for screen updating and warning output
void LCD_Update(void);
void Check_Warnings(void);
void LCD_ShowWarnings(void);
void LCD_PrintWarningValue(void);
void LCD_LoadCustomRU(void);


// CAN bus operation logic
void CAN_Check_Status(void);
void engine_request(void);


// Fan control and indication logic
uint8_t get_target_fan_speed(void);
void update_balance_mode(void);
void fan_send_command(uint8_t speed);
void fan_control(void);
void update_led(void);





/* ================= SWO PRINTF ================= */
#if DEBUG_MODE
int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        ITM_SendChar(*ptr++);
    }
    return len;
}
#endif


/* ================= RX CALLBACK (INTERRUPT) ================= */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK)
    {
        DEBUG_PRINT("DATA EROR!!!!\r\n");
        DEBUG_PRINT("\r\n");
        DEBUG_PRINT("\r\n");
        return;
    }

	#if DEBUG_MODE
    DEBUG_PRINT("ID:%03lX DLC:%lu DATA:", rxHeader.StdId, rxHeader.DLC);

            for (int i = 0; i < rxHeader.DLC; i++)
            {
            	DEBUG_PRINT(" %02X", rxData[i]);
            }

            DEBUG_PRINT("\r\n");
            DEBUG_PRINT("\r\n");
	#endif


////////////////////////// processing for ID 0x3CB, 0x7E8 /////////////////////////////
    if (rxHeader.StdId == 0x3CB && rxHeader.DLC >= 7)
    {
    	temp_bat_max = rxData[4];
    	temp_bat_min = rxData[5];

    	DEBUG_PRINT("TEMP BAT_MAX: = %d C\r\n", temp_bat_max);
    	DEBUG_PRINT("TEMP BAT_MIN: = %d C\r\n", temp_bat_min);
    	DEBUG_PRINT("\r\n");
    }

    if (rxHeader.StdId == 0x7E8 && rxHeader.DLC > 4 && rxData[0] == 0x03 && rxData[1] == 0x41 && rxData[2] == 0x05)
        {
    		temp_engine =  (int32_t)rxData[3] - 40;

            DEBUG_PRINT("t_engine: = %d C\r\n", temp_engine);
            DEBUG_PRINT("\r\n");
        }

    can_rx_flag = 1;

    can_activity_flag = 1;
    can_bus_error = 0;
    can_last_time = HAL_GetTick();
}

void CAN_SendRequest(uint16_t id, uint8_t len, uint8_t *data)
{
	CAN_TxHeaderTypeDef txHeader;
	uint32_t mailbox;

	txHeader.StdId = id;
	txHeader.ExtId = 0;
	txHeader.IDE = CAN_ID_STD;
	txHeader.RTR = CAN_RTR_DATA;
	txHeader.DLC = len;
	txHeader.TransmitGlobalTime = DISABLE;

	if (HAL_CAN_AddTxMessage(&hcan, &txHeader, data, &mailbox) != HAL_OK)
	    {
			DEBUG_PRINT("TX ERROR\r\n");
			can_bus_error = 1;
	    }
}

/* ================= CLOCK ================= */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ================= GPIO ================= */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    gpio.Pin =
        GPIO_PIN_0 |
        GPIO_PIN_1 |
        GPIO_PIN_2 |
        GPIO_PIN_3 |
        GPIO_PIN_4 |
        GPIO_PIN_5;

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;


    HAL_GPIO_Init(GPIOC, &gpio);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // OFF
}

/* ================= CAN INIT ================= */
static void MX_CAN_Init(void)
{
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    GPIO_InitTypeDef gpio;

    /* RX PA11 */
    gpio.Pin = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* TX PA12 */
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    hcan.Instance = CAN1;
    hcan.Init.Prescaler = 4;
    hcan.Init.Mode = CAN_MODE_NORMAL;
    hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
    hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan.Init.AutoBusOff = DISABLE;
    hcan.Init.AutoWakeUp = DISABLE;
    hcan.Init.AutoRetransmission = ENABLE;
    hcan.Init.ReceiveFifoLocked = DISABLE;
    hcan.Init.TransmitFifoPriority = DISABLE;

    HAL_CAN_Init(&hcan);
}

/* ================= FILTER (ACCEPT ALL) ================= */
static void CAN_Filter_Config(void)
{
	CAN_FilterTypeDef sFilterConfig;

	sFilterConfig.FilterBank = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.SlaveStartFilterBank = 14;

	// ---- ID1: 0x7E8 (temp engine)
	sFilterConfig.FilterIdHigh = (0x7E8 << 5) & 0xFFFF;

	// ---- ID2: 0x3CB (temp battery)
	sFilterConfig.FilterIdLow = (0x3CB << 5) & 0xFFFF;

	HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);

}

/* ================= MAIN ================= */
int main(void)
{
	HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_CAN_Init();
    LCD_Init();

	CAN_Filter_Config();


	LCD_Command(0x01);  // clear display
	HAL_Delay(10);		// wait

	LCD_SetCursor(0,2);
	LCD_String("Starting...");
	HAL_Delay(1500);


	if(HAL_CAN_Start(&hcan) != HAL_OK)
	{
	    LCD_Command(0x01);
	    LCD_SetCursor(0,3);
	    LCD_String("CAN START ERR");

	    DEBUG_PRINT("CAN STATE ERR\r\n");

	    while(1);
	}


	if(HAL_CAN_GetState(&hcan) == HAL_CAN_STATE_LISTENING)
	{
	    LCD_Command(0x01);
	    HAL_Delay(10);

	    LCD_SetCursor(0,2);
	    LCD_String("CAN START OK");

	    DEBUG_PRINT("STM32 CAN READY\r\n");
	    DEBUG_PRINT("\r\n");
	    DEBUG_PRINT("\r\n");

	    HAL_Delay(1500);
	}
	else
	{
	    LCD_Command(0x01);
	    LCD_SetCursor(0,3);
	    LCD_String("CAN STATE ERR");

	    DEBUG_PRINT("CAN STATE ERR\r\n");

	    while(1);
	}

	/* NVIC */
	HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	/* ENABLE RX INTERRUPT */
	HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);


	LCD_Command(0x01);
	HAL_Delay(10);


    while (1)
    {
		engine_request();
		update_balance_mode();
		fan_control();
		update_led();

		CAN_Check_Status();
		Check_Warnings();

		LCD_Update();

    }
}

void LCD_Update(void)
{
    uint32_t current_time = HAL_GetTick();

    if(current_time - last_lcd_update < lcd_update_period)
        return;

    last_lcd_update = current_time;


    if(warning_count > 0)
    {
        LCD_ShowWarnings();
        return;
    }


    sprintf(lcd_buffer, "t:%3d%cC Bmin:%d", temp_engine, 0xDF, temp_bat_min);

    LCD_SetCursor(0,0);
    LCD_String(lcd_buffer);


    sprintf(lcd_buffer, "Fan:%d   Bmax:%2d", fan_speed, temp_bat_max);

    LCD_SetCursor(1,0);
    LCD_String(lcd_buffer);
}

void CAN_Check_Status(void)
{
   uint32_t current_time = HAL_GetTick();


   if(can_activity_flag == 1)
	   if((current_time - can_last_time) > time_check_status)
		   can_bus_error = 1;
}

void update_balance_mode(void)
{
	uint8_t temp_diff = (temp_bat_max >= temp_bat_min) ? (temp_bat_max - temp_bat_min) : (temp_bat_min - temp_bat_max);

	if(temp_diff >= settings.upped_diff)
	        balance_mode = 1;
	    else if (balance_mode && (temp_diff <= settings.lower_diff))
	        balance_mode = 0;
}

uint8_t get_target_fan_speed(void)
{
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

void fan_send_command(uint8_t speed)
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

void fan_control(void)
{
	uint8_t target_speed = get_target_fan_speed();

	if(target_speed == FAN_OFF && fan_mode != 0)
	{
		fan_send_command(FAN_OFF);
		return;
	}

	if (HAL_GetTick() - last_fan_time >= timeTransiveFanSpeed)
	{
		last_fan_time = HAL_GetTick();
		fan_send_command(target_speed);
	}

}

void engine_request(void)
{
/* ============================ SEND Request temp engine ======================= */
    if (HAL_GetTick() - last_request_time >= request_period)
    {
        last_request_time = HAL_GetTick();

        CAN_SendRequest(0x7DF, 8, data_temp_engine);
    }
}

void update_led(void)
{
    static uint32_t last_blink_time = 0;
    static uint8_t led_state = 0;
    uint32_t current_time = HAL_GetTick();


    if (led_state == 0)
    {
        if (can_rx_flag == 1)
        {
            can_rx_flag = 0;
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            led_state = 1;
            last_blink_time = current_time;
        }
    }

    else if (led_state == 1)
    {
        if (current_time - last_blink_time >= led_timer)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            led_state = 2;
            last_blink_time = current_time;
        }
    }

    else if (led_state == 2)
    {
        if (current_time - last_blink_time >= led_timer)
        {
            can_rx_flag = 0;
            led_state = 0;
        }
    }
}

void Check_Warnings(void)
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

void LCD_ShowWarnings(void)
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

    LCD_PrintWarningValue();
}

void LCD_PrintWarningValue(void)
{
	 LCD_SetCursor(1,0);

	switch(warning_index)
	{
	case WARNING_ENGINE:
		sprintf(lcd_buffer, "TEMP: %3d%cC        ", temp_engine, 0xDF);
			break;

	case WARNING_BATTERY:
		sprintf(lcd_buffer, "TEMP: %3d%cC        ", temp_bat_max, 0xDF);
			break;

	case WARNING_CAN:
		sprintf(lcd_buffer, "CHECK CAN BUS    ");
			break;
	}

	LCD_String(lcd_buffer);
}



