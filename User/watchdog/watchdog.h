#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* ---- 心跳位掩码 (每个任务一个 bit) ---- */
#define WD_HEART_SENSOR   (1 << 0)
#define WD_HEART_ALARM    (1 << 1)
#define WD_HEART_MQ       (1 << 2)
#define WD_HEART_DISPLAY  (1 << 3)
#define WD_HEART_FLASH    (1 << 4)
#define WD_HEART_WIFI     (1 << 5)

#define WD_HEART_ALL (WD_HEART_SENSOR | WD_HEART_ALARM  | WD_HEART_MQ  | \
                      WD_HEART_DISPLAY | WD_HEART_FLASH | WD_HEART_WIFI)

/* ---- API ---- */
void Watchdog_Init(void);                     /* IWDG 初始化 (LSI/64, RLR=1250 → ~2s) */
void Watchdog_Heartbeat(uint8_t mask);        /* 任务周期性心跳上报 */
void Watchdog_Task(void *pvParameters);       /* WatchdogTask 主循环 (Prio 2, 500ms) */

#ifdef __cplusplus
}
#endif

#endif /* __WATCHDOG_H__ */
