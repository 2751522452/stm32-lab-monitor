/**
 * @brief  MQTT 协议报文构建单元测试
 *
 * 被测源文件: ../User/wifi/mqtt.c
 * 覆盖: CONNECT / PUBLISH / PINGREQ 报文结构 | 剩余长度编码
 */

#include "unity.h"
#include <stdint.h>
#include <string.h>

/* ---- 被测函数 ---- */
extern int mqtt_build_connect(uint8_t *buf, const char *client_id, uint16_t keepalive);
extern int mqtt_build_publish(uint8_t *buf, const char *topic,
                              const uint8_t *payload, uint16_t payload_len);
extern int mqtt_build_pingreq(uint8_t *buf);

/* ---- 报文类型 ---- */
#define MQTT_CONNECT  0x10
#define MQTT_PUBLISH  0x30
#define MQTT_PINGREQ  0xC0

/* ================================================================ */
void setUp(void) {}
void tearDown(void) {}

/* ================================================================
 *  PINGREQ 测试
 * ================================================================ */
void test_pingreq_length(void)
{
    uint8_t buf[4];
    int len = mqtt_build_pingreq(buf);
    TEST_ASSERT_EQUAL_INT(2, len);
}

void test_pingreq_content(void)
{
    uint8_t buf[4];
    mqtt_build_pingreq(buf);
    TEST_ASSERT_EQUAL_HEX(MQTT_PINGREQ, buf[0]);
    TEST_ASSERT_EQUAL_HEX(0x00, buf[1]);
}

/* ================================================================
 *  CONNECT 测试
 * ================================================================ */

void test_connect_protocol_name(void)
{
    uint8_t buf[128];
    int len = mqtt_build_connect(buf, "test", 60);
    TEST_ASSERT(len > 10);  /* 至少包含协议名 */

    /* 固定头 */
    TEST_ASSERT_EQUAL_HEX(MQTT_CONNECT, buf[0]);
    /* 协议名 "MQTT": 长度 0x0004 */
    uint16_t proto_len = ((uint16_t)buf[2] << 8) | buf[3];
    TEST_ASSERT_EQUAL_UINT16(4, proto_len);
}

void test_connect_keepalive(void)
{
    uint8_t buf[128];
    /* Keepalive 位置:
       固定头(1+RL) + ProtoName(2+4) + Level(1) + Flags(1) + KeepAlive(2)
       最小 RL = 4+4+1+1+2 + ClientID(2+N) */
    int len = mqtt_build_connect(buf, "x", 120);

    /* 找 keepalive 字段 — 它是可变头中 flags(1byte) 后的 2 字节 */
    int pos = 1;
    /* 剩余长度 */
    while (buf[pos] & 0x80) pos++;
    pos++;
    /* ProtoName 长度+内容 */
    pos += 2 + 4;
    /* Level */
    pos += 1;
    TEST_ASSERT_EQUAL_HEX(0x04, buf[pos - 1]);  /* MQTT 3.1.1 */
    /* Flags */
    pos += 1;
    TEST_ASSERT_EQUAL_HEX(0x02, buf[pos - 1]);  /* CleanSession */

    /* KeepAlive */
    uint16_t ka = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
    TEST_ASSERT_EQUAL_UINT16(120, ka);
}

void test_connect_client_id(void)
{
    uint8_t buf[128];
    int len = mqtt_build_connect(buf, "stm32-etms", 60);

    /* 找 Client ID: 在 keepalive 后面 */
    int pos = 1;
    while (buf[pos] & 0x80) pos++;
    pos++;  /* RemLen 结束 */
    pos += 2 + 4;  /* ProtoName */
    pos += 1;      /* Level */
    pos += 1;      /* Flags */
    pos += 2;      /* KeepAlive */

    /* Client ID 长度 + 内容 */
    uint16_t cid_len = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
    TEST_ASSERT_EQUAL_UINT16(10, cid_len);
    TEST_ASSERT_EQUAL_MEMORY("stm32-etms", &buf[pos + 2], 10);
}

/* ================================================================
 *  PUBLISH 测试
 * ================================================================ */

void test_publish_qos0(void)
{
    uint8_t buf[256];
    const char *payload = "hello";
    int len = mqtt_build_publish(buf, "test/topic",
                                 (const uint8_t *)payload, 5);
    TEST_ASSERT_EQUAL_HEX(MQTT_PUBLISH, buf[0]);  /* QoS0, DUP=0, Retain=0 */
    TEST_ASSERT(len > 0);
}

void test_publish_topic(void)
{
    uint8_t buf[256];
    mqtt_build_publish(buf, "etms/sensor",
                       (const uint8_t *)"data", 4);

    /* 跳过固定头 → topic len */
    int pos = 1;
    while (buf[pos] & 0x80) pos++;
    pos++;
    uint16_t topic_len = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
    TEST_ASSERT_EQUAL_UINT16(11, topic_len);
    TEST_ASSERT_EQUAL_MEMORY("etms/sensor", &buf[pos + 2], 11);
}

void test_publish_payload(void)
{
    uint8_t buf[256];
    const char *json = "{\"ts\":123,\"ps\":2.5}";
    uint16_t plen = (uint16_t)strlen(json);
    int len = mqtt_build_publish(buf, "t",
                                 (const uint8_t *)json, plen);

    int pos = 1;
    while (buf[pos] & 0x80) pos++;
    pos++;
    pos += 2 + 1;  /* topic */

    TEST_ASSERT_EQUAL_MEMORY(json, &buf[pos], plen);
}

void test_publish_empty_payload(void)
{
    uint8_t buf[256];
    int len = mqtt_build_publish(buf, "t", NULL, 0);
    TEST_ASSERT(len > 4);  /* 固定头 + 主题 */
}

/* ================================================================
 *  Test Runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pingreq_length);
    RUN_TEST(test_pingreq_content);
    RUN_TEST(test_connect_protocol_name);
    RUN_TEST(test_connect_keepalive);
    RUN_TEST(test_connect_client_id);
    RUN_TEST(test_publish_qos0);
    RUN_TEST(test_publish_topic);
    RUN_TEST(test_publish_payload);
    RUN_TEST(test_publish_empty_payload);
    return UNITY_END();
}
