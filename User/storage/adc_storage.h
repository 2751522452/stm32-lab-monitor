#ifndef __ADC_STORAGE_H
#define __ADC_STORAGE_H

#include "main.h"

/* ADC数据记录 (12 bytes, 自然对齐) */
typedef struct
{
    uint32_t timestamp;
    uint16_t ps;
    uint16_t mq135;
    uint16_t mq2;
    uint8_t  crc8;       /* CRC-8 of bytes[0..9] (timestamp+ps+mq135+mq2, 10 bytes) */
    uint8_t  rsvd;       /* 保留, 写入 0x00 */
} ADC_Record;

/* 状态 */
#define ADC_SAVE_OK        0
#define ADC_SAVE_ERROR     2

void     ADC_Storage_Init(void);
uint8_t  ADC_Save_Record(ADC_Record *record);
void     ADC_Read_Record(uint32_t index, ADC_Record *record);
void     ADC_Storage_Format(void);

uint32_t ADC_Get_Write_Address(void);
uint32_t ADC_Get_Record_Count(void);

#endif /* __ADC_STORAGE_H */
