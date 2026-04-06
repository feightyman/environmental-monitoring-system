#include "header.h"

#define debounce_time 20
#define long_press_time 1000
#define double_click_time 0

void key_init(Key_HandleTypeDef* key,GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin)
{
	key->click_cnt=0;
	key->event_flag=event_none;
	key->first_release_time=0;
	key->last_time=0;
	key->long_press_flag=false;
	key->pin=GPIO_Pin;
	key->port=GPIOx;
	key->state=state_idle;
}

void key_scan(Key_HandleTypeDef* key,uint32_t current_time)
{
	uint8_t level=HAL_GPIO_ReadPin(key->port,key->pin);
	switch(key->state)
	{
		case state_idle:
			if(key->click_cnt==1&&(current_time-key->first_release_time>=double_click_time))
			{
				key->event_flag|=event_single_click;
				key->click_cnt=0;
			}
			if(level==0)
			{
				key->state=state_debounce;
				key->last_time=current_time;
			}
		break;
		case state_debounce:
			if(current_time-key->last_time>=debounce_time)
			{
				if(level==0)
				{
					key->event_flag=event_press_down;
					key->state=state_pressed;
				}
				else
					key->state=state_idle;
			}
		break;
		case state_pressed:
			if(level==1)
			{
				key->state=state_released;
				key->last_time=current_time;
				if(!key->long_press_flag)
				{
					key->click_cnt++;
				}
			}
			else if(current_time-key->last_time>=long_press_time)
			{
				key->event_flag|=event_long_press;
				key->long_press_flag=true;
			}
		break;
		case state_released:
			if(current_time-key->last_time>=debounce_time)
			{
				key->event_flag|=event_release;
				if(key->long_press_flag)
				{
					key->long_press_flag=false;
					key->click_cnt=0;
				}
				else
				{
					if(key->click_cnt==1)
					{
						key->first_release_time=current_time;
					}
					else if(key->click_cnt>=2)
					{
						key->event_flag|=event_double_click;
						key->click_cnt=0;
					}
				}
				key->state=state_idle;
			}
		break;
	}
}

KeyEventType KeyGetEvent(Key_HandleTypeDef* key)
{
	return (KeyEventType)key->event_flag;
}

void KeyClearEvent(Key_HandleTypeDef* key,KeyEventType event)
{
	key->event_flag&=~event;
}







