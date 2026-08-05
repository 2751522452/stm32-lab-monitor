/**
 * @brief  W25Q64 SPI Flash 模拟层 — PC 端单元测试用
 *
 * 使用内存缓冲区模拟 8MB Flash 存储 (仅测试目的保留 8KB)
 * 支持: Read / Write / SectorErase / 读取查询
 */

#ifndef MOCK_FLASH_H
#define MOCK_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 模拟 Flash 容量 (仅测试用 8KB = 2 扇区) ---- */
#define MOCK_FLASH_SIZE       8192
#define MOCK_SECTOR_SIZE      4096

/* ---- 模拟控制 API ---- */
void mock_flash_reset(void);
void mock_flash_init_clean(void);
void mock_flash_init_blank(void);

/* ---- 读取模拟 Flash 内容 (供测试验证) ---- */
uint8_t mock_flash_read_byte(uint32_t addr);
void    mock_flash_read_range(uint32_t addr, uint8_t *buf, uint32_t len);
void    mock_flash_print_sector(uint32_t addr, uint32_t bytes);

/* ---- W25Q64 API 模拟实现 ---- */
void    W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len);
void    W25Q64_Write(uint32_t addr, const uint8_t *buf, uint32_t len);
void    W25Q64_SectorErase(uint32_t addr);
uint32_t W25Q64_ReadID(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_FLASH_H */
