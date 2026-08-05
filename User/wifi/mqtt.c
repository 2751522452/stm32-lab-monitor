/**
 * @brief  MQTT 3.1.1 报文构建 — 零依赖
 *
 * 参考: MQTT v3.1.1 规范
 *   CONNECT: 固定头 + 可变头(协议名/等级/标志/保活) + 负载(ClientID)
 *   PUBLISH: 固定头 + 可变头(主题) + 负载(任意数据)
 *   PINGREQ: 固定头 2 字节
 */

#include "mqtt.h"
#include <string.h>

/* ---- 内部: 剩余长度编码 ----------------------------------------------------- */
/*
 * MQTT 剩余长度: 1~4 字节可变长编码
 * 每字节低 7 位为数据，最高位=1 表示后续还有字节
 */
static int encode_remaining_length(uint8_t *buf, uint32_t len)
{
    int i = 0;
    do {
        uint8_t byte = (uint8_t)(len % 128);
        len /= 128;
        if (len > 0) byte |= 0x80;
        buf[i++] = byte;
    } while (len > 0 && i < 4);
    return i;
}

/* ---- 写入大端序 16 位 ------------------------------------------------------- */
static void write_u16(uint8_t *buf, uint16_t val)
{
    buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val);
}

/* ---- 写入字符串 (长度前缀 + UTF-8 数据) ------------------------------------- */
static int write_string(uint8_t *buf, const char *str)
{
    uint16_t len = (uint16_t)strlen(str);
    write_u16(buf, len);
    memcpy(buf + 2, str, len);
    return 2 + len;
}

/* ===================================================================
 *  mqtt_build_connect
 *
 *  报文结构:
 *    [0x10] [RemLen] [ProtoName] [Level] [Flags] [KeepAlive] [ClientID]
 * =================================================================== */
int mqtt_build_connect(uint8_t *buf, const char *client_id, uint16_t keepalive)
{
    int pos = 0;

    /* 可变头 + 负载 (先算长度) */
    uint8_t varbuf[128];
    int vp = 0;

    /* 协议名 "MQTT" */
    vp += write_string(varbuf + vp, "MQTT");

    /* 协议等级 4 = MQTT 3.1.1 */
    varbuf[vp++] = 0x04;

    /* 连接标志: CleanSession=1, 其余=0 */
    varbuf[vp++] = 0x02;

    /* 保活时间 */
    write_u16(varbuf + vp, keepalive);
    vp += 2;

    /* Client ID */
    vp += write_string(varbuf + vp, client_id);

    /* 固定头 */
    buf[pos++] = MQTT_CONNECT;

    /* 剩余长度 = 可变头 + 负载 */
    pos += encode_remaining_length(buf + pos, (uint32_t)vp);

    /* 拷贝可变头+负载 */
    memcpy(buf + pos, varbuf, vp);
    pos += vp;

    return pos;
}

/* ===================================================================
 *  mqtt_build_publish  (QoS 0, 不保留)
 *
 *  报文结构:
 *    [0x30] [RemLen] [TopicLen] [Topic] [Payload]
 * =================================================================== */
int mqtt_build_publish(uint8_t *buf, const char *topic,
                       const uint8_t *payload, uint16_t payload_len)
{
    int pos = 0;
    uint16_t topic_len = (uint16_t)strlen(topic);

    /* 剩余长度 = 主题长度前缀(2) + 主题 + 负载 */
    uint32_t rem_len = 2 + (uint32_t)topic_len + (uint32_t)payload_len;

    /* 固定头 */
    buf[pos++] = MQTT_PUBLISH;   /* QoS 0, DUP=0, Retain=0 */

    pos += encode_remaining_length(buf + pos, rem_len);

    /* 主题 */
    write_u16(buf + pos, topic_len);
    pos += 2;
    memcpy(buf + pos, topic, topic_len);
    pos += topic_len;

    /* 负载 */
    if (payload && payload_len > 0) {
        memcpy(buf + pos, payload, payload_len);
        pos += payload_len;
    }

    return pos;
}

/* ===================================================================
 *  mqtt_build_pingreq
 *
 *  报文结构: [0xC0] [0x00]  固定 2 字节
 * =================================================================== */
int mqtt_build_pingreq(uint8_t *buf)
{
    buf[0] = MQTT_PINGREQ;
    buf[1] = 0x00;
    return 2;
}
