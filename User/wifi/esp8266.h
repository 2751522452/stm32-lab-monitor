/**
 * @brief  ESP8266 AT 指令状态机 (非阻塞, 命令模式 + CIPSEND)
 *
 * 状态流程:
 *   RESET → INIT → SET_MODE → CONNECT_WIFI → CONNECT_SERVER → READY
 *
 * READY 状态下:
 *   发送数据: AT+CIPSEND=<len> → 收到 ">" → 发送二进制帧 → 收到 "SEND OK"
 *   心跳:    每 30s 发 AT\r\n, 检查 OK, 超时则重连
 */
#ifndef __ESP8266_H__
#define __ESP8266_H__

#include <stdint.h>
#include "ringbuf.h"

/* ---- 状态枚举 -------------------------------------------------------------- */
typedef enum {
    ESP_STATE_RESET = 0,
    ESP_STATE_INIT,
    ESP_STATE_SET_MODE,
    ESP_STATE_CONNECT_WIFI,
    ESP_STATE_CONNECT_SERVER,
    ESP_STATE_READY,
    ESP_STATE_SENDING,          /* 正在 CIPSEND 事务中 */
    ESP_STATE_ERROR,            /* 出错, 等待重试 */
} ESP8266_State;

/* ---- ESP8266 句柄 ---------------------------------------------------------- */
typedef struct {
    ESP8266_State   state;
    RingBuf         rx_buf;             /* UART ISR → 此缓冲 */

    /* 超时与重试 */
    uint32_t        cmd_tick;           /* 当前命令起始 tick */
    uint32_t        cmd_timeout;        /* 当前命令超时 (ms) */
    uint32_t        last_heartbeat;     /* 上次心跳 tick */
    int             retry_count;
    int             retry_max;

    /* 配置 */
    const char     *ssid;
    const char     *password;
    const char     *server_ip;
    uint16_t        server_port;

    /* 发送状态 */
    const uint8_t  *send_data;          /* 待发送数据指针 */
    uint16_t        send_len;           /* 待发送长度 */
    int             sending;            /* 1=正在发送二进制数据 */
    uint8_t         send_step;          /* CIPSEND 子步骤 0/1/2 */

    /* 响应解析缓冲 */
    char            line_buf[128];      /* 行缓冲区 */
    uint16_t        line_pos;

    /* 连接状态 */
    uint8_t         wifi_connected : 1;
    uint8_t         tcp_connected  : 1;
} ESP8266;

/* ---- 全局实例 -------------------------------------------------------------- */
extern ESP8266 g_esp;

/* ---- API ------------------------------------------------------------------ */
void ESP8266_Init   (ESP8266 *esp, const char *ssid, const char *pwd,
                     const char *ip, uint16_t port);
void ESP8266_Process(ESP8266 *esp);         /* 每轮 WiFiTask 调用 */
int  ESP8266_Send   (ESP8266 *esp, const uint8_t *data, uint16_t len);
int  ESP8266_SendRaw (ESP8266 *esp, const uint8_t *data, uint16_t len);
void ESP8266_RX_IRQ (uint8_t byte);         /* USART3 中断调用 */

/* ===================================================================
 *  二进制帧协议
 *
 *  帧格式: [0xAA 0x55] [TYPE 1B] [LEN 2B BE] [PAYLOAD N] [CRC8 1B]
 *  CRC8(poly=0x07) 覆盖 TYPE + LEN + PAYLOAD
 * =================================================================== */

#define FRAME_SYNC0          0xAA
#define FRAME_SYNC1          0x55
#define FRAME_TYPE_SENSOR    0x01

#define FRAME_HEADER_SIZE    5
#define FRAME_CRC_SIZE       1
#define MAX_FRAME_SIZE       256

/* 传感器数据负载 (通过 WiFi 发送) */
#pragma pack(1)
typedef struct {
    uint32_t timestamp;
    uint16_t ps_raw;
    uint16_t mq135_raw;
    uint16_t mq2_raw;
    float    temperature;
    float    humidity;
    uint8_t  dht_valid;
} SensorPayload;
#pragma pack()

#define SENSOR_PAYLOAD_SIZE  sizeof(SensorPayload)

#endif /* __ESP8266_H__ */
