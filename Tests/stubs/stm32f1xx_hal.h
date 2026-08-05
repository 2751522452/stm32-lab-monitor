/**
 * @brief  STM32 HAL 桩头文件 — PC 测试用
 * 替代真实 HAL 库，重定向到模拟层
 */
#ifndef STM32F1XX_HAL_H
#define STM32F1XX_HAL_H

#include "../mocks/mock_hal.h"

/* 额外需要的类型/宏桩 */
#define __weak      __attribute__((weak))
#define __packed    __attribute__((packed))

/* 空操作 */
#define __disable_irq()  do {} while(0)
#define __enable_irq()   do {} while(0)

/* 标准整数类型 */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

/* 常见宏 */
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif /* STM32F1XX_HAL_H */
