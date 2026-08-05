/**
 * @brief  传感器预热状态机单元测试
 *
 * 被测源文件: ../User/sensor/sensor.c (Sensor_WarmupInit / Sensor_WarmupUpdate)
 * 覆盖: PREHEAT→STABLE / STABLE→FAULT / FAULT→STABLE / 超时 / 恢复
 */

#include "unity.h"
#include "mocks/mock_hal.h"

/* ---- 被测函数 (来自 sensor.c, 编译时链接) ---- */
extern void Sensor_WarmupInit(void *ws, uint32_t preheat_ms);
extern void Sensor_WarmupUpdate(void *ws, uint16_t raw_adc);

/* ---- 类型与常数 (与 sensor.h 保持一致) ---- */
#define WARMUP_PREHEAT  0
#define WARMUP_STABLE   1
#define WARMUP_FAULT    2

typedef struct {
    uint32_t state;
    uint32_t start_tick;
    uint32_t fault_tick;
    uint32_t preheat_ms;
    uint8_t  ready;
    uint8_t  _pad[3];
} WarmupSensor;

#define WARMUP_ADC_MIN     100
#define WARMUP_TIMEOUT_MS  5000

/* ---- 测试夹具 ---- */
static WarmupSensor ws;

void setUp(void)
{
    mock_all_reset();
    mock_tick_init(0);
    memset(&ws, 0, sizeof(ws));
}

void tearDown(void) {}

/* ================================================================
 *  Sensor_WarmupInit
 * ================================================================ */

void test_init_preheat_state(void)
{
    Sensor_WarmupInit(&ws, 120000);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_PREHEAT, ws.state);
    TEST_ASSERT_EQUAL_UINT32(0,       ws.start_tick);
    TEST_ASSERT_EQUAL_UINT32(0,       ws.fault_tick);
    TEST_ASSERT_EQUAL_UINT32(120000,  ws.preheat_ms);
    TEST_ASSERT_EQUAL_UINT8(0,        ws.ready);
}

void test_init_preheat_180s(void)
{
    Sensor_WarmupInit(&ws, 180000);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_PREHEAT, ws.state);
    TEST_ASSERT_EQUAL_UINT32(180000, ws.preheat_ms);
}

/* ================================================================
 *  Sensor_WarmupUpdate — 状态迁移
 * ================================================================ */

void test_preheat_to_stable(void)
{
    Sensor_WarmupInit(&ws, 100000);
    mock_tick_advance(100000);
    Sensor_WarmupUpdate(&ws, 500);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);
    TEST_ASSERT_EQUAL_UINT8(1, ws.ready);
}

void test_preheat_not_yet(void)
{
    Sensor_WarmupInit(&ws, 100000);
    mock_tick_advance(50000);
    Sensor_WarmupUpdate(&ws, 500);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_PREHEAT, ws.state);
    TEST_ASSERT_EQUAL_UINT8(0, ws.ready);
}

void test_preheat_exact_boundary(void)
{
    Sensor_WarmupInit(&ws, 50000);
    mock_tick_advance(50000);
    Sensor_WarmupUpdate(&ws, 500);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);
}

void test_stable_stays_stable(void)
{
    Sensor_WarmupInit(&ws, 1000);
    mock_tick_advance(2000);
    Sensor_WarmupUpdate(&ws, 500);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);
    mock_tick_advance(1000);
    Sensor_WarmupUpdate(&ws, 600);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);
}

void test_stable_to_fault(void)
{
    Sensor_WarmupInit(&ws, 1000);
    mock_tick_advance(2000);
    Sensor_WarmupUpdate(&ws, 200);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);

    mock_tick_advance(1000);
    Sensor_WarmupUpdate(&ws, 50);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);  /* 还没超时 */

    mock_tick_advance(5000);
    Sensor_WarmupUpdate(&ws, 50);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_FAULT, ws.state);
    TEST_ASSERT_EQUAL_UINT8(0, ws.ready);
}

void test_stable_transient_low(void)
{
    Sensor_WarmupInit(&ws, 1000);
    mock_tick_advance(2000);
    Sensor_WarmupUpdate(&ws, 500);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);

    mock_tick_advance(1000);
    Sensor_WarmupUpdate(&ws, 50);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);  /* 计时中 */

    mock_tick_advance(1000);
    Sensor_WarmupUpdate(&ws, 500);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);

    mock_tick_advance(5000);
    Sensor_WarmupUpdate(&ws, 500);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);
}

void test_fault_to_stable_recovery(void)
{
    Sensor_WarmupInit(&ws, 1000);
    mock_tick_advance(2000);
    Sensor_WarmupUpdate(&ws, 50);
    mock_tick_advance(5000);
    Sensor_WarmupUpdate(&ws, 50);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_FAULT, ws.state);

    Sensor_WarmupUpdate(&ws, 500);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);
    TEST_ASSERT_EQUAL_UINT8(1, ws.ready);
}

void test_fault_stays_fault(void)
{
    Sensor_WarmupInit(&ws, 1000);
    mock_tick_advance(2000);
    Sensor_WarmupUpdate(&ws, 50);
    mock_tick_advance(5000);
    Sensor_WarmupUpdate(&ws, 50);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_FAULT, ws.state);

    mock_tick_advance(1000);
    Sensor_WarmupUpdate(&ws, 50);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_FAULT, ws.state);
}

void test_fault_boundary_adc(void)
{
    Sensor_WarmupInit(&ws, 1000);
    mock_tick_advance(2000);
    Sensor_WarmupUpdate(&ws, 50);
    mock_tick_advance(5000);
    Sensor_WarmupUpdate(&ws, 50);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_FAULT, ws.state);

    Sensor_WarmupUpdate(&ws, 100);
    TEST_ASSERT_EQUAL_UINT32(WARMUP_STABLE, ws.state);
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
