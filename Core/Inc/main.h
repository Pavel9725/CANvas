#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#define DEBUG_MODE 0 //////////////// 1 - on debug, 0 - off debug

#if DEBUG_MODE
    #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif


void Error_Handler(void);


#ifdef __cplusplus
}
#endif

#endif
