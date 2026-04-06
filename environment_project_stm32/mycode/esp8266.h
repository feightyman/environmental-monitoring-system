#ifndef _ESP01S_H_
#define _ESP01S_H_


#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志

void ESP01S_Init(void);
void ESP01S_Clear(void);
void ESP8266_SendData(unsigned char *data, unsigned short len);
unsigned char *ESP8266_GetIPD(unsigned short timeOut);
//void esp_Get_Data(void);
void Esp_PUB(void);
void lcd_proc(void);
void ESP8266_GetCMD(unsigned char *CMD);
typedef enum{
	Data,
	Para
}Interface;
typedef struct {
    double dark_threshold;   // 暗区阈值电压 (如0.5V)
    double dark_max_lux;     // 暗区最大lux (如100 lux)
    double medium_threshold; // 中等区阈值 (如2.0V)
    double medium_max_lux;   // 中等区最大lux (如1000 lux)
    double bright_slope;     // 强光区斜率 (如500 lux/V)
} LDRCalib;
void all_init(void);
void led_set(uint8_t led_num,bool state);
void led_turn(uint8_t led_num);

void uart_proc(void);
void key_proc(void);
void sig_proc(void);
void motor_ctrl(uint8_t duty);
void pubcmd(bool cmd_ctrl);
void pubcmd_illThreshold(char* number);
void pubcmd_tempThreshold(char* number);
#endif

