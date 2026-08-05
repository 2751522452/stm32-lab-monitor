#ifndef __ALARM_H__
#define __ALARM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* 告警等级 -------------------------------------------------------------------*/
typedef enum
{
		ALERT_LEVEL_0 = 0,   /* 安全 — LED 常亮低亮 */
		ALERT_LEVEL_1 = 1,   /* 注意 — LED 稍亮 */
		ALERT_LEVEL_2 = 2,   /* 警告 — LED 亮 + 舵机 0° */
		ALERT_LEVEL_3 = 3,   /* 危险 — LED 闪烁 + 舵机 45° + 蜂鸣器 */
		ALERT_LEVEL_4 = 4    /* 紧急 — LED 快闪 + 舵机 90° + 蜂鸣器 */
} AlertLevel;

/* 函数声明 -------------------------------------------------------------------*/
void Alarm_Init(void);
void Alarm_SetLevel(AlertLevel level);
void Alarm_Process(void);       /* 每轮主循环调用，驱动 LED/舵机 */
AlertLevel Alarm_CalcLevel(float voltage);

#ifdef __cplusplus
}
#endif

#endif /* __ALARM_H__ */
