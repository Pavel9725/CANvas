#ifndef INC_MENU_H_
#define INC_MENU_H_

#include "stm32f1xx_hal.h"

void Menu_Init(void);
void Menu_Process(void);
uint8_t Menu_IsActive(void);
static void Menu_Show(void);




#endif /* INC_MENU_H_ */
