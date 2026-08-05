/* Includes ------------------------------------------------------------------*/
#include "mq/mq.h"

/* Public functions ----------------------------------------------------------*/
void MQ_Init(MQ_Sensor *s, uint32_t preheat_ms)
{
		s->state           = MQ_STATE_PREHEAT;
		s->start_tick      = HAL_GetTick();
		s->stable          = 0;
		s->preheat_ms      = preheat_ms;
		s->fault_tick      = 0;
}

void MQ_Update(MQ_Sensor *s, uint16_t raw_adc)
{
		uint32_t now     = HAL_GetTick();
		uint32_t elapsed = now - s->start_tick;

		switch (s->state)
		{
		case MQ_STATE_PREHEAT:
				if (elapsed >= s->preheat_ms)
				{
						s->state  = MQ_STATE_STABLE;
						s->stable = 1;
				}
				break;

		case MQ_STATE_STABLE:
				if (raw_adc < MQ_FAULT_ADC_MIN)
				{
						if (s->fault_tick == 0)
						{
								s->fault_tick = now;
						}
						else if (now - s->fault_tick >= MQ_FAULT_TIMEOUT_MS)
						{
								s->state  = MQ_STATE_FAULT;
								s->stable = 0;
						}
				}
				else
				{
						s->fault_tick = 0;
				}
				break;

		case MQ_STATE_FAULT:
				if (raw_adc >= MQ_FAULT_ADC_MIN)
				{
						s->state     = MQ_STATE_STABLE;
						s->stable    = 1;
						s->fault_tick = 0;
				}
				break;
		}
}
