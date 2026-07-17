/*
 * can.c
 *
 *  Created on: Jul 17, 2026
 *      Author: Pavel
 */
#include "can.h"
#include "main.h"
#include "lcd.h"
#include <stdio.h>




/* ========================================================================== */
/*                   CAN Interface and Data Exchange                          */
/* ========================================================================== */


CAN_HandleTypeDef hcan;


int16_t temp_engine;
uint8_t temp_bat_min;
uint8_t temp_bat_max;

volatile uint8_t can_activity_flag = 0;
volatile uint8_t can_rx_flag = 0;
volatile uint8_t can_bus_error = 0;


static volatile uint32_t can_last_time = 0;				// timer last recieve can packet
static const uint32_t time_check_status = 1000; 		// period check status CAN in ms


static const uint32_t request_period = 1000;
static uint32_t last_request_time = 0;
uint8_t data_temp_engine[8] = {0x02, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};



void CAN_Init(void)
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

void CAN_FilterConfig(void)
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
	sFilterConfig.FilterMaskIdHigh = 0x0000;

	// ---- ID2: 0x3CB (temp battery)
	sFilterConfig.FilterIdLow = (0x3CB << 5) & 0xFFFF;
	sFilterConfig.FilterMaskIdLow = 0x0000;

	HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);

}

void CAN_CheckStatus(void)
{
   uint32_t current_time = HAL_GetTick();

   if(can_activity_flag == 1)
	   if((current_time - can_last_time) > time_check_status)
		   can_bus_error = 1;
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

void CAN_RequestEngineTemp(void)
{
    if (HAL_GetTick() - last_request_time >= request_period)
    {
        last_request_time = HAL_GetTick();

        CAN_SendRequest(0x7DF, 8, data_temp_engine);
    }
}

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

void CAN_StartAndCheck(void)
{
    LCD_Command(0x01);
    HAL_Delay(10);

    LCD_SetCursor(0, 2);
    LCD_String("Starting...");
    HAL_Delay(1500);

    if(HAL_CAN_Start(&hcan) != HAL_OK)
    {
        LCD_Command(0x01);
        LCD_SetCursor(0, 3);
        LCD_String("CAN START ERR");

        DEBUG_PRINT("CAN STATE ERR\r\n");
        while(1);
    }


    if(HAL_CAN_GetState(&hcan) == HAL_CAN_STATE_LISTENING)
    {
        LCD_Command(0x01);
        HAL_Delay(10);

        LCD_SetCursor(0, 2);
        LCD_String("CAN START OK");

        DEBUG_PRINT("STM32 CAN READY\r\n");
        HAL_Delay(1500);
    }
    else
    {
        LCD_Command(0x01);
        LCD_SetCursor(0, 3);
        LCD_String("CAN STATE ERR");

        DEBUG_PRINT("CAN STATE ERR\r\n");
        while(1);
    }
}

