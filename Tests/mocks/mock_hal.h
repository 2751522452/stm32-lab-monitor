/**
 * @brief  STM32 HAL 模拟层 — PC 端单元测试用
 *
 * 提供可控的 HAL 外设替代品:
 *   - mock_tick:  可控的 HAL_GetTick() 返回值
 *   - mock_gpio:  可查询的 GPIO 写入记录
 *   - mock_pwm:   可查询的 PWM 比较值
 */

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 *  类型定义 (匹配 STM32 HAL)
 * =================================================================== */

typedef uint32_t (*Mock_TickFn)(void);

/* GPIO */
#define GPIO_PIN_RESET  0
#define GPIO_PIN_SET    1

typedef struct {
    int dummy;
} GPIO_TypeDef;

typedef struct {
    uint16_t Pin;
    GPIO_TypeDef *Port;
} GPIO_PinConfig;

/* TIM */
typedef struct {
    int      dummy;
    uint32_t Channel;
    uint32_t CCR[4];   /* 模拟捕获/比较寄存器 */
} TIM_TypeDef;

typedef struct {
    TIM_TypeDef *Instance;
} TIM_HandleTypeDef;

/* Flash 状态 */
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;

/* ---- HAL 句柄 (测试可写) ---- */
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

/* ===================================================================
 *  模拟控制 API
 * =================================================================== */

/* 时钟模拟 */
void    mock_tick_set(uint32_t tick);
uint32_t mock_tick_get(void);
void    mock_tick_advance(uint32_t ms);
void    mock_tick_init(uint32_t start_tick);

/* GPIO 模拟 — 记录最后一次写入 */
void    mock_gpio_reset(void);
int     mock_gpio_get(uint16_t pin);
void    mock_gpio_set(uint16_t pin, int state);

/* PWM 模拟 — 记录 TIM 通道比较值 */
void    mock_pwm_reset(void);
uint32_t mock_pwm_get(TIM_HandleTypeDef *htim, uint32_t channel);
void    mock_pwm_set(TIM_HandleTypeDef *htim, uint32_t channel, uint32_t value);

/* 重置所有模拟状态 */
void mock_all_reset(void);

/* ===================================================================
 *  模拟 HAL 函数 (替代真实硬件调用)
 * =================================================================== */
uint32_t        HAL_GetTick(void);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
void            __HAL_TIM_SET_COMPARE(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t Compare);
void            HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, int PinState);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_HAL_H */
