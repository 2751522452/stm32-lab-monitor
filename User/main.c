/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOSConfig.h"
#include "main.h"
#include "adc/adc.h"
#include "tim/tim.h"
#include "usart/usart.h"
#include "gpio/gpio.h"
#include "i2c/i2c.h"
#include "i2c_led/i2c_led.h"
#include "i2c_scan/i2c_scan.h"
#include "dma/dma.h"
#include "spi/spi.h"
#include "spi_w25q64/spi_w25q64.h"
#include "sensor/sensor.h"
#include "mq/mq.h"
#include "alarm/alarm.h"
#include "storage/adc_storage.h"
#include "wifi/esp8266.h"
#include "wifi/mqtt.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
MQ_Sensor mq135;
MQ_Sensor mq2;
uint32_t  flash_id;

/* FreeRTOS 任务间共享数据 --------------------------------------------------*/
static SensorData         g_sensorData;
static SemaphoreHandle_t  g_sensorMutex;
static QueueHandle_t      g_wifiQueue;
static TaskHandle_t       hSensor, hAlarm, hMQ, hDisplay, hFlash, hWiFi;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SensorTask(void *pvParameters);
static void AlarmTask(void *pvParameters);
static void DisplayTask(void *pvParameters);
static void MQTask(void *pvParameters);
static void FlashTask(void *pvParameters);
static void WiFiTask(void *pvParameters);

/* ---- SensorTask (Prio 3, 20ms) ---- */
static void SensorTask(void *pvParameters)
{
	(void)pvParameters;
	for (;;) {
		SensorData *sd = Sensor_Read();

		if (sd->adc_fresh) {
			xSemaphoreTake(g_sensorMutex, portMAX_DELAY);
			g_sensorData = *sd;
			xSemaphoreGive(g_sensorMutex);
			/* 异步推送到 WiFi 队列 (覆盖旧数据) */
			xQueueOverwrite(g_wifiQueue, sd);
		}
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

/* ---- AlarmTask (Prio 2, 20ms) ---- */
static void AlarmTask(void *pvParameters)
{
	(void)pvParameters;
	for (;;) {
		SensorData sd;
		xSemaphoreTake(g_sensorMutex, portMAX_DELAY);
		sd = g_sensorData;
		xSemaphoreGive(g_sensorMutex);

		AlertLevel level = Alarm_CalcLevel(sd.ps_v);
		Alarm_SetLevel(level);
		Alarm_Process();

		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

/* ---- DisplayTask (Prio 1, 200ms) ---- */
static void DisplayTask(void *pvParameters)
{
	(void)pvParameters;
	char ps_buf[20], mq135_buf[20], mq2_buf[20], dht_buf[20], cnt_buf[20];

	for (;;) {
		SensorData sd;
		xSemaphoreTake(g_sensorMutex, portMAX_DELAY);
		sd = g_sensorData;
		xSemaphoreGive(g_sensorMutex);

		sprintf(ps_buf,    "PS_V:%.2fV   ", sd.ps_v);
		sprintf(mq135_buf, "MQ135_V:%.2fV   ", sd.mq135_v);
		sprintf(mq2_buf,   "MQ2_V:%.2fV   ", sd.mq2_v);
		OLED_ShowString(0, 0, ps_buf);
		OLED_ShowString(0, 1, mq135_buf);
		OLED_ShowString(0, 2, mq2_buf);

		if (sd.dht_valid)
			sprintf(dht_buf, "T:%.1fC H:%.1f%%", sd.temperature, sd.humidity);
		else
			sprintf(dht_buf, "DHT20: ERR");
		OLED_ShowString(0, 3, dht_buf);

		sprintf(cnt_buf, "Rec:%lu            ", (unsigned long)ADC_Get_Record_Count());
		OLED_ShowString(0, 4, cnt_buf);

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

/* ---- MQTask (Prio 2, 1s) ---- */
static void MQTask(void *pvParameters)
{
	(void)pvParameters;
	for (;;) {
		SensorData sd;
		xSemaphoreTake(g_sensorMutex, portMAX_DELAY);
		sd = g_sensorData;
		xSemaphoreGive(g_sensorMutex);

		if (sd.adc_fresh) {
			MQ_Update(&mq135, sd.mq135_raw);
			MQ_Update(&mq2,   sd.mq2_raw);
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

/* ---- FlashTask (Prio 1, 2s) ---- */
static void FlashTask(void *pvParameters)
{
	(void)pvParameters;
	for (;;) {
		SensorData sd;
		xSemaphoreTake(g_sensorMutex, portMAX_DELAY);
		sd = g_sensorData;
		xSemaphoreGive(g_sensorMutex);

		if (sd.adc_fresh) {
			ADC_Record rec;
			rec.timestamp = HAL_GetTick();
			rec.ps        = sd.ps_raw;
			rec.mq135     = sd.mq135_raw;
			rec.mq2       = sd.mq2_raw;
			ADC_Save_Record(&rec);
		}
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

/* ---- WiFiTask (Prio 2, 500ms) ---- */
static void WiFiTask(void *pvParameters)
{
	(void)pvParameters;
	static int      mqtt_state  = 0;  /* 0=CONNECT 1=等CONNACK 2=READY */
	static uint32_t last_ping   = 0;
	static uint32_t conn_tick   = 0;

	ESP8266_Init(&g_esp,
		"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD",
		"test.mosquitto.org", 1883);

	for (;;) {
		ESP8266_Process(&g_esp);

		/* MQTT: CONNECT */
		if (g_esp.state == ESP_STATE_READY && !g_esp.sending && mqtt_state == 0) {
			uint8_t buf[128];
			int len = mqtt_build_connect(buf, "stm32-etms", 60);
			if (ESP8266_SendRaw(&g_esp, buf, (uint16_t)len) == 0) {
				mqtt_state = 1;
				conn_tick  = HAL_GetTick();
			}
		}

		/* MQTT: 等 CONNACK */
		if (mqtt_state == 1) {
			uint16_t avail = ringbuf_avail(&g_esp.rx_buf);
			for (uint16_t i = 0; i + 4 <= avail; i++) {
				uint8_t b0, b1, b2, b3;
				if (ringbuf_peek(&g_esp.rx_buf, i+0, &b0) &&
				    ringbuf_peek(&g_esp.rx_buf, i+1, &b1) &&
				    ringbuf_peek(&g_esp.rx_buf, i+2, &b2) &&
				    ringbuf_peek(&g_esp.rx_buf, i+3, &b3)) {
					if (b0 == 0x20 && b1 == 0x02 && b2 == 0x00 && b3 == 0x00) {
						mqtt_state  = 2;
						last_ping   = HAL_GetTick();
						printf("[MQTT] connected\n");
						printf("[STACK] S=%u A=%u M=%u D=%u F=%u W=%u\n",
							(unsigned)uxTaskGetStackHighWaterMark(hSensor),
							(unsigned)uxTaskGetStackHighWaterMark(hAlarm),
							(unsigned)uxTaskGetStackHighWaterMark(hMQ),
							(unsigned)uxTaskGetStackHighWaterMark(hDisplay),
							(unsigned)uxTaskGetStackHighWaterMark(hFlash),
							(unsigned)uxTaskGetStackHighWaterMark(hWiFi));
						break;
					}
				}
			}
			if (mqtt_state == 1 && HAL_GetTick() - conn_tick > 5000) {
				mqtt_state = 0;
			}
		}

		/* MQTT: PUBLISH + PINGREQ */
		if (g_esp.state == ESP_STATE_READY && !g_esp.sending && mqtt_state == 2) {
			SensorData sd;
			if (xQueueReceive(g_wifiQueue, &sd, 0) == pdTRUE) {
				char json[128];
				snprintf(json, sizeof(json),
					"{\"ts\":%lu,\"ps\":%.2f,\"mq135\":%.2f,"
					"\"mq2\":%.2f,\"t\":%.1f,\"h\":%.1f}",
					(unsigned long)HAL_GetTick(),
					sd.ps_v, sd.mq135_v, sd.mq2_v,
					sd.temperature, sd.humidity);
				uint8_t buf[256];
				int len = mqtt_build_publish(buf, "etms/sensor",
					(uint8_t *)json, (uint16_t)strlen(json));
				ESP8266_SendRaw(&g_esp, buf, (uint16_t)len);
			}

			if (HAL_GetTick() - last_ping > 30000) {
				uint8_t ping[2];
				mqtt_build_pingreq(ping);
				ESP8266_SendRaw(&g_esp, ping, 2);
				last_ping = HAL_GetTick();
			}
		}

		/* TCP 断开 → 重置 MQTT */
		if (g_esp.state != ESP_STATE_READY &&
		    g_esp.state != ESP_STATE_SENDING) {
			mqtt_state = 0;
		}

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* MCU Configuration--------------------------------------------------------*/
	HAL_Init();
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART1_UART_Init();
	MX_ADC1_Init();
	MX_TIM3_Init();
	MX_TIM2_Init();
	MX_I2C1_Init();
	MX_SPI2_Init();
	MX_USART3_UART_Init();

	/* Flash 存储 */
	ADC_Storage_Init();
	printf("Storage Init OK\r\n");
	printf("Write Addr=0x%08lX\r\n", (unsigned long)ADC_Get_Write_Address());
	printf("Record Count=%lu\r\n", (unsigned long)ADC_Get_Record_Count());

	/* Flash ID 检测 */
	flash_id = W25Q64_ReadID();
	printf("W25Q64 ID = 0x%06lX\r\n", (unsigned long)flash_id);

	/* 外设模块初始化 */
	OLED_Init();
	W25Q64_Init();
	I2C_Scan();
	OLED_Clear();

	/* 四模块初始化 */
	Sensor_Init();
	Sensor_Start();
	Alarm_Init();
	MQ_Init(&mq135, 180000);   /* MQ135 预热 180 秒 */
	MQ_Init(&mq2,   120000);   /* MQ2  预热 120 秒 */

	/* 创建互斥锁 + 队列 + 六个任务 */
	g_sensorMutex = xSemaphoreCreateMutex();
	g_wifiQueue = xQueueCreate(1, sizeof(SensorData));
	xTaskCreate(SensorTask,  "Sensor",  200, NULL, 3, &hSensor);
	xTaskCreate(AlarmTask,   "Alarm",   256, NULL, 2, &hAlarm);
	xTaskCreate(MQTask,      "MQ",      256, NULL, 2, &hMQ);
	xTaskCreate(DisplayTask, "Display", 512, NULL, 1, &hDisplay);
	xTaskCreate(FlashTask,   "Flash",   256, NULL, 1, &hFlash);
	xTaskCreate(WiFiTask,    "WiFi",    512, NULL, 2, &hWiFi);

	printf("FreeRTOS starting...\r\n");
	vTaskStartScheduler();
	/* Should never reach here */
	while (1) {}
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/* printf 重定向 */
int fputc(int ch, FILE *f)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	__disable_irq();
	while (1)
	{
	}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	__disable_irq();
	while (1) {}
}

void vApplicationMallocFailedHook(void)
{
	__disable_irq();
	while (1) {}
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
