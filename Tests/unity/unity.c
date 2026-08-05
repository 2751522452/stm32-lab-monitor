/**
 * @brief  Unity Test Framework 实现
 */

#include "unity.h"
#include <string.h>
#include <math.h>

/* ---- 全局计数 ------------------------------------------------------------- */
int UnityFailures = 0;
int UnityTests    = 0;

/* ---- 内部状态 ------------------------------------------------------------- */
static int  TestFailures = 0;
static int  TestIgnored  = 0;
static char TestFileName[128];

/* ---- setUp / tearDown 弱符号 (用户可覆盖) ----------------------------------- */
__attribute__((weak)) void setUp(void)    {}
__attribute__((weak)) void tearDown(void) {}

/* ---- 框架入口 ------------------------------------------------------------- */
void UnityBegin(const char *filename)
{
    UnityFailures = 0;
    UnityTests    = 0;
    TestFailures  = 0;
    TestIgnored   = 0;
    strncpy(TestFileName, filename, sizeof(TestFileName) - 1);
    printf("\n=== Unity Test Runner ===\n");
    printf("File: %s\n\n", TestFileName);
}

int UnityEnd(void)
{
    UnityConcludeTest();
    printf("\n=========================\n");
    printf("Tests: %d | Passed: %d | Failed: %d | Ignored: %d\n",
           UnityTests,
           UnityTests - UnityFailures,
           UnityFailures,
           TestIgnored);
    if (UnityFailures == 0) {
        printf("RESULT: ALL PASSED\n");
        return 0;
    } else {
        printf("RESULT: %d FAILURE(S)\n", UnityFailures);
        return 1;
    }
}

/* ---- 测试间切换 ----------------------------------------------------------- */
void UnityConcludeTest(void)
{
    if (TestFailures > 0) {
        UnityFailures += TestFailures;
    }
    TestFailures = 0;
    TestIgnored  = 0;
}

/* ---- 输出辅助 ------------------------------------------------------------- */
static void print_line(int line)
{
    printf("  [L%d] ", line);
}

/* ---- 基础断言 ------------------------------------------------------------- */
void UnityAssert(int condition, int line, const char *msg)
{
    if (!condition) {
        print_line(line);
        printf("FAIL: %s\n", msg);
        TestFailures++;
    }
}

/* ---- 整数断言 ------------------------------------------------------------- */
void UnityAssertEqualInt(int32_t expected, int32_t actual, int line, const char *msg)
{
    if (expected != actual) {
        print_line(line);
        printf("FAIL: Expected %d, was %d", (int)expected, (int)actual);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

void UnityAssertEqualInt32(int32_t expected, int32_t actual, int line, const char *msg)
{
    UnityAssertEqualInt(expected, actual, line, msg);
}

void UnityAssertEqualUint32(uint32_t expected, uint32_t actual, int line, const char *msg)
{
    if (expected != actual) {
        print_line(line);
        printf("FAIL: Expected %lu, was %lu", (unsigned long)expected, (unsigned long)actual);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

void UnityAssertEqualUint16(uint16_t expected, uint16_t actual, int line, const char *msg)
{
    if (expected != actual) {
        print_line(line);
        printf("FAIL: Expected %u, was %u", (unsigned)expected, (unsigned)actual);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

void UnityAssertEqualUint8(uint8_t expected, uint8_t actual, int line, const char *msg)
{
    if (expected != actual) {
        print_line(line);
        printf("FAIL: Expected %u, was %u", (unsigned)expected, (unsigned)actual);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

void UnityAssertEqualHex(uint32_t expected, uint32_t actual, int line, const char *msg)
{
    if (expected != actual) {
        print_line(line);
        printf("FAIL: Expected 0x%08lX, was 0x%08lX", (unsigned long)expected, (unsigned long)actual);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

void UnityAssertEqualHex32(uint32_t expected, uint32_t actual, int line, const char *msg)
{
    UnityAssertEqualHex(expected, actual, line, msg);
}

/* ---- 浮点断言 ------------------------------------------------------------- */
void UnityAssertEqualFloat(float expected, float actual, float delta, int line, const char *msg)
{
    float diff = (expected > actual) ? (expected - actual) : (actual - expected);
    if (diff > delta) {
        print_line(line);
        printf("FAIL: Expected %.4f +/-%.4f, was %.4f", expected, delta, actual);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

/* ---- 字符串断言 ----------------------------------------------------------- */
void UnityAssertEqualString(const char *expected, const char *actual, int line, const char *msg)
{
    if ((expected == NULL && actual != NULL) ||
        (expected != NULL && actual == NULL) ||
        (expected != NULL && actual != NULL && strcmp(expected, actual) != 0)) {
        print_line(line);
        printf("FAIL: Expected \"%s\", was \"%s\"",
               expected ? expected : "NULL",
               actual   ? actual   : "NULL");
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

/* ---- 内存断言 ------------------------------------------------------------- */
void UnityAssertEqualMemory(const void *expected, const void *actual, uint32_t len, int line, const char *msg)
{
    if ((expected == NULL && actual != NULL) ||
        (expected != NULL && actual == NULL) ||
        (expected != NULL && actual != NULL && memcmp(expected, actual, len) != 0)) {
        print_line(line);
        printf("FAIL: Memory mismatch (%lu bytes)", (unsigned long)len);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

/* ---- 指针断言 ------------------------------------------------------------- */
void UnityAssertNull(const void *ptr, int line, const char *msg)
{
    if (ptr != NULL) {
        print_line(line);
        printf("FAIL: Expected NULL, was %p", ptr);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

void UnityAssertNotNull(const void *ptr, int line, const char *msg)
{
    if (ptr == NULL) {
        print_line(line);
        printf("FAIL: Expected Not NULL");
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

/* ---- 位操作断言 ----------------------------------------------------------- */
void UnityAssertBitHigh(uint32_t mask, uint32_t reg, int line, const char *msg)
{
    if ((reg & mask) == 0) {
        print_line(line);
        printf("FAIL: Bit 0x%08lX not set in 0x%08lX", (unsigned long)mask, (unsigned long)reg);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

void UnityAssertBitLow(uint32_t mask, uint32_t reg, int line, const char *msg)
{
    if ((reg & mask) != 0) {
        print_line(line);
        printf("FAIL: Bit 0x%08lX set in 0x%08lX", (unsigned long)mask, (unsigned long)reg);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

/* ---- 比较断言 ------------------------------------------------------------- */
void UnityAssertGreaterThan(int32_t threshold, int32_t actual, int line, const char *msg)
{
    if (actual <= threshold) {
        print_line(line);
        printf("FAIL: Expected > %d, was %d", (int)threshold, (int)actual);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

void UnityAssertLessThan(int32_t threshold, int32_t actual, int line, const char *msg)
{
    if (actual >= threshold) {
        print_line(line);
        printf("FAIL: Expected < %d, was %d", (int)threshold, (int)actual);
        if (msg) printf(" [%s]", msg);
        printf("\n");
        TestFailures++;
    }
}

/* ---- 辅助 ---------------------------------------------------------------- */
void UnityIgnore(int line, const char *msg)
{
    print_line(line);
    printf("IGNORED");
    if (msg) printf(": %s", msg);
    printf("\n");
    TestIgnored++;
}

void UnityMessage(const char *msg, int line)
{
    printf("  [L%d] INFO: %s\n", line, msg);
}
