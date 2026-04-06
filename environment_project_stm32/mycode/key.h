#ifndef __KEY_H
#define __KEY_H


typedef enum{
	event_none=0x00,
	event_press_down=0x01,
	event_single_click=0x02,
	event_double_click=0x04,
	event_long_press=0x08,
	event_release=0x10
}KeyEventType;

typedef enum{
	state_idle,
	state_debounce,
	state_pressed,
	state_released
}KeyState;

typedef struct{
	GPIO_TypeDef* port;
	uint16_t pin;
	KeyState state;
	uint32_t last_time;
	uint32_t first_release_time;
	uint8_t event_flag;
	uint8_t click_cnt;
	bool long_press_flag;
}Key_HandleTypeDef;


void key_init(Key_HandleTypeDef* key,GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin);
void key_scan(Key_HandleTypeDef* key,uint32_t current_time);
KeyEventType KeyGetEvent(Key_HandleTypeDef* key);
void KeyClearEvent(Key_HandleTypeDef* key,KeyEventType event);


#endif

