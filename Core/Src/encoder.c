#include "encoder.h"


#define BUTTON_DEBOUNCE_TIME    50
#define BUTTON_LONG_TIME      3000
#define BUTTON_RESET_TIME    10000



static TIM_HandleTypeDef *htim_encoder = NULL;
static uint16_t last_encoder_value = 0;
static int16_t encoder_accumulator = 0;



Encoder_t Encoder;

void Encoder_Init(void)
{
    extern TIM_HandleTypeDef htim3;
    htim_encoder = &htim3;

    HAL_TIM_Encoder_Start(htim_encoder, TIM_CHANNEL_ALL);


    __HAL_TIM_SET_COUNTER(htim_encoder, 0x8000);
    last_encoder_value = 0x8000;

	Encoder.state = 0;
	Encoder.last_state = 0;
	Encoder.debounce_time = 0;
	Encoder.press_time = 0;
	Encoder.pressed = 0;
	Encoder.event = BTN_NONE;
	Encoder.long3_triggered = 0;
	Encoder.long10_triggered = 0;


}


int8_t Encoder_Read(void)
{
    if(htim_encoder == NULL)
        return 0;


    uint16_t now = __HAL_TIM_GET_COUNTER(htim_encoder);

    int16_t delta = (int16_t)(now - last_encoder_value);

    last_encoder_value = now;


    encoder_accumulator += delta;


    if(encoder_accumulator >= 4)
    {
        encoder_accumulator = 0;
        return 1;
    }


    if(encoder_accumulator <= -4)
    {
        encoder_accumulator = 0;
        return -1;
    }


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

uint8_t Encoder_Button_Debounce(void)
{

	uint32_t now = HAL_GetTick();

	uint8_t state = (HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port, ENC_BTN_Pin) == GPIO_PIN_RESET);

	if(state != Encoder.last_state)
	{

		Encoder.debounce_time = now;
		Encoder.last_state = state;
	}

	if((now - Encoder.debounce_time) < BUTTON_DEBOUNCE_TIME)
		return Encoder.state;

	Encoder.state = state;

	return Encoder.state;
}

void Encoder_Button_Update(void)
{
	uint8_t current_state = Encoder_Button_Debounce();
	uint32_t now = HAL_GetTick();

	if(current_state)
	{
		if(Encoder.press_time == 0)
		{
			Encoder.press_time = now;
			Encoder.pressed = 1;
			Encoder.event = BTN_NONE;
			Encoder.long3_triggered = 0;
			Encoder.long10_triggered = 0;
		}

		uint32_t press = now - Encoder.press_time;

		if(press >= BUTTON_RESET_TIME && (!Encoder.long10_triggered))
		{
			Encoder.event = BTN_RESET;
			Encoder.long10_triggered = 1;
			printf("RESET!\n");
		}
		else if(press >= BUTTON_LONG_TIME && (!Encoder.long3_triggered))
		{
			Encoder.long3_triggered = 1;

		}
	}
	else
	{
		if(Encoder.pressed)
		{
			uint32_t press = now - Encoder.press_time;

			if(Encoder.long10_triggered == 0)
			{
				if(Encoder.long3_triggered == 1)
				{
					Encoder.event = BTN_LONG;
					printf("LONG 3!\n");
				}
				else if(press > BUTTON_DEBOUNCE_TIME)
				{
					Encoder.event = BTN_SHORT;
					printf("SHORT!\n");
				}
			}

			Encoder.pressed = 0;
			Encoder.press_time = 0;
			Encoder.long3_triggered = 0;
		}
	}
}

ButtonEvent_t Encoder_GetEvent(void)
{
    ButtonEvent_t event = Encoder.event;

    Encoder.event = BTN_NONE;

    return event;
}

