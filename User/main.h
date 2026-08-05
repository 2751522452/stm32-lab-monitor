/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
	
/* Private defines -----------------------------------------------------------*/
#define Photosensitive_Pin GPIO_PIN_4
#define Photosensitive_GPIO_Port GPIOA
#define MQ135_Pin GPIO_PIN_5
#define MQ135_GPIO_Port GPIOA
#define MQ2_Pin GPIO_PIN_6
#define MQ2_GPIO_Port GPIOA
#define ESP8266_TX_Pin GPIO_PIN_10
#define ESP8266_TX_GPIO_Port GPIOB
#define ESP8266_RX_Pin GPIO_PIN_11
#define ESP8266_RX_GPIO_Port GPIOB
#define FLASH_SCK_Pin GPIO_PIN_13
#define FLASH_SCK_GPIO_Port GPIOB
#define FLASH_MISO_Pin GPIO_PIN_14
#define FLASH_MISO_GPIO_Port GPIOB
#define FLASH_MOSI_Pin GPIO_PIN_15
#define FLASH_MOSI_GPIO_Port GPIOB
#define FLASH_CS_Pin GPIO_PIN_8
#define FLASH_CS_GPIO_Port GPIOA
#define DEBUGE_TX_Pin GPIO_PIN_9
#define DEBUGE_TX_GPIO_Port GPIOA
#define DEBUGE_RX_Pin GPIO_PIN_10
#define DEBUGE_RX_GPIO_Port GPIOA
#define BEEP_Pin GPIO_PIN_5
#define BEEP_GPIO_Port GPIOB
#define ESP8266_IO_Pin GPIO_PIN_8
#define ESP8266_IO_GPIO_Port GPIOB
#define ESP8266_RST_Pin GPIO_PIN_9
#define ESP8266_RST_GPIO_Port GPIOB

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
