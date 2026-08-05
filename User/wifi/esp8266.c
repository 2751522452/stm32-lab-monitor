/**
 * @brief  ESP8266 AT 指令状态机 — 非阻塞实现
 *
 * 状态流程:
 *   RESET → INIT → SET_MODE → CONNECT_WIFI → CONNECT_SERVER → READY
 */

#include "esp8266.h"
#include "usart/usart.h"
#include <string.h>

/* ---- 外部引用 -------------------------------------------------------------- */
extern UART_HandleTypeDef huart3;

/* ---- 全局实例 -------------------------------------------------------------- */
ESP8266 g_esp;

/* ---- 调试输出到 USART1 (裸寄存器, 不拉 printf) ----------------------------- */
static void dbg(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            while ((USART1->SR & (1U << 7)) == 0);
            USART1->DR = '\r';
        }
        while ((USART1->SR & (1U << 7)) == 0);
        USART1->DR = (uint32_t)(uint8_t)(*s++);
    }
}

/* ---- 单字节接收缓冲 --------------------------------------------------------- */
static uint8_t esp_rx_byte;

/* ===================================================================
 *  CRC-8 查找表 (poly=0x07, x^8+x^2+x+1)
 * =================================================================== */
static const uint8_t crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

static uint8_t crc8_calc(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++)
        crc = crc8_table[crc ^ data[i]];
    return crc;
}

static void u16_to_str(uint16_t val, char *buf)
{
    char tmp[6];
    int i = 0, j;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (val > 0) { tmp[i++] = (char)('0' + (val % 10)); val /= 10; }
    for (j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
}

/* ===================================================================
 *  二进制帧协议: [0xAA 0x55] [TYPE 1B] [LEN 2B BE] [PAYLOAD N] [CRC8 1B]
 * =================================================================== */
#define FRAME_SYNC0       0xAA
#define FRAME_SYNC1       0x55
#define FRAME_TYPE_SENSOR 0x01
#define FRAME_HEADER_SIZE 5
#define FRAME_CRC_SIZE    1
#define MAX_FRAME_SIZE    256

static uint16_t frame_build_sensor(uint8_t *frame_buf,
                                   const void *payload, uint16_t payload_len)
{
    uint16_t len_be;
    frame_buf[0] = FRAME_SYNC0;
    frame_buf[1] = FRAME_SYNC1;
    frame_buf[2] = FRAME_TYPE_SENSOR;
    len_be = (uint16_t)((payload_len >> 8) | (payload_len << 8));
    frame_buf[3] = (uint8_t)(len_be >> 8);
    frame_buf[4] = (uint8_t)(len_be);
    memcpy(&frame_buf[FRAME_HEADER_SIZE], payload, payload_len);
    frame_buf[FRAME_HEADER_SIZE + payload_len] =
        crc8_calc(&frame_buf[2], (int)(FRAME_HEADER_SIZE - 2 + payload_len));
    return (uint16_t)(FRAME_HEADER_SIZE + payload_len + FRAME_CRC_SIZE);
}

/* ===================================================================
 *  RingBuf 辅助
 * =================================================================== */
/* 从环形缓冲中取一行到 line_buf, 返回 1=取到完整行 */
static int ringbuf_readline(RingBuf *rb, char *line, uint16_t max_len)
{
    uint16_t avail = ringbuf_avail(rb);
    uint16_t i;
    uint8_t ch;
    if (avail == 0) return 0;
    for (i = 0; i < avail && i < max_len - 1; i++) {
        ringbuf_peek(rb, i, &ch);
        line[i] = (char)ch;
        if (ch == '\n') {
            ringbuf_discard(rb, i + 1);
            line[i + 1] = '\0';
            return 1;
        }
    }
    return 0;
}

static int ringbuf_find_and_consume(RingBuf *rb, const char *pattern)
{
    uint16_t pos;
    if (ringbuf_strstr(rb, pattern, &pos)) {
        ringbuf_discard(rb, (uint16_t)(pos + strlen(pattern)));
        return 1;
    }
    return 0;
}

/* ===================================================================
 *  AT 命令发送
 * =================================================================== */
static void uart3_send(const char *str)
{
    uint16_t len = (uint16_t)strlen(str);
    HAL_UART_Transmit(&huart3, (uint8_t *)str, len, 1000);
}

static void uart3_send_raw(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)data, len, 5000);
}

/* ---- 公共 API -------------------------------------------------------------- */

void ESP8266_Init(ESP8266 *esp, const char *ssid, const char *pwd,
                  const char *ip, uint16_t port)
{
    memset(esp, 0, sizeof(ESP8266));
    ringbuf_init(&esp->rx_buf);
    esp->state       = ESP_STATE_RESET;
    esp->ssid        = ssid;
    esp->password    = pwd;
    esp->server_ip   = ip;
    esp->server_port = port;
    esp->retry_max   = 3;
    esp->cmd_timeout = 5000;
    HAL_UART_Receive_IT(&huart3, &esp_rx_byte, 1);
}

void ESP8266_RX_IRQ(uint8_t byte)
{
    ringbuf_put(&g_esp.rx_buf, byte);
}

/* ===================================================================
 *  ESP8266_Process — 主状态机
 * =================================================================== */
void ESP8266_Process(ESP8266 *esp)
{
    uint32_t now = HAL_GetTick();

    /* 轮询漏掉的字节 (中断之外的兜底) */
    while (USART3->SR & USART_SR_RXNE) {
        ESP8266_RX_IRQ((uint8_t)(USART3->DR & 0xFF));
    }

#define TIMED_OUT  ((int32_t)(now - esp->cmd_tick) >= (int32_t)esp->cmd_timeout)

    switch (esp->state) {

    case ESP_STATE_RESET:
        if (esp->cmd_tick == 0) {
            ringbuf_flush(&esp->rx_buf);
            uart3_send("AT\r\n");
            esp->cmd_tick    = now;
            esp->cmd_timeout = 2000;
            dbg("[WiFi] AT probe...\n");
        } else if (ringbuf_find_and_consume(&esp->rx_buf, "OK\r\n")) {
            esp->state       = ESP_STATE_INIT;
            esp->cmd_tick    = 0;
            esp->retry_count = 0;
        } else if (TIMED_OUT) {
            ringbuf_flush(&esp->rx_buf);
            if (++esp->retry_count > esp->retry_max)
                esp->state = ESP_STATE_ERROR;
            esp->cmd_tick = 0;
        }
        break;

    case ESP_STATE_INIT:
        if (esp->cmd_tick == 0) {
            uart3_send("ATE0\r\n");
            esp->cmd_tick    = now;
            esp->cmd_timeout = 2000;
        } else if (ringbuf_find_and_consume(&esp->rx_buf, "OK\r\n")) {
            esp->state       = ESP_STATE_SET_MODE;
            esp->cmd_tick    = 0;
            esp->retry_count = 0;
            /* OK */
        } else if (TIMED_OUT) {
            ringbuf_flush(&esp->rx_buf);
            if (++esp->retry_count > esp->retry_max)
                esp->state = ESP_STATE_ERROR;
            esp->cmd_tick = 0;
        }
        break;

    case ESP_STATE_SET_MODE:
        if (esp->cmd_tick == 0) {
            uart3_send("AT+CWMODE=1\r\n");
            esp->cmd_tick    = now;
            esp->cmd_timeout = 2000;
        } else if (ringbuf_find_and_consume(&esp->rx_buf, "OK\r\n")) {
            esp->state       = ESP_STATE_CONNECT_WIFI;
            esp->cmd_tick    = 0;
            esp->retry_count = 0;
            /* step */
        } else if (TIMED_OUT) {
            ringbuf_flush(&esp->rx_buf);
            if (++esp->retry_count > esp->retry_max)
                esp->state = ESP_STATE_ERROR;
            esp->cmd_tick = 0;
        }
        break;

    case ESP_STATE_CONNECT_WIFI:
        if (esp->cmd_tick == 0) {
            char buf[128];
            strcpy(buf, "AT+CWJAP=\"");
            strcat(buf, esp->ssid);
            strcat(buf, "\",\"");
            strcat(buf, esp->password);
            strcat(buf, "\"\r\n");
            uart3_send(buf);
            esp->cmd_tick    = now;
            esp->cmd_timeout = 15000;
            /* step */
        } else if (ringbuf_find_and_consume(&esp->rx_buf, "WIFI GOT IP\r\n")) {
            esp->wifi_connected = 1;
            esp->state          = ESP_STATE_CONNECT_SERVER;
            esp->cmd_tick       = 0;
            esp->retry_count    = 0;
            dbg("[WiFi] GOT IP\n");
        } else if (ringbuf_find_and_consume(&esp->rx_buf, "FAIL\r\n")) {
            esp->wifi_connected = 0;
            esp->cmd_tick       = 0;
            /* step */
            if (++esp->retry_count > esp->retry_max)
                esp->state = ESP_STATE_ERROR;
        } else if (TIMED_OUT) {
            esp->wifi_connected = 0;
            esp->cmd_tick       = 0;
            ringbuf_flush(&esp->rx_buf);
            /* step */
            if (++esp->retry_count > esp->retry_max)
                esp->state = ESP_STATE_ERROR;
        }
        break;

    case ESP_STATE_CONNECT_SERVER:
        if (esp->cmd_tick == 0) {
            char buf[96];
            strcpy(buf, "AT+CIPSTART=\"TCP\",\"");
            strcat(buf, esp->server_ip);
            strcat(buf, "\",");
            u16_to_str(esp->server_port, buf + strlen(buf));
            strcat(buf, "\r\n");
            uart3_send(buf);
            esp->cmd_tick    = now;
            esp->cmd_timeout = 10000;
            /* step */
        } else if (ringbuf_find_and_consume(&esp->rx_buf, "CONNECT\r\n")) {
            esp->tcp_connected  = 1;
            esp->state          = ESP_STATE_READY;
            esp->cmd_tick       = 0;
            esp->retry_count    = 0;
            esp->last_heartbeat = now;
            dbg("[WiFi] TCP OK -> READY\n");
        } else if (ringbuf_find_and_consume(&esp->rx_buf, "ERROR\r\n") ||
                   ringbuf_find_and_consume(&esp->rx_buf, "CLOSED\r\n")) {
            esp->tcp_connected = 0;
            esp->cmd_tick      = 0;
            if (++esp->retry_count > esp->retry_max) {
                esp->state          = ESP_STATE_CONNECT_WIFI;
                esp->wifi_connected = 0;
            }
        } else if (TIMED_OUT) {
            esp->tcp_connected = 0;
            esp->cmd_tick      = 0;
            ringbuf_flush(&esp->rx_buf);
            if (++esp->retry_count > esp->retry_max) {
                esp->state          = ESP_STATE_CONNECT_WIFI;
                esp->wifi_connected = 0;
            }
        }
        break;

    case ESP_STATE_READY:
        /* 清除 broker 回包的 +IPD 数据, 防止撑满 ringbuf */
        {
            uint8_t ch;
            while (ringbuf_peek(&esp->rx_buf, 0, &ch) && ch == '+')
                ringbuf_readline(&esp->rx_buf, esp->line_buf, sizeof(esp->line_buf));
        }

        if (esp->send_data && esp->send_len > 0 && !esp->sending) {
            esp->state    = ESP_STATE_SENDING;
            esp->cmd_tick = 0;
            esp->sending  = 1;
            break;
        }
        if ((int32_t)(now - esp->last_heartbeat) >= 30000) {
            uart3_send("AT\r\n");
            esp->last_heartbeat = now;
            esp->cmd_tick       = now;
            esp->cmd_timeout    = 3000;
        }
        if (esp->cmd_tick != 0) {
            if (ringbuf_find_and_consume(&esp->rx_buf, "OK\r\n")) {
                esp->cmd_tick = 0;
            } else if (ringbuf_find_and_consume(&esp->rx_buf, "ERROR\r\n")) {
                esp->tcp_connected = 0;
                esp->state         = ESP_STATE_CONNECT_SERVER;
                esp->cmd_tick      = 0;
                esp->retry_count   = 0;
            } else if (TIMED_OUT) {
                esp->tcp_connected = 0;
                esp->state         = ESP_STATE_CONNECT_SERVER;
                esp->cmd_tick      = 0;
                esp->retry_count   = 0;
            }
        }
        break;

    case ESP_STATE_SENDING:
        switch (esp->send_step) {
        case 0:
            if (esp->cmd_tick == 0) {
                char buf[32];
                strcpy(buf, "AT+CIPSEND=");
                u16_to_str(esp->send_len, buf + strlen(buf));
                strcat(buf, "\r\n");
                uart3_send(buf);
                esp->cmd_tick    = now;
                esp->cmd_timeout = 5000;
            } else if (ringbuf_find_and_consume(&esp->rx_buf, "> ")) {
                esp->send_step = 1;
                esp->cmd_tick  = 0;
            } else if (ringbuf_find_and_consume(&esp->rx_buf, "ERROR\r\n") ||
                       TIMED_OUT) {
                esp->sending   = 0;
                esp->send_data = NULL;
                esp->send_len  = 0;
                esp->state     = ESP_STATE_READY;
                esp->cmd_tick  = 0;
            }
            break;
        case 1:
            uart3_send_raw(esp->send_data, esp->send_len);
            esp->send_step   = 2;
            esp->cmd_tick    = now;
            esp->cmd_timeout = 5000;
            break;
        case 2:
            if (ringbuf_find_and_consume(&esp->rx_buf, "SEND OK\r\n")) {
                esp->sending   = 0;
                esp->send_data = NULL;
                esp->send_len  = 0;
                esp->send_step = 0;
                esp->state     = ESP_STATE_READY;
                esp->cmd_tick  = 0;
                dbg("[WiFi] SEND OK\n");
            } else if (ringbuf_find_and_consume(&esp->rx_buf, "ERROR\r\n") ||
                       TIMED_OUT) {
                esp->sending   = 0;
                esp->send_data = NULL;
                esp->send_len  = 0;
                esp->send_step = 0;
                esp->state     = ESP_STATE_READY;
                esp->cmd_tick  = 0;
            }
            break;
        }
        break;

    case ESP_STATE_ERROR:
        if (esp->cmd_tick == 0) {
            esp->cmd_tick    = now;
            esp->cmd_timeout = 10000;
            dbg("[WiFi] ERROR -> wait 10s\n");
        } else if (TIMED_OUT) {
            esp->state       = ESP_STATE_RESET;
            esp->cmd_tick    = 0;
            esp->retry_count = 0;
        }
        break;
    }
#undef TIMED_OUT
}

int ESP8266_Send(ESP8266 *esp, const uint8_t *payload, uint16_t payload_len)
{
    static uint8_t frame_buf[MAX_FRAME_SIZE];
    if (esp->sending) return -1;
    uint16_t total = frame_build_sensor(frame_buf, payload, payload_len);
    if (total > MAX_FRAME_SIZE) return -2;
    esp->send_data = frame_buf;
    esp->send_len  = total;
    return 0;
}

/* ---- ESP8266_SendRaw — 直接发送原始数据 (不经 AA55 帧封装) ----------------- */
int ESP8266_SendRaw(ESP8266 *esp, const uint8_t *data, uint16_t len)
{
    if (esp->sending) return -1;
    esp->send_data = data;
    esp->send_len  = len;
    return 0;
}

/* ---- HAL UART RX 回调 (带 drain 修复漏字节) -------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        ESP8266_RX_IRQ(esp_rx_byte);
        /* drain 窗口期到达的额外字节，防止 ORE 丢字节 */
        while (USART3->SR & USART_SR_RXNE) {
            ESP8266_RX_IRQ((uint8_t)(USART3->DR & 0xFF));
        }
        HAL_UART_Receive_IT(&huart3, &esp_rx_byte, 1);
    }
}
