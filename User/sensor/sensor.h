/**
 * @brief  传感器模块 — 数据采集 + 气体传感器预热状态机
 *
 * Sensor_Read() 内部完成:
 *   1. ADC DMA 数据读取 (MQ135/MQ2/PS)
 *   2. DHT20 温湿度采集
 *   3. 气体传感器预热状态机更新 (PREHEAT→STABLE→FAULT)
 *
 * 对调用者透明 — 调用方只需读 SensorData, 通过 Sensor_IsReady() 查询状态。
 */

#ifndef __SENSOR_H__
#define __SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ---- 气体传感器预热状态 (内部用) ---------------------------------------------*/
typedef enum {
    WARMUP_PREHEAT = 0,
    WARMUP_STABLE  = 1,
    WARMUP_FAULT   = 2
} WarmupState;

typedef struct {
    WarmupState state;
    uint32_t    start_tick;
    uint32_t    fault_tick;
    uint32_t    preheat_ms;
    uint8_t     ready;          /* 1=读数可用 */
} WarmupSensor;

#define WARMUP_ADC_MIN     100
#define WARMUP_TIMEOUT_MS  5000

/* ---- 传感器统一数据结构 ------------------------------------------------------*/
typedef struct {
    /* ADC 原始值 */
    uint16_t ps_raw;
    uint16_t mq135_raw;
    uint16_t mq2_raw;

    /* 电压值 (V) */
    float ps_v;
    float mq135_v;
    float mq2_v;

    /* DHT20 温湿度 */
    float   temperature;
    float   humidity;
    uint8_t dht_valid;

    /* 状态 */
    uint8_t adc_fresh;     /* 1=本轮有新数据 */
} SensorData;

/* ---- API ------------------------------------------------------------------*/
void         Sensor_Init(void);
void         Sensor_Start(void);
SensorData*  Sensor_Read(void);

/* 气体传感器预热状态 (单元测试需要, 声明为公开) */
void    Sensor_WarmupInit(WarmupSensor *ws, uint32_t preheat_ms);
void    Sensor_WarmupUpdate(WarmupSensor *ws, uint16_t raw_adc);
uint8_t Sensor_IsReady(int ch);   /* 0=MQ135, 1=MQ2 */

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_H__ */
