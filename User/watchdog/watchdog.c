/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "watchdog.h"

/* Private variables ---------------------------------------------------------*/
static IWDG_HandleTypeDef  hiwdg;
static volatile uint8_t    g_heartbeat_flags = 0;

/* ---- Watchdog_Init: IWDG 硬件初始化 ---- */
void Watchdog_Init(void)
{
    /* 调试模式暂停时冻结 IWDG (防止断点触发复位) */
    DBGMCU->CR |= DBGMCU_CR_DBG_IWDG_STOP;

    /* IWDG 参数:
     *   LSI = 40kHz (典型值)
     *   分频 = /64 → 625Hz
     *   重载 = 1250
     *   超时 = 1250 / 625 = 2s
     *   最差情况 (LSI=50kHz): 超时 = 1250 × 64 / 50000 = 1.6s
     */
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload    = 1250;

    HAL_IWDG_Init(&hiwdg);
    /* 注意: IWDG 一经启动无法软件停止，仅硬件复位可关闭 */
}

/* ---- Watchdog_Heartbeat: 任务心跳上报 (临界区保护) ---- */
void Watchdog_Heartbeat(uint8_t mask)
{
    taskENTER_CRITICAL();
    g_heartbeat_flags |= mask;
    taskEXIT_CRITICAL();
}

/* ---- Watchdog_Task: 独立喂狗任务 (Prio 2, 500ms 周期) ---- */
void Watchdog_Task(void *pvParameters)
{
    (void)pvParameters;

    /* 启动宽容期: 前 10 秒无条件喂狗, 等所有任务完成首次心跳上报 */
    uint32_t boot_ticks = 0;
    #define BOOT_GRACE_MS  10000   /* 10s 启动宽容期 */

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
        boot_ticks += 500;

        uint8_t flags;
        taskENTER_CRITICAL();
        flags = g_heartbeat_flags;
        g_heartbeat_flags = 0;  /* 清零，下个周期重新收集 */
        taskEXIT_CRITICAL();

        if (boot_ticks < BOOT_GRACE_MS) {
            /* 启动宽容期: 无条件喂狗，避免因任务启动时序差异导致误复位 */
            HAL_IWDG_Refresh(&hiwdg);
        } else if (flags == WD_HEART_ALL) {
            HAL_IWDG_Refresh(&hiwdg);   /* 所有任务存活 → 喂狗 */
        }
        /* 否则: 不喂狗 → IWDG 超时 → 系统复位
         * 宁可误复位，不可漏掉真正卡死的任务 */
    }
}
