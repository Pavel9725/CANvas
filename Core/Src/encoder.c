#include "encoder.h"


static TIM_HandleTypeDef *htim_encoder = NULL;
static uint16_t last_encoder_value = 0;
static uint32_t last_button_time = 0;



void Encoder_Init(void)
{

    extern TIM_HandleTypeDef htim3;
    htim_encoder = &htim3;

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_TIM3_PARTIAL();


    HAL_TIM_Encoder_Start(htim_encoder, TIM_CHANNEL_ALL);


    __HAL_TIM_SET_COUNTER(htim_encoder, 0x8000);
    last_encoder_value = 0x8000;


    last_button_time = HAL_GetTick();
}


int8_t Encoder_Read(void)
{
    if(htim_encoder == NULL) return 0;

    uint16_t now = __HAL_TIM_GET_COUNTER(htim_encoder);
    int16_t delta = now - last_encoder_value;
    last_encoder_value = now;


    if(delta > 1000) delta -= 0xFFFF;
    if(delta < -1000) delta += 0xFFFF;


    if(delta > 2) return 1;
    if(delta < -2) return -1;
    return 0;
}


void Encoder_Reset(void)
{
    if(htim_encoder == NULL) return;

    __HAL_TIM_SET_COUNTER(htim_encoder, 0x8000);
    last_encoder_value = 0x8000;
}


int16_t Encoder_GetValue(void)
{
    if(htim_encoder == NULL) return 0;

    uint16_t now = __HAL_TIM_GET_COUNTER(htim_encoder);
    return (int16_t)(now - 0x8000);
}


uint8_t Encoder_Button_Pressed(void)
{
    uint32_t now = HAL_GetTick();


    if(now - last_button_time < 50) return 0;


    if(HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port, ENC_BTN_Pin) == GPIO_PIN_RESET)
    {
        last_button_time = now;
        return 1;
    }

    return 0;
}


uint8_t Encoder_Button_Held(uint32_t hold_time_ms)
{
    static uint32_t press_start_time = 0;
    static uint8_t was_pressed = 0;

    uint8_t current_state = HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port, ENC_BTN_Pin);

    if(current_state == GPIO_PIN_RESET)
    {
        if(!was_pressed)
        {
            was_pressed = 1;
            press_start_time = HAL_GetTick();
        }
        else if(HAL_GetTick() - press_start_time >= hold_time_ms)
        {
            was_pressed = 0;
            return 1;
        }
    }
    else
    {
        was_pressed = 0;
    }

    return 0;
}
