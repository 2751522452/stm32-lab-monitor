/**
 * @brief  W25Q64 SPI Flash 模拟层实现
 */

#include "mock_flash.h"
#include <string.h>
#include <stdio.h>

/* ---- 模拟 Flash 存储 ---- */
static uint8_t flash_buf[MOCK_FLASH_SIZE];

/* ===================================================================
 *  模拟控制 API
 * =================================================================== */

void mock_flash_reset(void)
{
    memset(flash_buf, 0xFF, sizeof(flash_buf));
}

void mock_flash_init_clean(void)
{
    memset(flash_buf, 0xFF, sizeof(flash_buf));
}

void mock_flash_init_blank(void)
{
    memset(flash_buf, 0xFF, sizeof(flash_buf));
}

uint8_t mock_flash_read_byte(uint32_t addr)
{
    if (addr >= MOCK_FLASH_SIZE) return 0xFF;
    return flash_buf[addr];
}

void mock_flash_read_range(uint32_t addr, uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len && (addr + i) < MOCK_FLASH_SIZE; i++) {
        buf[i] = flash_buf[addr + i];
    }
}

void mock_flash_print_sector(uint32_t addr, uint32_t bytes)
{
    printf("Flash[0x%06lX]: ", (unsigned long)addr);
    for (uint32_t i = 0; i < bytes && (addr + i) < MOCK_FLASH_SIZE; i++) {
        printf("%02X ", flash_buf[addr + i]);
        if ((i + 1) % 16 == 0 && i < bytes - 1)
            printf("\n              ");
    }
    printf("\n");
}

/* ===================================================================
 *  W25Q64 API 模拟实现
 * =================================================================== */

void W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (addr + len > MOCK_FLASH_SIZE) {
        len = MOCK_FLASH_SIZE - addr;
    }
    memcpy(buf, flash_buf + addr, len);
}

void W25Q64_Write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    if (addr + len > MOCK_FLASH_SIZE) {
        len = MOCK_FLASH_SIZE - addr;
    }
    for (uint32_t i = 0; i < len; i++) {
        /* Flash 物理约束: 只能 1→0 */
        flash_buf[addr + i] &= buf[i];
    }
}

void W25Q64_SectorErase(uint32_t addr)
{
    /* 对齐到扇区边界 */
    addr = (addr / MOCK_SECTOR_SIZE) * MOCK_SECTOR_SIZE;
    if (addr + MOCK_SECTOR_SIZE <= MOCK_FLASH_SIZE) {
        memset(flash_buf + addr, 0xFF, MOCK_SECTOR_SIZE);
    }
}

uint32_t W25Q64_ReadID(void)
{
    return 0xEF4017;  /* W25Q64 标准 JEDEC ID */
}
