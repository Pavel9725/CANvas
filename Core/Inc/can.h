#ifndef INC_CAN_H_
#define INC_CAN_H_

#include "stm32f1xx_hal.h"


extern int16_t temp_engine;
extern uint8_t temp_bat_min;
extern uint8_t temp_bat_max;
extern volatile uint8_t can_rx_flag;
extern volatile uint8_t can_bus_error;

extern CAN_HandleTypeDef hcan;

void CAN_Init(void);
void CAN_FilterConfig(void);
void CAN_SendRequest(uint16_t id, uint8_t len, uint8_t *data);
void CAN_CheckStatus(void);
void CAN_RequestEngineTemp(void);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
void CAN_StartAndCheck(void);


#endif
