/**
 * @brief  STM32 HAL 模拟层实现
 */

#include "mock_hal.h"
#include <string.h>

/* ===================================================================
 *  模拟内部状态
 * =================================================================== */

static uint32_t mock_tick_val = 0;

/* GPIO 记录: pin → state (简化: 只记录最后状态) */
static struct {
    uint16_t pin;
    int      state;
} gpio_records[16];
static int gpio_count = 0;

/* PWM 记录: (htim, channel) → compare value */
static struct {
    TIM_HandleTypeDef *htim;
    uint32_t           channel;
    uint32_t           value;
} pwm_records[8];
static int pwm_count = 0;

/* ---- HAL 句柄实例 ---- */
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* ===================================================================
 *  模拟控制 API
 * =================================================================== */

void mock_tick_set(uint32_t tick)
{
    mock_tick_val = tick;
}

uint32_t mock_tick_get(void)
{
    return mock_tick_val;
}

void mock_tick_advance(uint32_t ms)
{
    mock_tick_val += ms;
}

void mock_tick_init(uint32_t start_tick)
{
    mock_tick_val = start_tick;
}

void mock_gpio_reset(void)
{
    gpio_count = 0;
    memset(gpio_records, 0, sizeof(gpio_records));
}

int mock_gpio_get(uint16_t pin)
{
    for (int i = 0; i < gpio_count; i++) {
        if (gpio_records[i].pin == pin)
            return gpio_records[i].state;
    }
    return -1;  /* 未记录 */
}

void mock_gpio_set(uint16_t pin, int state)
{
    for (int i = 0; i < gpio_count; i++) {
        if (gpio_records[i].pin == pin) {
            gpio_records[i].state = state;
            return;
        }
    }
    if (gpio_count < 16) {
        gpio_records[gpio_count].pin   = pin;
        gpio_records[gpio_count].state = state;
        gpio_count++;
    }
}

void mock_pwm_reset(void)
{
    pwm_count = 0;
    memset(pwm_records, 0, sizeof(pwm_records));
}

uint32_t mock_pwm_get(TIM_HandleTypeDef *htim, uint32_t channel)
{
    for (int i = 0; i < pwm_count; i++) {
        if (pwm_records[i].htim == htim && pwm_records[i].channel == channel)
            return pwm_records[i].value;
    }
    return 0;
}

void mock_pwm_set(TIM_HandleTypeDef *htim, uint32_t channel, uint32_t value)
{
    for (int i = 0; i < pwm_count; i++) {
        if (pwm_records[i].htim == htim && pwm_records[i].channel == channel) {
            pwm_records[i].value = value;
            return;
        }
    }
    if (pwm_count < 8) {
        pwm_records[pwm_count].htim    = htim;
        pwm_records[pwm_count].channel = channel;
        pwm_records[pwm_count].value   = value;
        pwm_count++;
    }
}

void mock_all_reset(void)
{
    mock_tick_val = 0;
    mock_gpio_reset();
    mock_pwm_reset();
}

/* ===================================================================
 *  模拟 HAL 函数
 * =================================================================== */

uint32_t HAL_GetTick(void)
{
    return mock_tick_val;
}

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    mock_pwm_set(htim, Channel, 0);
    return HAL_OK;
}

void __HAL_TIM_SET_COMPARE(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t Compare)
{
    mock_pwm_set(htim, Channel, Compare);
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, int PinState)
{
    mock_gpio_set(GPIO_Pin, PinState);
}
