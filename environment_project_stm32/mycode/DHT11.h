#ifndef __DHT11_H__
#define __DHT11_H__
 

/* Private define ------------------------------------------------------------*/
#define DHT11_Port GPIOB
#define DHT11_Pin GPIO_PIN_15

#define DHT11_PIN_SET   HAL_GPIO_WritePin(DHT11_Port,DHT11_Pin,GPIO_PIN_SET)                                            //  设置GPIO为高
#define DHT11_PIN_RESET HAL_GPIO_WritePin(DHT11_Port,DHT11_Pin,GPIO_PIN_RESET)                                          //  设置GPIO为低
#define DHT11_READ_IO   HAL_GPIO_ReadPin(DHT11_Port,DHT11_Pin)                                                          //  DHT11 GPIO定义
 
extern uint8_t data[5];
 
/* Private function prototypes -----------------------------------------------*/
void DHT11(void);
void DHT11_START(void);
unsigned char DHT11_READ_BIT(void);
unsigned char DHT11_READ_BYTE(void);
unsigned char DHT11_READ_DATA(void);
unsigned char DHT11_Check(void);
static void DHT11_GPIO_MODE_SET(uint8_t mode);
 
void Coarse_delay_us(uint32_t us);
    
#endif
 
