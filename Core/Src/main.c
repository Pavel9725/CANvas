#include "main.h"
#include "lcd.h"
#include "encoder.h"
#include "settings.h"
#include "menu.h"
#include "fan.h"
#include "can.h"
#include "warnings.h"
#include <stdio.h>


TIM_HandleTypeDef htim3;


/* ======================= Emergency flags =================================== */
volatile uint8_t engine_hot = 0;
volatile uint8_t battery_hot = 0;


/* ================= LED TIMER ================= */
const uint32_t led_timer = 50;


/* ========================================================================== */
/*            			    FUNCTION PROTOTYPES     		                  */
/* ========================================================================== */

void SystemClock_Config(void);
static void MX_TIM3_Init(void);
static void MX_GPIO_Init(void);
void Update_Led(void);


/* ================= SWO PRINTF ================= */
//#if DEBUG_MODE
int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        ITM_SendChar(*ptr++);
    }
    return len;
}
//#endif



int main(void)
{
	HAL_Init();
    SystemClock_Config();

    Settings_Load();

    MX_GPIO_Init();
    CAN_Init();
    LCD_Init();
    Warnings_Init();

    MX_TIM3_Init();
    Encoder_Init();
    Menu_Init();


	CAN_FilterConfig();


	CAN_StartAndCheck();


	/* NVIC */
	HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	/* ENABLE RX INTERRUPT */
	HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);


	LCD_Command(0x01);
	HAL_Delay(5);



    while (1)
    {
		CAN_RequestEngineTemp();
		Fan_UpdateBalanceMode();
		Fan_Control();
		Update_Led();
		CAN_CheckStatus();
		Warnings_Check();

    	Encoder_Button_Update();

    	Menu_Process();

    	LCD_Update();

    	HAL_Delay(10);


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

void Update_Led(void)
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


static void MX_TIM3_Init(void)
{
	__HAL_RCC_TIM3_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();

	__HAL_AFIO_REMAP_TIM3_PARTIAL();

	TIM_Encoder_InitTypeDef sConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 65535;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
	sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC1Filter = 12;
	sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC2Filter = 12;
	if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
}



/* ================= GPIO ================= */
static void MX_GPIO_Init(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

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


    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // OFF


    gpio.Pin = GPIO_PIN_13;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &gpio);

	gpio.Pin = ENC_A_Pin | ENC_B_Pin;
	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &gpio);

	gpio.Pin = ENC_BTN_Pin;
	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(ENC_BTN_GPIO_Port, &gpio);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

