/* Includes ------------------------------------------------------------------*/
#include "alarm.h"
#include "tim/tim.h"

/* 硬件参数 -------------------------------------------------------------------*/
#define SERVO_0     500
#define SERVO_45    1500
#define SERVO_90    2500
#define LED_STEP    100

/* 告警内部状态 ---------------------------------------------------------------*/
typedef struct
{
		AlertLevel level;
		uint16_t   led_pwm;
		uint8_t    led_mode;      /* 0=常亮  1=闪烁 */
		uint16_t   led_period;    /* 闪烁周期 ms */
		uint16_t   servo_target;
} AlarmState;

static AlarmState alarm;

/* 外部引用 — TIM 句柄 */
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

/* Public functions ----------------------------------------------------------*/

/**
	* @brief  初始化告警模块（LED PWM 启动，舵机归零，蜂鸣器关闭）
	*/
void Alarm_Init(void)
{
		alarm.level        = ALERT_LEVEL_0;
		alarm.led_pwm      = 0;
		alarm.led_mode     = 0;
		alarm.led_period   = 500;
		alarm.servo_target = SERVO_0;

		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
		HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, SERVO_0);
		HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
}

/**
	* @brief  设置告警等级（由外部根据传感器数据计算后调用）
	*/
void Alarm_SetLevel(AlertLevel level)
{
		if (level == alarm.level)
				return;

		alarm.level = level;

		switch (level)
		{
		case ALERT_LEVEL_0:
				alarm.led_mode     = 0;
				alarm.led_pwm      = 0;
				alarm.servo_target = SERVO_0;
				HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
				break;

		case ALERT_LEVEL_1:
				alarm.led_mode     = 0;
				alarm.led_pwm      = LED_STEP;
				alarm.servo_target = SERVO_0;
				HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
				break;

		case ALERT_LEVEL_2:
				alarm.led_mode     = 0;
				alarm.led_pwm      = LED_STEP * 10;
				alarm.servo_target = SERVO_0;
				HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
				break;

		case ALERT_LEVEL_3:
				alarm.led_mode     = 1;
				alarm.led_pwm      = LED_STEP * 10;
				alarm.led_period   = 500;
				alarm.servo_target = SERVO_45;
				HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
				break;

		case ALERT_LEVEL_4:
				alarm.led_mode     = 1;
				alarm.led_pwm      = LED_STEP * 10;
				alarm.led_period   = 100;
				alarm.servo_target = SERVO_90;
				HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
				break;
		}
}

/**
	* @brief  驱动 LED/舵机（每轮主循环调用）
	* @note   基于 HAL_GetTick() 非阻塞闪烁
	*/
void Alarm_Process(void)
{
		static uint32_t last_tick = 0;
		static uint8_t  blink_state = 0;

		/* LED 驱动 */
		if (alarm.led_mode == 0)
		{
				/* 常亮模式 */
				__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, alarm.led_pwm);
		}
		else
		{
				/* 闪烁模式 */
				uint32_t now = HAL_GetTick();
				if (now - last_tick >= alarm.led_period)
				{
						last_tick = now;
						blink_state = !blink_state;
						__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2,
																	blink_state ? alarm.led_pwm : 0);
				}
		}

		/* 舵机驱动 */
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, alarm.servo_target);
}

/**
	* @brief  根据光照电压计算告警等级
	* @param  voltage 光敏传感器电压 (V)
	* @retval AlertLevel
	*/
AlertLevel Alarm_CalcLevel(float voltage)
{
		if      (voltage > 2.6f) return ALERT_LEVEL_0;
		else if (voltage > 2.3f) return ALERT_LEVEL_1;
		else if (voltage > 2.0f) return ALERT_LEVEL_2;
		else if (voltage > 0.0f) return ALERT_LEVEL_3;
		else                     return ALERT_LEVEL_4;
}
