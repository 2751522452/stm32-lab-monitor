#ifndef __SPI_W25Q64_H
#define __SPI_W25Q64_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "gpio/gpio.h"
#include "spi/spi.h"

/* 初始化 */
void W25Q64_Init(void);

void W25Q64_WriteEnable(void);//写使能
uint32_t W25Q64_ReadID(void);//读取 JEDEC ID
uint8_t W25Q64_ReadStatus1(void);// 读取 W25Q64 的状态
void W25Q64_WaitBusy(void);// 等待 Busy
void W25Q64_SectorErase(uint32_t addr);//擦除
void W25Q64_Read(uint32_t addr, uint8_t *buf, uint16_t len);//读取
void W25Q64_PageProgram(uint32_t addr,uint8_t *buf,uint16_t len);//页编程
void W25Q64_Write(uint32_t addr, uint8_t *buf, uint32_t len);//写入

#ifdef __cplusplus
}
#endif

#endif
