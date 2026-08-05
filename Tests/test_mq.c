/**
 * @brief  MQ 气体传感器状态机单元测试
 *
 * 被测源文件: ../User/mq/mq.c
 * 覆盖: PREHEAT→STABLE / STABLE→FAULT / FAULT→STABLE / 故障超时
 */

#include "unity.h"
#include "mocks/mock_hal.h"

/* ---- 被测函数 ---- */
extern void MQ_Init(void *s, uint32_t preheat_ms);
extern void MQ_Update(void *s, uint16_t raw_adc);

/* ---- MQ 状态枚举 (与 mq.h 一致) ---- */
#define MQ_STATE_PREHEAT  0
#define MQ_STATE_STABLE   1
#define MQ_STATE_FAULT    2

/* ---- MQ_Sensor 结构体 (与 mq.h 一致, 字节级兼容) ---- */
typedef struct {
    uint32_t state;
    uint32_t start_tick;
    uint32_t fault_tick;
    uint32_t preheat_ms;
    uint8_t  stable;
    uint8_t  _pad[3];
} MQ_Sensor;

/* ---- 常数 ---- */
#define MQ_FAULT_ADC_MIN    100
#define MQ_FAULT_TIMEOUT_MS 5000

/* ================================================================
 *  setUp / tearDown
 * ================================================================ */
static MQ_Sensor mq;

void setUp(void)
{
    mock_all_reset();
    mock_tick_init(0);
    memset(&mq, 0, sizeof(mq));
}

void tearDown(void) {}

/* ================================================================
 *  MQ_Init — 初始化
 * ================================================================ */

void test_init_preheat_state(void)
{
    MQ_Init(&mq, 120000);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_PREHEAT, mq.state);
    TEST_ASSERT_EQUAL_UINT32(0,   mq.start_tick);
    TEST_ASSERT_EQUAL_UINT32(0,   mq.fault_tick);
    TEST_ASSERT_EQUAL_UINT32(120000, mq.preheat_ms);
    TEST_ASSERT_EQUAL_UINT8(0,    mq.stable);
}

void test_init_preheat_180s(void)
{
    MQ_Init(&mq, 180000);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_PREHEAT, mq.state);
    TEST_ASSERT_EQUAL_UINT32(180000, mq.preheat_ms);
}

/* ================================================================
 *  MQ_Update — 状态迁移
 * ================================================================ */

/* PREHEAT → STABLE: 预热时间到达 */
void test_preheat_to_stable(void)
{
    MQ_Init(&mq, 100000);
    mock_tick_advance(100000);
    MQ_Update(&mq, 500);  /* 正常 ADC 值 */
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);
    TEST_ASSERT_EQUAL_UINT8(1, mq.stable);
}

/* PREHEAT: 时间未到, 保持预热 */
void test_preheat_not_yet(void)
{
    MQ_Init(&mq, 100000);
    mock_tick_advance(50000);
    MQ_Update(&mq, 500);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_PREHEAT, mq.state);
    TEST_ASSERT_EQUAL_UINT8(0, mq.stable);
}

/* PREHEAT: 刚好到达 */
void test_preheat_exact_boundary(void)
{
    MQ_Init(&mq, 50000);
    mock_tick_advance(50000);  /* elapsed == preheat_ms */
    MQ_Update(&mq, 500);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);
}

/* STABLE: 正常读数保持 STABLE */
void test_stable_stays_stable(void)
{
    MQ_Init(&mq, 1000);
    mock_tick_advance(2000);
    MQ_Update(&mq, 500);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);
    mock_tick_advance(1000);
    MQ_Update(&mq, 600);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);
}

/* STABLE → FAULT: ADC 持续低于阈值 5s */
void test_stable_to_fault(void)
{
    MQ_Init(&mq, 1000);
    mock_tick_advance(2000);
    MQ_Update(&mq, 200);  /* 正常值, 进入 STABLE */
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);

    /* ADC 降到 50 (< MIN=100), 开始计时 */
    mock_tick_advance(1000);
    MQ_Update(&mq, 50);   /* 首次异常, 记录 fault_tick */
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);  /* 还没超时 */

    /* 持续异常 ≥ 5s */
    mock_tick_advance(5000);
    MQ_Update(&mq, 50);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_FAULT, mq.state);
    TEST_ASSERT_EQUAL_UINT8(0, mq.stable);
}

/* STABLE: ADC 短暂低于阈值但恢复 → 不进入 FAULT */
void test_stable_transient_low(void)
{
    MQ_Init(&mq, 1000);
    mock_tick_advance(2000);
    MQ_Update(&mq, 500);  /* STABLE */
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);

    mock_tick_advance(1000);
    MQ_Update(&mq, 50);   /* 异常 */
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);  /* 计时中 */

    mock_tick_advance(1000);
    MQ_Update(&mq, 500);  /* 恢复 — 清零 fault_tick */
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);

    /* 5s 后仍 STABLE (没有持续异常) */
    mock_tick_advance(5000);
    MQ_Update(&mq, 500);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);
}

/* FAULT → STABLE: ADC 恢复正常值 */
void test_fault_to_stable_recovery(void)
{
    /* 先进入 FAULT */
    MQ_Init(&mq, 1000);
    mock_tick_advance(2000);
    MQ_Update(&mq, 50);
    mock_tick_advance(5000);
    MQ_Update(&mq, 50);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_FAULT, mq.state);

    /* 恢复 */
    MQ_Update(&mq, 500);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);
    TEST_ASSERT_EQUAL_UINT8(1, mq.stable);
}

/* FAULT: ADC 仍低于阈值, 保持 FAULT */
void test_fault_stays_fault(void)
{
    MQ_Init(&mq, 1000);
    mock_tick_advance(2000);
    MQ_Update(&mq, 50);
    mock_tick_advance(5000);
    MQ_Update(&mq, 50);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_FAULT, mq.state);

    mock_tick_advance(1000);
    MQ_Update(&mq, 50);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_FAULT, mq.state);
}

/* FAULT: ADC 刚好等于阈值 → 恢复 */
void test_fault_boundary_adc(void)
{
    MQ_Init(&mq, 1000);
    mock_tick_advance(2000);
    MQ_Update(&mq, 50);
    mock_tick_advance(5000);
    MQ_Update(&mq, 50);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_FAULT, mq.state);

    /* ADC = 100 — 刚好等于阈值 */
    MQ_Update(&mq, 100);
    TEST_ASSERT_EQUAL_UINT32(MQ_STATE_STABLE, mq.state);
}

/* ================================================================
 *  Test Runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_preheat_state);
    RUN_TEST(test_init_preheat_180s);
    RUN_TEST(test_preheat_to_stable);
    RUN_TEST(test_preheat_not_yet);
    RUN_TEST(test_preheat_exact_boundary);
    RUN_TEST(test_stable_stays_stable);
    RUN_TEST(test_stable_to_fault);
    RUN_TEST(test_stable_transient_low);
    RUN_TEST(test_fault_to_stable_recovery);
    RUN_TEST(test_fault_stays_fault);
    RUN_TEST(test_fault_boundary_adc);
    return UNITY_END();
}
