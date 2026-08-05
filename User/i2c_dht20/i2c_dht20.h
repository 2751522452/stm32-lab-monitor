#ifndef __DHT20_H
#define __DHT20_H

#include "stm32f1xx_hal.h"   // 按你的芯片改

typedef struct
{
    float temperature;
    float humidity;
} DHT20_Data;

uint8_t DHT20_Init(void);
uint8_t DHT20_Read(DHT20_Data *out);

#endif
