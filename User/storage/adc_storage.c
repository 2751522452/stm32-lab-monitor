/**
 * @brief  W25Q64 双扇区交替提交 — 掉电保护存储
 *
 * 扇区布局:
 * ┌───────────────┬──────────────────────────────┐
 * │ SectorHeader  │  ADC_Record × N              │
 * │   12 Bytes    │                              │
 * └───────────────┴──────────────────────────────┘
 *
 * 状态迁移 (单向 1→0, 不违反 Flash 物理约束):
 *   0xFF (Idle) → 0x7F (Writing) → 0x3F (Valid)
 *
 * 流程:
 *   1. 擦除备用扇区 (→全 0xFF)
 *   2. 写入头部 (status = 0x7F Writing)
 *   3. 切换 cur/old 指针
 *   4. 写入第一条记录 (CRC8 校验通过)
 *   5. 提交扇区 (status 0x7F → 0x3F, 仅 bit6 1→0)
 *   6. 逐条写入后续记录 (Header 不再触碰)
 *
 * STATUS_VALID 的语义:
 *   "该扇区已完成初始化，且至少包含一条 CRC 校验通过的有效记录"
 *
 * 掉电恢复:
 *   status=0x3F, header CRC8 通过 → 有效扇区, record CRC8 扫描统计记录数
 *   status=0x7F                   → 写入中断, 有旧扇区则回退, 无则扫描恢复
 *   status=0xFF                   → 空闲, 跳过
 */

#include "adc_storage.h"
#include "spi_w25q64/spi_w25q64.h"
#include <string.h>

/* ================================================================
 *  常量
 * ================================================================ */

#define FLASH_SECTOR_SIZE      4096

#define SECTOR_A_ADDR          0x000000
#define SECTOR_B_ADDR          0x001000

#define SECTOR_MAGIC           0xA5A5A5A5

/* 状态 — 单向 1→0 迁移 */
#define STATUS_IDLE            0xFF
#define STATUS_WRITING         0x7F
#define STATUS_VALID           0x3F

/* ================================================================
 *  扇区头部 (12 Bytes, 自然对齐)
 * ================================================================ */
typedef struct {
    uint32_t magic;          /* SECTOR_MAGIC                       */
    uint32_t version;        /* 单调递增版本号                     */
    uint8_t  status;         /* STATUS_IDLE / WRITING / VALID      */
    uint8_t  header_crc8;    /* CRC-8 of magic+version (8 bytes)   */
    uint16_t pad;            /* 保留 0xFFFF                        */
} SectorHeader;

/* 每扇区可存记录数 */
#define RECORDS_PER_SECTOR \
    ((FLASH_SECTOR_SIZE - sizeof(SectorHeader)) / sizeof(ADC_Record))

/* ================================================================
 *  CRC-8 (poly=0x07, x^8+x^2+x+1)
 * ================================================================ */

static uint8_t crc8_calc(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ 0x07);
            else
                crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* 计算一条记录的 CRC8 (覆盖前 10 bytes: timestamp+ps+mq135+mq2) */
static uint8_t record_crc8(const ADC_Record *rec)
{
    return crc8_calc((const uint8_t *)rec, 10);
}

/* ================================================================
 *  运行时状态
 * ================================================================ */
static uint32_t cur_sector;       /* 当前扇区基地址    */
static uint32_t old_sector;       /* 备用/旧扇区基地址 */
static uint32_t cur_version;      /* 当前版本号        */
static uint32_t cur_count;        /* 当前扇区记录数    */
static uint32_t old_count;        /* 旧扇区记录数      */

/* ================================================================
 *  内部: 扇区头部读写 & 验证
 * ================================================================ */

/* 验证扇区头部 — 返回 1=有效 */
static int header_validate(uint32_t addr, uint32_t *ver_out)
{
    SectorHeader hdr;
    uint8_t buf[8], crc;

    W25Q64_Read(addr, (uint8_t *)&hdr, sizeof(hdr));

    if (hdr.magic != SECTOR_MAGIC)  return 0;
    if (hdr.status != STATUS_VALID) return 0;

    buf[0] = (uint8_t) hdr.magic;
    buf[1] = (uint8_t)(hdr.magic >> 8);
    buf[2] = (uint8_t)(hdr.magic >> 16);
    buf[3] = (uint8_t)(hdr.magic >> 24);
    buf[4] = (uint8_t) hdr.version;
    buf[5] = (uint8_t)(hdr.version >> 8);
    buf[6] = (uint8_t)(hdr.version >> 16);
    buf[7] = (uint8_t)(hdr.version >> 24);
    crc = crc8_calc(buf, 8);

    if (hdr.header_crc8 != crc) return 0;

    if (ver_out) *ver_out = hdr.version;
    return 1;
}

/* 写扇区头部 (status 可 0x7F/0x3F) */
static void header_write(uint32_t addr, uint32_t ver, uint8_t status)
{
    SectorHeader hdr;
    uint8_t buf[8], crc;

    hdr.magic   = SECTOR_MAGIC;
    hdr.version = ver;
    hdr.status  = status;
    hdr.pad     = 0xFFFF;

    buf[0] = (uint8_t) hdr.magic;
    buf[1] = (uint8_t)(hdr.magic >> 8);
    buf[2] = (uint8_t)(hdr.magic >> 16);
    buf[3] = (uint8_t)(hdr.magic >> 24);
    buf[4] = (uint8_t) hdr.version;
    buf[5] = (uint8_t)(hdr.version >> 8);
    buf[6] = (uint8_t)(hdr.version >> 16);
    buf[7] = (uint8_t)(hdr.version >> 24);
    crc = crc8_calc(buf, 8);
    hdr.header_crc8 = crc;

    W25Q64_Write(addr, (uint8_t *)&hdr, sizeof(hdr));
}

/* 提交扇区: status 0x7F → 0x3F (仅写 1 字节) */
static void header_commit(uint32_t addr)
{
    uint8_t val = STATUS_VALID;
    uint32_t status_addr = addr + offsetof(SectorHeader, status);
    W25Q64_Write(status_addr, &val, 1);
}

/* 判断整条记录是否全 0xFF (Flash 擦除态) */
static int record_is_empty(const ADC_Record *rec)
{
    const uint8_t *p = (const uint8_t *)rec;
    for (int i = 0; i < (int)sizeof(ADC_Record); i++) {
        if (p[i] != 0xFF) return 0;
    }
    return 1;
}

/* 扫描扇区内有效记录数 — 全 FF 判空 + CRC8 校验 */
static uint32_t scan_count(uint32_t sector_addr)
{
    uint32_t i;
    uint32_t base = sector_addr + sizeof(SectorHeader);

    for (i = 0; i < RECORDS_PER_SECTOR; i++) {
        uint32_t   ra = base + i * sizeof(ADC_Record);
        ADC_Record rec;

        W25Q64_Read(ra, (uint8_t *)&rec, sizeof(ADC_Record));

        if (record_is_empty(&rec))  break;               /* 全 0xFF → 空区域 */
        if (record_crc8(&rec) != rec.crc8) break;        /* CRC 错 → 掉电半写 */
    }
    return i;
}

/* ================================================================
 *  公开 API
 * ================================================================ */

void ADC_Storage_Init(void)
{
    uint32_t ver_a = 0, ver_b = 0;
    int      va = header_validate(SECTOR_A_ADDR, &ver_a);
    int      vb = header_validate(SECTOR_B_ADDR, &ver_b);

    if (!va && !vb) {
        /* 首次上电: A=当前, B=备用 */
        cur_sector  = SECTOR_A_ADDR;
        old_sector  = SECTOR_B_ADDR;
        cur_version = 1;
        cur_count   = 0;
        old_count   = 0;

        W25Q64_SectorErase(SECTOR_A_ADDR);
        W25Q64_SectorErase(SECTOR_B_ADDR);
        header_write(SECTOR_A_ADDR, cur_version, STATUS_WRITING);
        /* 不 commit — 等第一条记录写完后由 ADC_Save_Record 提交 */
    }
    else if (va && !vb) {
        cur_sector  = SECTOR_A_ADDR;
        old_sector  = SECTOR_B_ADDR;
        cur_version = ver_a;
        cur_count   = scan_count(SECTOR_A_ADDR);
        old_count   = 0;
    }
    else if (!va && vb) {
        cur_sector  = SECTOR_B_ADDR;
        old_sector  = SECTOR_A_ADDR;
        cur_version = ver_b;
        cur_count   = scan_count(SECTOR_B_ADDR);
        old_count   = 0;
    }
    else {
        /* 两个都有效 — 版本号大的为当前 */
        if (ver_a > ver_b) {
            cur_sector  = SECTOR_A_ADDR;
            old_sector  = SECTOR_B_ADDR;
            cur_version = ver_a;
            old_count   = scan_count(SECTOR_B_ADDR);
        } else {
            cur_sector  = SECTOR_B_ADDR;
            old_sector  = SECTOR_A_ADDR;
            cur_version = ver_b;
            old_count   = scan_count(SECTOR_A_ADDR);
        }
        cur_count = scan_count(cur_sector);
    }

    /* 若是 Writing 状态 (掉电中断), 恢复或回退 */
    {
        SectorHeader hdr;
        W25Q64_Read(cur_sector, (uint8_t *)&hdr, sizeof(hdr));
        if (hdr.status == STATUS_WRITING) {
            if (old_count > 0) {
                /* 有旧扇区可回退 → 丢弃当前, 回退旧扇区 */
                uint32_t tmp_a = cur_sector;
                cur_sector  = old_sector;
                old_sector  = tmp_a;
                cur_version = (ver_a > ver_b) ? ver_a : ver_b;
                cur_count   = old_count;
                old_count   = 0;
            } else {
                /* 首次上电中断或空备用扇区 → 提交当前扇区并扫描 */
                header_commit(cur_sector);
                cur_count = scan_count(cur_sector);
            }
        }
    }
}

uint8_t ADC_Save_Record(ADC_Record *record)
{
    /* 扇区满 → 切扇区 */
    if (cur_count >= RECORDS_PER_SECTOR) {
        uint32_t new_sector = old_sector;
        uint32_t new_ver    = cur_version + 1;

        /* ① 擦除备用扇区, 写头部 WRITING */
        W25Q64_SectorErase(new_sector);
        header_write(new_sector, new_ver, STATUS_WRITING);

        /* ② 切换: 新扇区成为当前 (仍为 WRITING, 等第一条记录写完后提交) */
        old_sector  = cur_sector;
        old_count   = cur_count;
        cur_sector  = new_sector;
        cur_version = new_ver;
        cur_count   = 0;
    }

    /* 写记录 */
    {
        uint32_t addr = cur_sector + sizeof(SectorHeader)
                      + cur_count * sizeof(ADC_Record);

        record->crc8 = record_crc8(record);
        record->rsvd = 0x00;
        W25Q64_Write(addr, (uint8_t *)record, sizeof(ADC_Record));
        cur_count++;
    }

    /* 扇区首条记录写入完成 → 提交: WRITING → VALID */
    if (cur_count == 1) {
        header_commit(cur_sector);
    }

    return ADC_SAVE_OK;
}

void ADC_Read_Record(uint32_t index, ADC_Record *record)
{
    uint32_t total = old_count + cur_count;

    if (index >= total) {
        memset(record, 0xFF, sizeof(ADC_Record));
        return;
    }

    uint32_t sector_addr;
    uint32_t idx_in_sector;

    if (index < old_count) {
        sector_addr   = old_sector;
        idx_in_sector = index;
    } else {
        sector_addr   = cur_sector;
        idx_in_sector = index - old_count;
    }

    uint32_t addr = sector_addr + sizeof(SectorHeader)
                  + idx_in_sector * sizeof(ADC_Record);
    W25Q64_Read(addr, (uint8_t *)record, sizeof(ADC_Record));
}

void ADC_Storage_Format(void)
{
    W25Q64_SectorErase(SECTOR_A_ADDR);
    W25Q64_SectorErase(SECTOR_B_ADDR);

    cur_sector  = SECTOR_A_ADDR;
    old_sector  = SECTOR_B_ADDR;
    cur_version = 1;
    cur_count   = 0;
    old_count   = 0;

    header_write(SECTOR_A_ADDR, cur_version, STATUS_WRITING);
}

uint32_t ADC_Get_Write_Address(void)
{
    return cur_sector + sizeof(SectorHeader)
         + cur_count * sizeof(ADC_Record);
}

uint32_t ADC_Get_Record_Count(void)
{
    return old_count + cur_count;
}
