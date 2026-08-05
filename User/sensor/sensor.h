#ifndef __SENSOR_H__
#define __SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* 传感器统一数据结构 --------------------------------------------------------*/
typedef struct
{
		/* ADC 原始值 */
		uint16_t ps_raw;
		uint16_t mq135_raw;
		uint16_t mq2_raw;

		/* 电压值 (V) */
		float ps_v;
		float mq135_v;
		float mq2_v;

		/* DHT20 温湿度 */
		float    temperature;
		float    humidity;
		uint8_t  dht_valid;     /* 1=读取成功  0=失败 */

		/* 状态 */
		uint8_t  adc_fresh;     /* 1=本轮有新数据  0=无新数据 */
} SensorData;

/* 函数声明 ------------------------------------------------------------------*/
void Sensor_Init(void);
void Sensor_Start(void);
SensorData* Sensor_Read(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_H__ */
