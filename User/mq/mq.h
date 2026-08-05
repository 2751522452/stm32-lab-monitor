#ifndef __MQ_H__
#define __MQ_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* MQ 传感器状态枚举 ----------------------------------------------------------*/
typedef enum
{
		MQ_STATE_PREHEAT = 0,   /* 预热中 */
		MQ_STATE_STABLE  = 1,   /* 稳定工作 */
		MQ_STATE_FAULT   = 2    /* 故障 */
} MQ_State;

/* MQ 传感器句柄 --------------------------------------------------------------*/
typedef struct
{
		MQ_State state;           /* 当前状态 */
		uint32_t start_tick;      /* 预热起始 tick */
		uint32_t fault_tick;      /* 故障计时 tick */
		uint32_t preheat_ms;      /* 预热时长 (ms) */
		uint8_t  stable;          /* 1=稳定可读数  0=不可用 */
} MQ_Sensor;

/* 故障判定参数 ----------------------------------------------------------------*/
#define MQ_FAULT_ADC_MIN     100    /* ADC 低于此值视为异常 */
#define MQ_FAULT_TIMEOUT_MS  5000   /* 连续异常 5s 判定故障 */

/* 函数声明 -------------------------------------------------------------------*/
void MQ_Init(MQ_Sensor *s, uint32_t preheat_ms);
void MQ_Update(MQ_Sensor *s, uint16_t raw_adc);

#ifdef __cplusplus
}
#endif

#endif /* __MQ_H__ */
