#include "header.h"


#define ESP01S_WIFI_INFO		"AT+CWJAP=\"MySSID\",\"12356789\"\r\n"
const char* ClintID="test";
const char* username="ch40Qpq8G4";
const char* passwd="version=2018-10-31&res=products%2Fch40Qpq8G4%2Fdevices%2Ftest&et=1956499200&method=md5&sign=n8jt2%2FRs2QIX7UKIRPHtVA%3D%3D";
const char* Url="mqtts.heclouds.com";
const char* pubtopic="$sys/ch40Qpq8G4/test/thing/property/post";
const char* subtopic="$sys/ch40Qpq8G4/test/thing/property/post/reply";
const char* sub2topic="$sys/ch40Qpq8G4/test/thing/property/set";
const char* func[]={"LightLux","EnvironmentHumidity","EnvironmentTemperature","aqi","lightThreshold","tempThreshold","CMD"};
extern uint8_t humi,humi_dec,temp,temp_dec;
uint16_t temp_hh=50,ill_ll=100,pub_tick=2000,ill;
char ESP01S_buf[1024],buf[25],aqi[10]="\"Good\"";//Good or Moderate
char cmdBuf[1024];
unsigned short ESP01S_cnt = 0, ESP01S_cntPre = 0;
bool change_flag;
char *cmd_state="false";
Interface interface;
Key_HandleTypeDef key[4];
char rx_buf[1024];
volatile uint16_t rx_index;
bool rx_flag,exch;
uint16_t ADC_Value[2]={0};
double V_Value[2]={0};
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim16;
uint8_t motor_duty;

const LDRCalib CALIB_FACTORS = {
    .dark_threshold = 0.5,    // 暗光区最大电压（如0.5V对应100 lux）
    .dark_max_lux = 100.0,
    .medium_threshold = 2.0,  // 中等区最大电压（如2.0V对应1000 lux）
    .medium_max_lux = 1000.0,
    .bright_slope = 500.0     // 强光区每伏特增加500 lux
};

uint8_t aRxBuffer;			//接收中断缓冲
//==========================================================
//	函数名称：	ESP01S_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP01S_Clear(void)
{

	memset(ESP01S_buf, 0, sizeof(ESP01S_buf));
	ESP01S_cnt = 0;

}

//==========================================================
//	函数名称：	ESP01S_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP01S_WaitRecive(void)
{

	if(ESP01S_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(ESP01S_cnt == ESP01S_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		ESP01S_cnt = 0;							//清0接收计数
			
		return REV_OK;								//返回接收完成标志
	}
		
	ESP01S_cntPre = ESP01S_cnt;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP01S_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP01S_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 200;

	Usart_SendString(huart3, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP01S_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)ESP01S_buf, res) != NULL)		//如果检索到关键词
			{
				ESP01S_Clear();									//清空缓存
				 
				return 0;
			}
		}
		
		HAL_Delay(10);
	}
	
	return 1;

}


//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP01S_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)ESP01S_buf, "+MQTTSUBRECV");				//搜索“IPD”头
//			ptrIPD = strstr((char *)ESP01S_buf, "CMD");
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
				UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		
		HAL_Delay(5);													//延时等待
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针

}

//==========================================================
//	函数名称：	ESP01S_Init
//
//	函数功能：	初始化ESP01S
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP01S_Init(void)
{
	
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_11);
//	HAL_Delay(100);
//	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_11);
	ESP01S_Clear();
	
	UsartPrintf(USART_DEBUG, "0. AT\r\n");
	sprintf(buf,"     0. AT            ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	while(ESP01S_SendCmd("AT\r\n", "OK"))
	sprintf(buf,"     OK               ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
		HAL_Delay(500);
	
	UsartPrintf(USART_DEBUG, "1. RST\r\n");
	sprintf(buf,"     1. RST          ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	ESP01S_SendCmd("AT+RST\r\n", "");
	sprintf(buf,"     OK               ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
        HAL_Delay(500);	
	
	
	UsartPrintf(USART_DEBUG, "2. CWMODE\r\n");
	sprintf(buf,"     2. CWMODE        ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	while(ESP01S_SendCmd("AT+CWMODE=1\r\n", "OK"))     //模式1(Station),默认保存Flash
	sprintf(buf,"     OK               ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
		HAL_Delay(500);
	
	UsartPrintf(USART_DEBUG, "3. AT+CWDHCP\r\n");       //开启DHCP(获取分配IP),默认保存Flash
	sprintf(buf,"     3. AT+CWDHCP     ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	while(ESP01S_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
	sprintf(buf,"     OK               ");		
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
		HAL_Delay(500);
	
	UsartPrintf(USART_DEBUG, "4. CWJAP\r\n");           //链接WIFI
	sprintf(buf,"     4. CWJAP         ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	while(ESP01S_SendCmd(ESP01S_WIFI_INFO, "OK"))
	sprintf(buf,"     OK               ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
		HAL_Delay(500);

	
	UsartPrintf(USART_DEBUG, "5.MQTTUSERCFG\r\n");           //用户信息配置
	sprintf(buf,"     5.MQTTUSERCFG    ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	sprintf(cmdBuf,"AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", ClintID,username,passwd);
	while(ESP01S_SendCmd(cmdBuf, "OK"))
	sprintf(buf,"     OK               ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	HAL_Delay(500);
	memset(cmdBuf,0,sizeof(cmdBuf));
	
	UsartPrintf(USART_DEBUG, "6.MQTTCONN\r\n");           //连接服务器
	sprintf(buf,"     6.MQTTCONN      ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	sprintf(cmdBuf,"AT+MQTTCONN=0,\"%s\",1883,1\r\n",Url);
	while(ESP01S_SendCmd(cmdBuf, "OK"))
	sprintf(buf,"     OK              ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	HAL_Delay(500);
	memset(cmdBuf,0,sizeof(cmdBuf));
	
	
	UsartPrintf(USART_DEBUG, "7.MQTTSUB\r\n");           //订阅消息
	sprintf(buf,"     7.MQTTSUB       ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	sprintf(cmdBuf,"AT+MQTTSUB=0,\"%s\",1\r\n",subtopic);
	while(ESP01S_SendCmd(cmdBuf, "OK"))
	sprintf(buf,"     OK              ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	HAL_Delay(500);
	memset(cmdBuf,0,sizeof(cmdBuf));
	
	UsartPrintf(USART_DEBUG, "8.MQTTSUB\r\n");           //订阅消息
	sprintf(buf,"     8.MQTTSUB       ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	sprintf(cmdBuf,"AT+MQTTSUB=0,\"%s\",1\r\n",sub2topic);
	sprintf(buf,"     OK              ");
	LCD_DisplayStringLine(Line4,(uint8_t*)buf);
	while(ESP01S_SendCmd(cmdBuf, "OK"))
	HAL_Delay(500);
	memset(cmdBuf,0,sizeof(cmdBuf));
}

//灯光控制部分

void led_set(uint8_t led_num,bool state)
{
	if(state)
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<(led_num-1),GPIO_PIN_RESET);
	else
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<(led_num-1),GPIO_PIN_SET);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_2);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_2);
}
void led_turn(uint8_t led_num)
{
	HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_8<<(led_num-1));
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_2);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_2);
}


//蓝牙控制部分

void uart_proc(void)
{
	char *ptrIPD = NULL;
	char cmdBuffer[11] = {0};
	uint8_t i;
	if(rx_flag)
	{
//		strcpy(rx_text,rx_buf);
		
		if(strcmp((const char*)rx_buf,(const char*)"ledon")==0)
		{
//			led_set(8,1);
			pubcmd(1);
			HAL_UART_Transmit(&huart2,(uint8_t*)"led ok\r\n",strlen("led ok\r\n"),100);
		}
		else if(strcmp((const char*)rx_buf,(const char*)"ledoff")==0)
		{
//			led_set(8,0);
			pubcmd(0);
			
			HAL_UART_Transmit(&huart2,(uint8_t*)"led ok\r\n",strlen("led ok\r\n"),100);
		}
		else if(strstr((const char*)rx_buf,(const char*)"settemp")!=NULL)
		{
//			led_set(8,0);
			ptrIPD=strstr((const char*)rx_buf,(const char*)"settemp");
			ptrIPD = strchr(ptrIPD, ':');							//找到':'
			if(ptrIPD != NULL)
			{
				ptrIPD++;
				for(i=0;i<10;i++)
				{
					if(ptrIPD[i]<'0'||ptrIPD[i]>'9')
					{
						cmdBuffer[i]='\0';
	//					UsartPrintf(USART_DEBUG,"cmd:%s",cmdBuffer);
						break;
					}
					cmdBuffer[i]=ptrIPD[i];
				}
				UsartPrintf(huart2,"temp:%s",cmdBuffer);
				pubcmd_tempThreshold(cmdBuffer);
			}
			HAL_UART_Transmit(&huart2,(uint8_t*)"set ok\r\n",strlen("set ok\r\n"),100);
		}
		else if(strstr((const char*)rx_buf,(const char*)"setill")!=NULL)
		{
//			led_set(8,0);
			ptrIPD=strstr((const char*)rx_buf,(const char*)"setill");
			ptrIPD = strchr(ptrIPD, ':');							//找到':'
			if(ptrIPD != NULL)
			{
				ptrIPD++;
				for(i=0;i<10;i++)
				{
					if(ptrIPD[i]<'0'||ptrIPD[i]>'9')
					{
						cmdBuffer[i]='\0';
	//					UsartPrintf(USART_DEBUG,"cmd:%s",cmdBuffer);
						break;
					}
					cmdBuffer[i]=ptrIPD[i];
				}
				UsartPrintf(huart2,"ill:%s",cmdBuffer);
				pubcmd_illThreshold(cmdBuffer);
			}
			HAL_UART_Transmit(&huart2,(uint8_t*)"set ok\r\n",strlen("set ok\r\n"),100);
//			HAL_UART_Transmit(&huart1,(uint8_t*)"ledok\r\n",strlen("ledok\r\n"),100);
		}
//		led_turn(7);
//		strcpy(tx_buf,rx_buf);
//		HAL_UART_Transmit(&huart1,(uint8_t*)rx_buf,strlen(rx_buf),50);
		
		rx_flag=false;
		memset((void*)rx_buf,0,1024);
		rx_index=0;
	}
}


//上传照明灯状态数据

void pubcmd(bool cmd_ctrl)
{
	if(cmd_ctrl)
	{
		led_set(8,1);
		sprintf(cmdBuf,"AT+MQTTPUB=0,\"%s\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%s}}}\",0,0\r\n",pubtopic,func[6],"true");
//		Usart_SendString(huart1, (unsigned char *)cmdBuf, strlen((const char *)cmdBuf));
		Usart_SendString(huart3, (unsigned char *)cmdBuf, strlen((const char *)cmdBuf));
	}
	else
	{
		led_set(8,0);
		sprintf(cmdBuf,"AT+MQTTPUB=0,\"%s\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%s}}}\",0,0\r\n",pubtopic,func[6],"false");
//		Usart_SendString(huart1, (unsigned char *)cmdBuf, strlen((const char *)cmdBuf));
		Usart_SendString(huart3, (unsigned char *)cmdBuf, strlen((const char *)cmdBuf));
	}
}

//将不定长字符串转至整数形式

int32_t string_to_int(const char *str) {
    int32_t result = 0;
    int sign = 1; // 符号位，默认为正
    
    // 跳过前导空格
    while (*str == ' ') str++;
    
    // 处理符号
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // 逐字符转换
    while (*str != '\0') {
        if (*str >= '0' && *str <= '9') {
            // 检查溢出（适用于32位系统）
            if (result > (INT32_MAX - (*str - '0')) / 10) {
                return (sign == 1) ? INT32_MAX : INT32_MIN; // 返回极值
            }
            result = result * 10 + (*str - '0');
        } else {
            break; // 遇到非数字字符终
        }
        str++;
    }
    
    return sign * result;
}

//上传湿度阈值与光照阈值

void pubcmd_illThreshold(char* number)
{
	ill_ll=string_to_int(number);
	sprintf(cmdBuf,"AT+MQTTPUB=0,\"%s\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%d}}}\",0,0\r\n",pubtopic,func[4],ill_ll);
	Usart_SendString(huart3, (unsigned char *)cmdBuf, strlen((const char *)cmdBuf));
}
void pubcmd_tempThreshold(char* number)
{
	temp_hh=string_to_int(number);
	sprintf(cmdBuf,"AT+MQTTPUB=0,\"%s\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%d}}}\",0,0\r\n",pubtopic,func[5],temp_hh);
	Usart_SendString(huart3, (unsigned char *)cmdBuf, strlen((const char *)cmdBuf));
}

void pubcmd_tempThresholdnum(uint16_t number)
{
	temp_hh=number;
	sprintf(cmdBuf,"AT+MQTTPUB=0,\"%s\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%d}}}\",0,0\r\n",pubtopic,func[5],temp_hh);
	Usart_SendString(huart3, (unsigned char *)cmdBuf, strlen((const char *)cmdBuf));
}

void pubcmd_illThresholdnum(uint16_t number)
{
	ill_ll=number;
	sprintf(cmdBuf,"AT+MQTTPUB=0,\"%s\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%d}}}\",0,0\r\n",pubtopic,func[4],ill_ll);
	Usart_SendString(huart3, (unsigned char *)cmdBuf, strlen((const char *)cmdBuf));
}

//获取平台下发命令

void ESP8266_GetCMD(unsigned char *CMD)
{
	char *ptrIPD = NULL;
	char cmdBuffer[11] = {0};
	uint8_t i;
	if(strstr((char*)CMD,"CMD")!=NULL)
	{
		ptrIPD=strstr((char*)CMD,"CMD");
		ptrIPD = strchr(ptrIPD, ':');							//找到':'
		UsartPrintf(USART_DEBUG,"ipd:%s",ptrIPD);
		if(ptrIPD != NULL)
		{
			ptrIPD++;
			for(i=0;i<10;i++)
			{
				if(ptrIPD[i]<'a'||ptrIPD[i]>'z')
				{
					cmdBuffer[i]='\0';
//					UsartPrintf(USART_DEBUG,"cmd:%s",cmdBuffer);
					break;
				}
				cmdBuffer[i]=ptrIPD[i];
			}
			UsartPrintf(USART_DEBUG,"cmd:%s",cmdBuffer);
			if(strcmp((const char*)cmdBuffer,"true")==0)
			{
				pubcmd(1);
			}
			else if(strcmp((const char*)cmdBuffer,"false")==0)
			{
				pubcmd(0);
			}
		}
		
	}
	if(strstr((char*)CMD,"tempThreshold")!=NULL)
	{
		ptrIPD=strstr((char*)CMD,"tempThreshold");
		ptrIPD = strchr(ptrIPD, ':');							//找到':'
		UsartPrintf(USART_DEBUG,"ipd:%s",ptrIPD);
		if(ptrIPD != NULL)
		{
			ptrIPD++;
			for(i=0;i<10;i++)
			{
				if(ptrIPD[i]<'0'||ptrIPD[i]>'9')
				{
					cmdBuffer[i]='\0';
//					UsartPrintf(USART_DEBUG,"cmd:%s",cmdBuffer);
					break;
				}
				cmdBuffer[i]=ptrIPD[i];
			}
			UsartPrintf(USART_DEBUG,"cmd:%s",cmdBuffer);
			pubcmd_tempThreshold(cmdBuffer);
		}
	}
	if(strstr((char*)CMD,"lightThreshold")!=NULL)
	{
		ptrIPD=strstr((char*)CMD,"lightThreshold");
		ptrIPD = strchr(ptrIPD, ':');							//找到':'
		UsartPrintf(USART_DEBUG,"ipd:%s",ptrIPD);
		if(ptrIPD != NULL)
		{
			ptrIPD++;
			for(i=0;i<10;i++)
			{
				if(ptrIPD[i]<'0'||ptrIPD[i]>'9')
				{
					cmdBuffer[i]='\0';
//					UsartPrintf(USART_DEBUG,"cmd:%s",cmdBuffer);
					break;
				}
				cmdBuffer[i]=ptrIPD[i];
			}
			UsartPrintf(USART_DEBUG,"cmd:%s",cmdBuffer);
			pubcmd_illThreshold(cmdBuffer);
		}
	}
}

//读电压值

void readVoltage(void) {
	uint8_t scan_cnt;

    for(scan_cnt=0;scan_cnt<2;scan_cnt++)
	{
		HAL_ADC_Start(&hadc1);
		if(HAL_ADC_PollForConversion(&hadc1,10)==HAL_OK)
			ADC_Value[scan_cnt]=HAL_ADC_GetValue(&hadc1);
	}
	V_Value[0]=ADC_Value[0]/818.5;
	V_Value[1]=ADC_Value[1]/818.5;

}


// 自定义线性映射函数

uint16_t linearMap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
//读取照度

uint16_t calculateLux(float voltage, const LDRCalib* calib) {
    if (voltage <= calib->dark_threshold) {
        return linearMap(voltage, 0.0f, calib->dark_threshold, 0.0f, calib->dark_max_lux);
    } else if (voltage <= calib->medium_threshold) {
        return linearMap(voltage, calib->dark_threshold, calib->medium_threshold,
                        calib->dark_max_lux, calib->medium_max_lux);
    } else {
        return calib->medium_max_lux + 
               (voltage - calib->medium_threshold) * calib->bright_slope;
    }
}

//设置灯光模式

void led_mode(uint8_t num)
{
	int i,j;
	for(i=0;i<=num;i++)
	{
		led_set(i,1);
	}
	for(j=7-num;j>num;j--)
	{
		led_set(j+num,0);
	}
}

//处理传感器数据

void sig_proc(void)
{
	uint16_t ill_delt;
	readVoltage();
	ill=calculateLux(3.3-V_Value[1],&CALIB_FACTORS);
	if(V_Value[0]>0.6)
	{
		sprintf(aqi,"\"Moderate\"");
	}
	else
	{
		sprintf(aqi,"\"Good\"");
	}
	if(ill<ill_ll)
	{
		ill_delt=ill_ll-ill;
		ill_delt+=99;
		ill_delt/=100;
		ill_delt=ill_delt>7?7:ill_delt;
		led_mode(ill_delt);
	}
	else
	{
		led_mode(0);
	}
	if(temp>temp_hh)
	{
		motor_duty=20;
		motor_ctrl(motor_duty);
	}
	else
	{
		motor_duty=0;
		motor_ctrl(motor_duty);
	}
}


//显示数据

void lcd_proc(void)
{
	switch(interface)
	{
		case Data:
			sprintf(buf,"        DATA        ");
			LCD_DisplayStringLine(Line1,(uint8_t*)buf);
			sprintf(buf,"     TEMP:%d.%d C     ",temp,temp_dec);
			LCD_DisplayStringLine(Line3,(uint8_t*)buf);
			sprintf(buf,"     HUMI:%d.%d %%      ",humi,humi_dec);
			LCD_DisplayStringLine(Line4,(uint8_t*)buf);
			sprintf(buf,"     ILL:%d lux       ",ill);
			LCD_DisplayStringLine(Line5,(uint8_t*)buf);
			sprintf(buf,"     AQI:%s          ",aqi);
			LCD_DisplayStringLine(Line6,(uint8_t*)buf);
			sprintf(buf,"     motor:%d          ",motor_duty);
			LCD_DisplayStringLine(Line7,(uint8_t*)buf);
//			sprintf(buf,"     V:%.2f         ",V_Value[0]);
//			LCD_DisplayStringLine(Line7,(uint8_t*)buf);
//			sprintf(buf,"     adcv:%d        ",ADC_Value[1]);
//			LCD_DisplayStringLine(Line8,(uint8_t*)buf);
		break;
		case Para:
			sprintf(buf,"        PARA        ");
			LCD_DisplayStringLine(Line1,(uint8_t*)buf);
			if(!exch)
			{
				LCD_SetBackColor(Red);
				LCD_SetTextColor(Green);
				sprintf(buf,"    TEMP_HH:%d C      ",temp_hh);
				LCD_DisplayStringLine(Line3,(uint8_t*)buf);
				LCD_SetBackColor(Green);
				LCD_SetTextColor(Red);
				sprintf(buf,"    ILL_LL:%d lux     ",ill_ll);
				LCD_DisplayStringLine(Line4,(uint8_t*)buf);
			}
			else
			{
				
				
				sprintf(buf,"    TEMP_HH:%d C      ",temp_hh);
				LCD_DisplayStringLine(Line3,(uint8_t*)buf);
				LCD_SetBackColor(Red);
				LCD_SetTextColor(Green);
				sprintf(buf,"    ILL_LL:%d lux     ",ill_ll);
				LCD_DisplayStringLine(Line4,(uint8_t*)buf);
				LCD_SetBackColor(Green);
				LCD_SetTextColor(Red);
			}
			sprintf(buf,"                    ");
			LCD_DisplayStringLine(Line5,(uint8_t*)buf);
			sprintf(buf,"                    ");
			LCD_DisplayStringLine(Line6,(uint8_t*)buf);
			sprintf(buf,"                    ");
			LCD_DisplayStringLine(Line7,(uint8_t*)buf);
		break;
	}
}

//初始化

void all_init(void)
{
	key_init(&key[0],GPIOB,GPIO_PIN_0);
	key_init(&key[1],GPIOB,GPIO_PIN_1);
	key_init(&key[2],GPIOB,GPIO_PIN_2);
	key_init(&key[3],GPIOA,GPIO_PIN_0);
	__HAL_UART_ENABLE_IT(&huart2,UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(&huart2,UART_IT_IDLE);
	GPIOC->ODR=0xFF<<8;
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_2);
	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_2);
	HAL_TIM_PWM_Start(&htim16,TIM_CHANNEL_1);
	
}

//按键处理函数

void key_proc(void)
{
	KeyEventType event[4];
	event[0]=KeyGetEvent(&key[0]);
	event[1]=KeyGetEvent(&key[1]);
	event[2]=KeyGetEvent(&key[2]);
	event[3]=KeyGetEvent(&key[3]);
	
	if(event[0]&event_single_click)
	{
		interface=!interface;
		KeyClearEvent(&key[0],event_single_click);
	}
	if(event[1]&event_single_click)
	{
		if(interface==Para)
		{
			if(!exch)
			{
				temp_hh++;
				temp_hh=temp_hh>100?0:temp_hh;
				pubcmd_tempThresholdnum(temp_hh);
			}
			else
			{
				ill_ll++;
				ill_ll=ill_ll>1000?0:ill_ll;
				pubcmd_illThresholdnum(ill_ll);
			}
		}
		else
		{
			motor_duty+=10;
			motor_ctrl(motor_duty);
		}
		KeyClearEvent(&key[1],event_single_click);
	}
	if(event[1]&event_long_press)
	{
		if(interface==Para)
		{
			if(!exch)
			{
				temp_hh++;
				temp_hh=temp_hh>100?0:temp_hh;
			}
			else
			{
				ill_ll+=10;
				ill_ll=ill_ll>1000?0:ill_ll;
			}
		}
		
		if(event[1]&event_release)
		{
			if(!exch)
			{
				pubcmd_tempThresholdnum(temp_hh);
			}
			else
			{
				pubcmd_illThresholdnum(ill_ll);
			}
			
			KeyClearEvent(&key[1],event_long_press);
			KeyClearEvent(&key[1],event_release);
		}
	}
	if(event[2]&event_single_click)
	{
		if(interface==Para)
		{
			if(!exch)
			{
				temp_hh=temp_hh-1<0?100:temp_hh;
				temp_hh--;
				pubcmd_tempThresholdnum(temp_hh);
			}
			else
			{
				ill_ll=ill_ll-1<0?1000:ill_ll;
				ill_ll-=1;
				pubcmd_illThresholdnum(ill_ll);
			}
		}
		else
		{
			motor_duty-=10;
			motor_ctrl(motor_duty);
		}
		KeyClearEvent(&key[2],event_single_click);
	}
	if(event[2]&event_long_press)
	{
		if(interface==Para)
		{
			if(!exch)
			{
				temp_hh=temp_hh-1<0?100:temp_hh;
				temp_hh--;
				
			}
			else
			{
				ill_ll=ill_ll-10<0?1000:ill_ll;
				ill_ll-=10;
				
			}
		}
		if(event[2]&event_release)
		{
			if(!exch)
			{
				pubcmd_tempThresholdnum(temp_hh);
			}
			else
			{
				pubcmd_illThresholdnum(ill_ll);
			}
			KeyClearEvent(&key[2],event_long_press);
			KeyClearEvent(&key[2],event_release);
		}
	}
	if(event[3]&event_single_click)
	{
		if(interface==Para)
		exch=!exch;
		else
		{
			led_turn(8);
		}
		KeyClearEvent(&key[3],event_single_click);
	}

	
}

//电机控制函数

void motor_ctrl(uint8_t duty)
{
	HAL_GPIO_WritePin(GPIOB ,GPIO_PIN_13,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA ,GPIO_PIN_7,GPIO_PIN_RESET);
	duty=duty>30?30:duty;
	__HAL_TIM_SET_COMPARE(&htim16,TIM_CHANNEL_1,duty);
}

//主循环中调用，循环发送传感器数据

void Esp_PUB(void)
{
    char jsonPayload[1024]; // 单独的JSON缓冲区
	if(pub_tick<2000)
		return;
	pub_tick=0;
    // 1. 构造正确的JSON
//	if(change_flag)
//	{
		snprintf(jsonPayload, sizeof(jsonPayload),
			"{\"id\":\"123\"\\,\"params\":{"
			"\"%s\":{\"value\":%d}\\,"
			"\"%s\":{\"value\":%d.%d}\\,"
			"\"%s\":{\"value\":%d.%d}\\,"
			"\"%s\":{\"value\":%s}}}",
			func[0], ill,
			func[1], humi,humi_dec,
			func[2], temp,temp_dec,
			func[3], aqi
			);
//		change_flag=!change_flag;
//	}
//	else
//	{
//		snprintf(jsonPayload, sizeof(jsonPayload),
//			"{\"id\":\"123\"\\,\"params\":{"
//			"\"%s\":{\"value\":%d}\\,"
//			"\"%s\":{\"value\":%d.%d}\\,"
//			"\"%s\":{\"value\":%d.%d}\\,"
//			"\"%s\":{\"value\":%d.%d}}}",
//			func[4], n,
//			func[5], 10,1,
//			func[], humi,humi_dec,
//			func[], temp,temp_dec
//			);
//		change_flag=!change_flag;
//	}
    // 2. 构造AT指令（正确转义）
    int len = snprintf(cmdBuf, sizeof(cmdBuf),
        "AT+MQTTPUB=0,\"%s\",\"", pubtopic);
    for (char *p = jsonPayload; *p && len < sizeof(cmdBuf) - 5; p++) {
        if (*p == '"') {
            // 只需添加反斜杠和双引号（\"）
            if(len + 2 < sizeof(cmdBuf)) {
                cmdBuf[len++] = '\\';  // 添加反斜杠
                cmdBuf[len++] = '"';   // 添加双引号
            }
        } else {
            cmdBuf[len++] = *p;
        }
    }
    // 添加指令结尾
    snprintf(cmdBuf + len, sizeof(cmdBuf) - len, "\",0,0\r\n");
    
    UsartPrintf(USART_DEBUG, "Sending: %s\r\n", cmdBuf);
    // 3. 带超时的发送逻辑
    int retry = 3;
    bool success = false;
    
    while (retry-- > 0 && !success) {
        success = !ESP01S_SendCmd(cmdBuf, "OK");
        if (!success) {
            UsartPrintf(USART_DEBUG, "Retrying...\r\n");
            HAL_Delay(500);
        }
    }
    
    if (!success) {
        UsartPrintf(USART_DEBUG, "Publish failed after %d retries\r\n", 3 - retry);
    }
}

//esp8266串口接收函数
 
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  
    UNUSED(huart);
 
	if(ESP01S_cnt >= sizeof(ESP01S_buf))  //溢出判断
	{
		ESP01S_cnt = 0;
		memset(ESP01S_buf,0x00,sizeof(ESP01S_buf));
		HAL_UART_Transmit(&huart1, (uint8_t *)"接收缓存溢出", 10,0xFFFF); 	      
	}
	else
	{
		ESP01S_buf[ESP01S_cnt++] = aRxBuffer;   //接收数据转存	
//		  if(aRxBuffer=='1')  HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_SET);
//        if(aRxBuffer=='0')  HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_RESET);
 	}
	
	HAL_UART_Receive_IT(&huart3, (uint8_t *)&aRxBuffer, 1);   //再开启接收中断
}

