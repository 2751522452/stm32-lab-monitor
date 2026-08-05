/* Includes ------------------------------------------------------------------*/                             
#include "sensor/sensor.h"                                                                                   
#include "i2c_dht20/i2c_dht20.h"
#include "adc/adc.h" 

/* Private variables ---------------------------------------------------------*/
static SensorData sensor_data;
static volatile uint16_t adc_buf[3];
static volatile uint8_t  adc_ready = 0;

extern ADC_HandleTypeDef hadc1;

void Sensor_Init(void)
{
		DHT20_Init();
}

void Sensor_Start(void)
{
		HAL_ADCEx_Calibration_Start(&hadc1);
		HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 3);
}

SensorData* Sensor_Read(void)
{
		sensor_data.adc_fresh = 0;

		if (!adc_ready)
				return &sensor_data;

		adc_ready = 0;
		sensor_data.adc_fresh = 1;

		/* ADC raw */
		sensor_data.ps_raw    = adc_buf[0];
		sensor_data.mq135_raw = adc_buf[1];
		sensor_data.mq2_raw   = adc_buf[2];

		/* 电压转换 */
		sensor_data.ps_v    = (float)adc_buf[0] / 4095.0f * 3.3f;
		sensor_data.mq135_v = (float)adc_buf[1] / 4095.0f * 3.3f;
		sensor_data.mq2_v   = (float)adc_buf[2] / 4095.0f * 3.3f;

		/* DHT20 */
		DHT20_Data dht;
		if (DHT20_Read(&dht) == 0)
		{
				sensor_data.temperature = dht.temperature;
				sensor_data.humidity    = dht.humidity;
				sensor_data.dht_valid   = 1;
		}
		else
		{
				sensor_data.dht_valid = 0;
		}

		return &sensor_data;
}

/* DMA 回调 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
		if (hadc->Instance == ADC1)
		{
				adc_ready = 1;
		}
}
