/**
 * @brief  中断安全环形缓冲区 (Single Producer Single Consumer)
 *
 * ISR 写入 (ringbuf_put), Task 读取 (ringbuf_get), 无需锁
 */
#ifndef __RINGBUF_H__
#define __RINGBUF_H__

#include <stdint.h>

#define RINGBUF_SIZE  512

typedef struct {
    uint8_t           buffer[RINGBUF_SIZE];
    volatile uint16_t head;       /* ISR 写指针 */
    uint16_t          tail;       /* Task 读指针 */
} RingBuf;

void     ringbuf_init   (RingBuf *rb);
int      ringbuf_put    (RingBuf *rb, uint8_t byte);   /* ISR 调用 */
int      ringbuf_get    (RingBuf *rb, uint8_t *byte);  /* Task 调用, 1=有数据 0=空 */
uint16_t ringbuf_avail  (RingBuf *rb);                  /* 可读字节数 */
void     ringbuf_flush  (RingBuf *rb);                  /* 清空 */
int      ringbuf_peek   (RingBuf *rb, uint16_t offset, uint8_t *byte);
int      ringbuf_strstr (RingBuf *rb, const char *pattern, uint16_t *pos);
void     ringbuf_discard(RingBuf *rb, uint16_t n);      /* 丢弃 n 字节 */

#endif /* __RINGBUF_H__ */
