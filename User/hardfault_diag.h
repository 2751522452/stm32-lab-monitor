#ifndef __HARDFAULT_DIAG_H
#define __HARDFAULT_DIAG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Handler 进入跟踪位（由 stm32f1xx_it.c 设置，HardFault_Dump 输出）
 */
extern volatile uint32_t trace_svc_cnt;      /* SVC_Handler 进入次数      */
extern volatile uint32_t trace_pendsv_cnt;   /* PendSV_Handler 进入次数   */
extern volatile uint32_t trace_systick_cnt;  /* SysTick 进入次数           */
extern volatile uint32_t trace_systick_freertos_cnt; /* SysTick 中调用了 xPortSysTickHandler 的次数 */

/**
 * @brief  HardFault 诊断输出
 * @param  sp         栈指针（由调用方传入 MSP 或 PSP）
 * @param  exc_return EXC_RETURN 值（由调用方传入 LR）
 */
void HardFault_Dump(uint32_t *sp, uint32_t exc_return);

#ifdef __cplusplus
}
#endif

#endif /* __HARDFAULT_DIAG_H */
