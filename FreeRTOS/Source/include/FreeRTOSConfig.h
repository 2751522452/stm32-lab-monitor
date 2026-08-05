#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f1xx_hal.h"

#define configUSE_PREEMPTION                1
#define configUSE_IDLE_HOOK                 0
#define configUSE_TICK_HOOK                 0
#define configCPU_CLOCK_HZ                  ( SystemCoreClock )
#define configTICK_RATE_HZ                  ( (TickType_t)1000 )
#define configMAX_PRIORITIES                ( 5 )
#define configMINIMAL_STACK_SIZE            ( (uint16_t)128 )
#define configTOTAL_HEAP_SIZE               ( (size_t)(10 * 1024) )
#define configMAX_TASK_NAME_LEN             ( 12 )
#define configUSE_TRACE_FACILITY            1
#define configUSE_16_BIT_TICKS              0
#define configIDLE_SHOULD_YIELD             1
#define configUSE_MUTEXES                   1
#define configUSE_COUNTING_SEMAPHORES       1
#define configUSE_TASK_NOTIFICATIONS        1
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configCHECK_FOR_STACK_OVERFLOW      2
#define configUSE_TIMERS                    0
#define INCLUDE_vTaskDelay                  1
#define configUSE_CO_ROUTINES               0

#define configPRIO_BITS                     4
#define configKERNEL_INTERRUPT_PRIORITY     ( 15 << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY ( 5 << (8 - configPRIO_BITS) )

#define configASSERT(x) if((x)==0) { taskDISABLE_INTERRUPTS(); for(;;); }

#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define configUSE_MALLOC_FAILED_HOOK        1

#endif

