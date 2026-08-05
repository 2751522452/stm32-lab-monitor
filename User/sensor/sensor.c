/**
 * @brief  传感器模块 — 实现
 *
 * Sensor_Read() 被 SensorTask @20ms 调用, 内部完成:
 *   - ADC DMA 数据解包 + 电压转换
 *   - DHT20 温湿度采集
 *   - MQ135/MQ2 预热状态机更新 (PREHEAT→STABLE→FAULT)
 */

#include "sensor/sensor.h"
#include "i2c_dht20/i2c_dht20.h"
#include "adc/adc.h"

/* ---- 模块内部状态 ------------------------------------------------------------*/
static SensorData   sensor_data;
static volatile uint16_t adc_buf[3];
static volatile uint8_t  adc_ready;

static WarmupSensor mq135;
static WarmupSensor mq2;

extern ADC_HandleTypeDef hadc1;

/* ---- 气体传感器预热状态机 ----------------------------------------------------*/

void Sensor_WarmupInit(WarmupSensor *ws, uint32_t preheat_ms)
{
    ws->state      = WARMUP_PREHEAT;
    ws->start_tick = HAL_GetTick();
    ws->ready      = 0;
    ws->preheat_ms = preheat_ms;
    ws->fault_tick = 0;
}

void Sensor_WarmupUpdate(WarmupSensor *ws, uint16_t raw_adc)
{
    uint32_t now     = HAL_GetTick();
    uint32_t elapsed = now - ws->start_tick;

    switch (ws->state) {
    case WARMUP_PREHEAT:
        if (elapsed >= ws->preheat_ms) {
            ws->state = WARMUP_STABLE;
            ws->ready = 1;
        }
        break;

    case WARMUP_STABLE:
        if (raw_adc < WARMUP_ADC_MIN) {
            if (ws->fault_tick == 0) {
                ws->fault_tick = now;
            } else if (now - ws->fault_tick >= WARMUP_TIMEOUT_MS) {
                ws->state = WARMUP_FAULT;
                ws->ready = 0;
            }
        } else {
            ws->fault_tick = 0;
        }
        break;

    case WARMUP_FAULT:
        if (raw_adc >= WARMUP_ADC_MIN) {
            ws->state      = WARMUP_STABLE;
            ws->ready      = 1;
            ws->fault_tick = 0;
        }
        break;
    }
}

uint8_t Sensor_IsReady(int ch)
{
    return (ch == 0) ? mq135.ready : mq2.ready;
}

/* ---- 传感器主逻辑 ------------------------------------------------------------*/

void Sensor_Init(void)
{
    DHT20_Init();
    Sensor_WarmupInit(&mq135, 180000);  /* MQ135 预热 180s */
    Sensor_WarmupInit(&mq2,   120000);  /* MQ2  预热 120s */
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

    /* 气体传感器预热状态机 — 采集完即刻更新 */
    Sensor_WarmupUpdate(&mq135, sensor_data.mq135_raw);
    Sensor_WarmupUpdate(&mq2,   sensor_data.mq2_raw);

    /* DHT20 */
    DHT20_Data dht;
    if (DHT20_Read(&dht) == 0) {
        sensor_data.temperature = dht.temperature;
        sensor_data.humidity    = dht.humidity;
        sensor_data.dht_valid   = 1;
    } else {
        sensor_data.dht_valid = 0;
    }

    return &sensor_data;
}

/* ---- DMA 回调 ---------------------------------------------------------------*/

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        adc_ready = 1;
    }
}
