/**
 * @brief  Unity Test Framework — 轻量级嵌入式 C 单元测试框架
 *
 * 仅 ~500 行，无需任何外部依赖。原生支持:
 *   - 基本断言 (INT/HEX/FLOAT/STRING/MEMORY)
 *   - 测试分组 (RUN_TEST)
 *   - 失败计数 + 摘要输出
 *
 * 用法:
 *   #include "unity.h"
 *   void setUp(void)   { /* 每个测试前调用 */ }
 *   void tearDown(void) { /* 每个测试后调用 */ }
 *
 *   void test_my_feature(void) {
 *       TEST_ASSERT_EQUAL(42, my_func());
 *   }
 *
 *   int main(void) {
 *       UNITY_BEGIN();
 *       RUN_TEST(test_my_feature);
 *       return UNITY_END();
 *   }
 */

#ifndef UNITY_H
#define UNITY_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 配置 --------------------------------------------------------------- */
#ifndef UNITY_OUTPUT_CHAR
#define UNITY_OUTPUT_CHAR(a) putchar(a)
#endif

#ifndef UNITY_INT_WIDTH
#define UNITY_INT_WIDTH 32
#endif

/* ---- 测试计数 ------------------------------------------------------------- */
extern int UnityFailures;
extern int UnityTests;

/* ---- 框架入口 ------------------------------------------------------------- */
#define UNITY_BEGIN()   UnityBegin(__FILE__)
#define UNITY_END()     UnityEnd()

void UnityBegin(const char *filename);
int  UnityEnd(void);

/* ---- 测试注册 ------------------------------------------------------------- */
#define RUN_TEST(test)  UnityConcludeTest(); \
                        if (!UnityFailures) { \
                            UnityTests++; \
                            setUp(); \
                            test(); \
                        } else { \
                            /* skip remaining after first group failure */ \
                        }

/* ---- 用户定义的 setup/teardown (弱引用) ------------------------------------ */
void setUp(void);
void tearDown(void);

/* ---- 断言 (自动展开 line 号) ---------------------------------------------- */

#define TEST_ASSERT(condition) \
    UnityAssert((condition), __LINE__, "Expected TRUE")

#define TEST_ASSERT_TRUE(condition) \
    UnityAssert((condition), __LINE__, "Expected TRUE")

#define TEST_ASSERT_FALSE(condition) \
    UnityAssert(!(condition), __LINE__, "Expected FALSE")

#define TEST_ASSERT_EQUAL(expected, actual) \
    UnityAssertEqualInt((int32_t)(expected), (int32_t)(actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    UnityAssertEqualInt((int32_t)(expected), (int32_t)(actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_INT32(expected, actual) \
    UnityAssertEqualInt32((int32_t)(expected), (int32_t)(actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_UINT32(expected, actual) \
    UnityAssertEqualUint32((uint32_t)(expected), (uint32_t)(actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_UINT16(expected, actual) \
    UnityAssertEqualUint16((uint16_t)(expected), (uint16_t)(actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_UINT8(expected, actual) \
    UnityAssertEqualUint8((uint8_t)(expected), (uint8_t)(actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_HEX(expected, actual) \
    UnityAssertEqualHex((uint32_t)(expected), (uint32_t)(actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_HEX32(expected, actual) \
    UnityAssertEqualHex32((uint32_t)(expected), (uint32_t)(actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual) \
    UnityAssertEqualFloat((expected), (actual), 0.0001f, __LINE__, NULL)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) \
    UnityAssertEqualFloat((expected), (actual), (delta), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    UnityAssertEqualString((expected), (actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) \
    UnityAssertEqualMemory((expected), (actual), (len), __LINE__, NULL)

#define TEST_ASSERT_NULL(ptr) \
    UnityAssertNull((ptr), __LINE__, "Expected NULL")

#define TEST_ASSERT_NOT_NULL(ptr) \
    UnityAssertNotNull((ptr), __LINE__, "Expected Not NULL")

#define TEST_ASSERT_BIT_HIGH(mask, reg) \
    UnityAssertBitHigh((mask), (reg), __LINE__, NULL)

#define TEST_ASSERT_BIT_LOW(mask, reg) \
    UnityAssertBitLow((mask), (reg), __LINE__, NULL)

#define TEST_ASSERT_GREATER_THAN(threshold, actual) \
    UnityAssertGreaterThan((threshold), (actual), __LINE__, NULL)

#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    UnityAssertLessThan((threshold), (actual), __LINE__, NULL)

#define TEST_IGNORE() \
    UnityIgnore(__LINE__, NULL)

#define TEST_MESSAGE(msg) \
    UnityMessage((msg), __LINE__)

/* ---- 底层断言函数 --------------------------------------------------------- */
void UnityConcludeTest(void);

void UnityAssert(int condition, int line, const char *msg);
void UnityAssertEqualInt(int32_t expected, int32_t actual, int line, const char *msg);
void UnityAssertEqualInt32(int32_t expected, int32_t actual, int line, const char *msg);
void UnityAssertEqualUint32(uint32_t expected, uint32_t actual, int line, const char *msg);
void UnityAssertEqualUint16(uint16_t expected, uint16_t actual, int line, const char *msg);
void UnityAssertEqualUint8(uint8_t expected, uint8_t actual, int line, const char *msg);
void UnityAssertEqualHex(uint32_t expected, uint32_t actual, int line, const char *msg);
void UnityAssertEqualHex32(uint32_t expected, uint32_t actual, int line, const char *msg);
void UnityAssertEqualFloat(float expected, float actual, float delta, int line, const char *msg);
void UnityAssertEqualString(const char *expected, const char *actual, int line, const char *msg);
void UnityAssertEqualMemory(const void *expected, const void *actual, uint32_t len, int line, const char *msg);
void UnityAssertNull(const void *ptr, int line, const char *msg);
void UnityAssertNotNull(const void *ptr, int line, const char *msg);
void UnityAssertBitHigh(uint32_t mask, uint32_t reg, int line, const char *msg);
void UnityAssertBitLow(uint32_t mask, uint32_t reg, int line, const char *msg);
void UnityAssertGreaterThan(int32_t threshold, int32_t actual, int line, const char *msg);
void UnityAssertLessThan(int32_t threshold, int32_t actual, int line, const char *msg);
void UnityIgnore(int line, const char *msg);
void UnityMessage(const char *msg, int line);

#ifdef __cplusplus
}
#endif

#endif /* UNITY_H */
