/**
 * @brief  main.h 桩 — PC 测试用
 * 提供与真实 main.h 相同的 GPIO 引脚定义和类型
 */
#ifndef MAIN_H
#define MAIN_H

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- GPIO 引脚定义 (与真实项目一致) ---- */
#define Photosensitive_Pin      GPIO_PIN_4
#define Photosensitive_GPIO_Port ((GPIO_TypeDef*)0x01)
#define MQ135_Pin               GPIO_PIN_5
#define MQ135_GPIO_Port         ((GPIO_TypeDef*)0x02)
#define MQ2_Pin                 GPIO_PIN_6
#define MQ2_GPIO_Port           ((GPIO_TypeDef*)0x03)
#define BEEP_Pin                GPIO_PIN_5
#define BEEP_GPIO_Port          ((GPIO_TypeDef*)0x04)

/* ---- 额外宏 ---- */
#ifndef GPIO_PIN_4
#define GPIO_PIN_4   ((uint16_t)0x0010)
#endif
#ifndef GPIO_PIN_5
#define GPIO_PIN_5   ((uint16_t)0x0020)
#endif
#ifndef GPIO_PIN_6
#define GPIO_PIN_6   ((uint16_t)0x0040)
#endif

/* ---- 公共函数声明 ---- */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
