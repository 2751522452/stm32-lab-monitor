/**
 * @brief  MQTT 3.1.1 最小协议实现 — 纯报文构建，无动态内存
 *
 * 支持: CONNECT / PUBLISH(QoS0) / SUBSCRIBE / PINGREQ
 * 不依赖任何第三方库，所有输出直接可送 ESP8266 CIPSEND
 */
#ifndef __MQTT_H__
#define __MQTT_H__

#include <stdint.h>

/* ---- 报文类型 --------------------------------------------------------------- */
#define MQTT_CONNECT    0x10
#define MQTT_CONNACK    0x20
#define MQTT_PUBLISH    0x30
#define MQTT_SUBSCRIBE  0x82
#define MQTT_SUBACK     0x90
#define MQTT_PINGREQ    0xC0
#define MQTT_PINGRESP   0xD0

/* ---- API ------------------------------------------------------------------ */

/**
 * @brief  构建 MQTT CONNECT 报文
 * @param  buf        输出缓冲区 (需 ≥128 字节)
 * @param  client_id  客户端 ID (UTF-8 字符串)
 * @param  keepalive  心跳间隔 秒
 * @return 报文总字节数
 */
int mqtt_build_connect(uint8_t *buf, const char *client_id, uint16_t keepalive);

/**
 * @brief  构建 MQTT PUBLISH 报文 (QoS 0, 不保留)
 * @param  buf        输出缓冲区 (需 ≥256 字节)
 * @param  topic      主题名
 * @param  payload    负载数据
 * @param  payload_len 负载长度
 * @return 报文总字节数
 */
int mqtt_build_publish(uint8_t *buf, const char *topic,
                       const uint8_t *payload, uint16_t payload_len);

/**
 * @brief  构建 MQTT SUBSCRIBE 报文 (QoS 0)
 * @param  buf        输出缓冲区
 * @param  pkt_id     报文标识符 (用于 SUBACK 匹配)
 * @param  topic      订阅主题
 * @param  qos        请求的 QoS 等级 (0/1/2)
 * @return 报文总字节数
 */
int mqtt_build_subscribe(uint8_t *buf, uint16_t pkt_id,
                         const char *topic, uint8_t qos);

/**
 * @brief  构建 MQTT PINGREQ 心跳报文
 * @return 固定 2 字节
 */
int mqtt_build_pingreq(uint8_t *buf);

#endif /* __MQTT_H__ */
