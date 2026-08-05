/**
 * @brief  告警模块单元测试 — Alarm_CalcLevel / Alarm_SetLevel / Alarm_Process
 *
 * 被测源文件: ../User/alarm/alarm.c
 * 覆盖: 5 级电压阈值边界 | 告警输出配置 | LED 闪烁/常亮模式
 */

#include "unity.h"
#include "mocks/mock_hal.h"

/* ---- 被测函数声明 (alarm.c 编译链接) ---- */
extern void      Alarm_Init(void);
extern void      Alarm_SetLevel(int level);
extern void      Alarm_Process(void);
extern int       Alarm_CalcLevel(float voltage);

/* ---- 枚举 (与 alarm.h 一致) ---- */
enum {
    ALERT_LEVEL_0 = 0, ALERT_LEVEL_1 = 1, ALERT_LEVEL_2 = 2,
    ALERT_LEVEL_3 = 3, ALERT_LEVEL_4 = 4,
};

/* ---- 硬件参数 ---- */
#define SERVO_0     500
#define SERVO_45    1500
#define SERVO_90    2500
#define LED_STEP    100
#define TIM_CHANNEL_2  2
#define TIM_CHANNEL_3  3

/* ---- 引脚 (与 main.h 一致) ---- */
#define BEEP_Pin          ((uint16_t)0x0020)
#define BEEP_GPIO_Port    ((GPIO_TypeDef*)0x04)

/* ================================================================
 *  setUp / tearDown
 * ================================================================ */
void setUp(void)
{
    mock_all_reset();
    mock_tick_init(10000);
}

void tearDown(void) {}

/* ================================================================
 *  Alarm_CalcLevel — 电压阈值边界测试 (15 条)
 * ================================================================ */

void test_calc_L0_normal(void)
{
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_0, Alarm_CalcLevel(3.3f));
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_0, Alarm_CalcLevel(2.7f));
}

void test_calc_L0_boundary(void)
{
    /* 2.6V 不满足 >2.6 → L1 */
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_1, Alarm_CalcLevel(2.6f));
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_0, Alarm_CalcLevel(2.61f));
}

void test_calc_L1_range(void)
{
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_1, Alarm_CalcLevel(2.5f));
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_1, Alarm_CalcLevel(2.31f));
}

void test_calc_L1_L2_boundary(void)
{
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_2, Alarm_CalcLevel(2.3f));
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_1, Alarm_CalcLevel(2.31f));
}

void test_calc_L2_range(void)
{
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_2, Alarm_CalcLevel(2.2f));
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_2, Alarm_CalcLevel(2.01f));
}

void test_calc_L2_L3_boundary(void)
{
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_3, Alarm_CalcLevel(2.0f));
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_2, Alarm_CalcLevel(2.01f));
}

void test_calc_L3_range(void)
{
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_3, Alarm_CalcLevel(1.0f));
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_3, Alarm_CalcLevel(0.01f));
}

void test_calc_L3_L4_boundary(void)
{
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_4, Alarm_CalcLevel(0.0f));
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_3, Alarm_CalcLevel(0.001f));
}

void test_calc_L4_dead(void)
{
    TEST_ASSERT_EQUAL_INT(ALERT_LEVEL_4, Alarm_CalcLevel(0.0f));
}

/* ================================================================
 *  Alarm_SetLevel — 输出配置 (9 条)
 * ================================================================ */

void test_init_safe_state(void)
{
    Alarm_Init();
    TEST_ASSERT_EQUAL_UINT32(0, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    TEST_ASSERT_EQUAL_UINT32(SERVO_0, mock_pwm_get(&htim3, TIM_CHANNEL_3));
    TEST_ASSERT_EQUAL_INT(GPIO_PIN_RESET, mock_gpio_get(BEEP_Pin));
}

void test_set_L0_all_off(void)
{
    Alarm_Init();
    mock_pwm_set(&htim2, TIM_CHANNEL_2, 999);
    Alarm_SetLevel(ALERT_LEVEL_0);
    TEST_ASSERT_EQUAL_UINT32(0, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    TEST_ASSERT_EQUAL_UINT32(SERVO_0, mock_pwm_get(&htim3, TIM_CHANNEL_3));
}

void test_set_L1_led_dim(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_1);
    TEST_ASSERT_EQUAL_UINT32(LED_STEP, mock_pwm_get(&htim2, TIM_CHANNEL_2));
}

void test_set_L2_led_bright(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_2);
    TEST_ASSERT_EQUAL_UINT32(LED_STEP * 10, mock_pwm_get(&htim2, TIM_CHANNEL_2));
}

void test_set_L3_servo_45(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_3);
    TEST_ASSERT_EQUAL_UINT32(SERVO_45, mock_pwm_get(&htim3, TIM_CHANNEL_3));
}

void test_set_L4_buzzer_on(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_4);
    TEST_ASSERT_EQUAL_UINT32(SERVO_90, mock_pwm_get(&htim3, TIM_CHANNEL_3));
    TEST_ASSERT_EQUAL_INT(GPIO_PIN_SET, mock_gpio_get(BEEP_Pin));
}

void test_set_idempotent(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_4);
    /* 篡改哨兵值 */
    mock_pwm_set(&htim2, TIM_CHANNEL_2, 777);
    mock_pwm_set(&htim3, TIM_CHANNEL_3, 888);
    mock_gpio_set(BEEP_Pin, GPIO_PIN_RESET);
    /* 重复设置同等级 → 提前返回 */
    Alarm_SetLevel(ALERT_LEVEL_4);
    TEST_ASSERT_EQUAL_UINT32(777, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    TEST_ASSERT_EQUAL_UINT32(888, mock_pwm_get(&htim3, TIM_CHANNEL_3));
}

void test_escalation_0_to_3(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_0);
    TEST_ASSERT_EQUAL_UINT32(0, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    Alarm_SetLevel(ALERT_LEVEL_1);
    TEST_ASSERT_EQUAL_UINT32(LED_STEP, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    Alarm_SetLevel(ALERT_LEVEL_2);
    TEST_ASSERT_EQUAL_UINT32(LED_STEP * 10, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    Alarm_SetLevel(ALERT_LEVEL_3);
    TEST_ASSERT_EQUAL_UINT32(SERVO_45, mock_pwm_get(&htim3, TIM_CHANNEL_3));
}

/* ================================================================
 *  Alarm_Process — LED 驱动 (5 条)
 * ================================================================ */

void test_process_steady_mode(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_2);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(LED_STEP * 10, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    mock_tick_advance(500);
    Alarm_Process();
    /* 常亮模式: PWM 不变 */
    TEST_ASSERT_EQUAL_UINT32(LED_STEP * 10, mock_pwm_get(&htim2, TIM_CHANNEL_2));
}

void test_process_blink_500ms(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_3);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(LED_STEP * 10, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    mock_tick_advance(500);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(0, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    mock_tick_advance(500);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(LED_STEP * 10, mock_pwm_get(&htim2, TIM_CHANNEL_2));
}

void test_process_fast_blink_100ms(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_4);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(LED_STEP * 10, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    mock_tick_advance(100);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(0, mock_pwm_get(&htim2, TIM_CHANNEL_2));
    mock_tick_advance(100);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(LED_STEP * 10, mock_pwm_get(&htim2, TIM_CHANNEL_2));
}

void test_process_servo_holds(void)
{
    Alarm_Init();
    Alarm_SetLevel(ALERT_LEVEL_4);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(SERVO_90, mock_pwm_get(&htim3, TIM_CHANNEL_3));
    mock_tick_advance(1000);
    Alarm_Process();
    TEST_ASSERT_EQUAL_UINT32(SERVO_90, mock_pwm_get(&htim3, TIM_CHANNEL_3));
}

/* ================================================================
 *  Test Runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();
    /* CalcLevel */
    RUN_TEST(test_calc_L0_normal);
    RUN_TEST(test_calc_L0_boundary);
    RUN_TEST(test_calc_L1_range);
    RUN_TEST(test_calc_L1_L2_boundary);
    RUN_TEST(test_calc_L2_range);
    RUN_TEST(test_calc_L2_L3_boundary);
    RUN_TEST(test_calc_L3_range);
    RUN_TEST(test_calc_L3_L4_boundary);
    RUN_TEST(test_calc_L4_dead);
    /* SetLevel */
    RUN_TEST(test_init_safe_state);
    RUN_TEST(test_set_L0_all_off);
    RUN_TEST(test_set_L1_led_dim);
    RUN_TEST(test_set_L2_led_bright);
    RUN_TEST(test_set_L3_servo_45);
    RUN_TEST(test_set_L4_buzzer_on);
    RUN_TEST(test_set_idempotent);
    RUN_TEST(test_escalation_0_to_3);
    /* Process */
    RUN_TEST(test_process_steady_mode);
    RUN_TEST(test_process_blink_500ms);
    RUN_TEST(test_process_fast_blink_100ms);
    RUN_TEST(test_process_servo_holds);
    return UNITY_END();
}
