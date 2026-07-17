#include "encoder.h"

/////////////////////////////////////////////////////////////////ИСПРАВИТЬ ОБРАБОТКУ НАЖАТИЙ ЭНКОДЕРА!
static TIM_HandleTypeDef *htim_encoder = NULL;
static uint16_t last_encoder_value = 0;


static uint8_t button_pressed_event = 0;
static uint8_t button_hold_event = 0;

static uint8_t button_state = 0;
static uint32_t button_time = 0;



void Encoder_Init(void)
{
    extern TIM_HandleTypeDef htim3;
    htim_encoder = &htim3;

    HAL_TIM_Encoder_Start(htim_encoder, TIM_CHANNEL_ALL);


    __HAL_TIM_SET_COUNTER(htim_encoder, 0x8000);
    last_encoder_value = 0x8000;
}


int8_t Encoder_Read(void)
{
    if(htim_encoder == NULL) return 0;

    uint16_t now = __HAL_TIM_GET_COUNTER(htim_encoder);
    int16_t delta = (int16_t)(now - last_encoder_value);
    last_encoder_value = now;


    if(delta >= 2) return 1;
    if(delta <= -2) return -1;
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
    if(button_pressed_event)
    {
        button_pressed_event = 0;
        return 1;
    }

    return 0;
}


uint8_t Encoder_Button_Held(uint32_t hold_time_ms)
{
    (void)hold_time_ms;

    if(button_hold_event)
    {
        button_hold_event = 0;
        return 1;
    }

    return 0;
}

void Encoder_Update(void)
{
    uint8_t pin = HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port, ENC_BTN_Pin);

    switch(button_state)
    {
        // Кнопка отпущена
        case 0:

            if(pin == GPIO_PIN_RESET)
            {
                button_state = 1;
                button_time = HAL_GetTick();
            }

            break;

        // Ожидание антидребезга
        case 1:

            if(HAL_GetTick() - button_time >= 20)
            {
                if(pin == GPIO_PIN_RESET)
                {
                    button_pressed_event = 1;
                    button_state = 2;
                    button_time = HAL_GetTick();
                }
                else
                {
                    button_state = 0;
                }
            }

            break;

        // Кнопка удерживается
        case 2:

            if(pin == GPIO_PIN_SET)
            {
                button_state = 0;
            }
            else
            {
                if(HAL_GetTick() - button_time >= 5000)
                {
                    button_hold_event = 1;
                    button_state = 3;
                }
            }

            break;

        // Долгое удержание уже произошло
        case 3:

            if(pin == GPIO_PIN_SET)
            {
                button_state = 0;
            }

            break;
    }
}
