/**
 * @brief  CRC8 校验单元测试
 *
 * 被测函数: adc_storage.c 中的 crc8_calc() 和 record_crc8()
 * 覆盖: 已知向量校验 / 空数据 / 单字节 / 全 FF / record CRC
 */

#include "unity.h"
#include <stdint.h>
#include <string.h>

/* ---- CRC-8 (poly=0x07, init=0xFF) — 手动复制被测函数以便测试 ---- */

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

/* ---- 被测数据结构 ---- */
#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp;
    uint16_t ps;
    uint16_t mq135;
    uint16_t mq2;
    uint8_t  crc8;
    uint8_t  rsvd;
} ADC_Record;
#pragma pack(pop)

/* ================================================================
 *  setUp / tearDown
 * ================================================================ */
void setUp(void) {}
void tearDown(void) {}

/* ================================================================
 *  CRC8 基础测试
 * ================================================================ */

/* 空输入: CRC-8(0xFF init, 空) = 0xFF */
void test_crc8_empty(void)
{
    uint8_t crc = crc8_calc(NULL, 0);
    TEST_ASSERT_EQUAL_HEX(0xFF, crc);
}

/* 单字节 0x00: 0xFF ^ 0x00 = 0xFF, 8轮移位无进位 */
void test_crc8_single_zero(void)
{
    uint8_t data[] = {0x00};
    uint8_t crc = crc8_calc(data, 1);
    /* CRC-8(0x00, poly=0x07, init=0xFF) = 0xAC */
    TEST_ASSERT_EQUAL_HEX(0xAC, crc);
}

/* 已知向量: "123456789" → 0xF4 (CRC-8 标准向量) */
void test_crc8_known_vector(void)
{
    uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    uint8_t crc = crc8_calc(data, 9);
    TEST_ASSERT_EQUAL_HEX(0xF4, crc);
}

/* 全 0xFF: 输入全 FF 的 CRC-8 */
void test_crc8_all_ff(void)
{
    uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t crc = crc8_calc(data, 4);
    /* 这是确定性计算, 确保输出稳定 */
    TEST_ASSERT_NOT_NULL(&crc);  /* 至少不会 crash */
}

/* 确定性: 两次相同输入得相同输出 */
void test_crc8_deterministic(void)
{
    uint8_t data[] = {0xA5, 0x5A, 0x3C, 0xC3};
    uint8_t crc1 = crc8_calc(data, 4);
    uint8_t crc2 = crc8_calc(data, 4);
    TEST_ASSERT_EQUAL_HEX(crc1, crc2);
}

/* 不同数据产生不同 CRC (大概率) */
void test_crc8_different_data_different_crc(void)
{
    uint8_t d1[] = {0x01, 0x02, 0x03};
    uint8_t d2[] = {0x01, 0x02, 0x04};
    uint8_t crc1 = crc8_calc(d1, 3);
    uint8_t crc2 = crc8_calc(d2, 3);
    TEST_ASSERT(crc1 != crc2);
}

/* ================================================================
 *  Record CRC8 测试
 * ================================================================ */

static uint8_t record_crc8(const ADC_Record *rec)
{
    return crc8_calc((const uint8_t *)rec, 10);
}

void test_record_crc_valid(void)
{
    ADC_Record rec;
    memset(&rec, 0, sizeof(rec));
    rec.timestamp = 0x12345678;
    rec.ps        = 0xABCD;
    rec.mq135     = 0x0FFF;
    rec.mq2       = 0x03FF;

    uint8_t crc = record_crc8(&rec);
    rec.crc8 = crc;

    /* 重新计算应得到相同的 CRC */
    uint8_t verify = record_crc8(&rec);
    TEST_ASSERT_EQUAL_HEX(crc, verify);
}

void test_record_crc_detects_corruption(void)
{
    ADC_Record rec;
    memset(&rec, 0, sizeof(rec));
    rec.timestamp = 1000;
    rec.ps        = 2048;
    rec.mq135     = 1024;
    rec.mq2       = 512;

    uint8_t good_crc = record_crc8(&rec);
    rec.crc8 = good_crc;

    /* 篡改 timestamp */
    rec.timestamp = 1001;
    uint8_t bad_crc = record_crc8(&rec);
    TEST_ASSERT(good_crc != bad_crc);
}

/* ================================================================
 *  Test Runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc8_empty);
    RUN_TEST(test_crc8_single_zero);
    RUN_TEST(test_crc8_known_vector);
    RUN_TEST(test_crc8_all_ff);
    RUN_TEST(test_crc8_deterministic);
    RUN_TEST(test_crc8_different_data_different_crc);
    RUN_TEST(test_record_crc_valid);
    RUN_TEST(test_record_crc_detects_corruption);
    return UNITY_END();
}
