/* Includes ------------------------------------------------------------------*/
#include "ringbuf.h"
#include <string.h>

/* RINGBUF_SIZE 必须是 2^n, 否则改为取模 */

void ringbuf_init(RingBuf *rb)
{
    rb->head = 0;
    rb->tail = 0;
    memset(rb->buffer, 0, RINGBUF_SIZE);
}

/* ISR 上下文 — 压入 1 字节, 返回 1=成功 0=满 */
int ringbuf_put(RingBuf *rb, uint8_t byte)
{
    uint16_t next = (rb->head + 1) & (RINGBUF_SIZE - 1);

    if (next == rb->tail)
        return 0;                     /* 满 → 丢弃 (避免覆盖未读数据) */

    rb->buffer[rb->head] = byte;
    rb->head = next;
    return 1;
}

/* Task 上下文 — 取出 1 字节, 返回 1=有数据 0=空 */
int ringbuf_get(RingBuf *rb, uint8_t *byte)
{
    if (rb->head == rb->tail)
        return 0;                     /* 空 */

    *byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) & (RINGBUF_SIZE - 1);
    return 1;
}

uint16_t ringbuf_avail(RingBuf *rb)
{
    return (rb->head - rb->tail) & (RINGBUF_SIZE - 1);
}

void ringbuf_flush(RingBuf *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

/* peek offset 处的字节 (不消费), offset=0 即最早未读字节 */
int ringbuf_peek(RingBuf *rb, uint16_t offset, uint8_t *byte)
{
    if (offset >= ringbuf_avail(rb))
        return 0;

    uint16_t idx = (rb->tail + offset) & (RINGBUF_SIZE - 1);
    *byte = rb->buffer[idx];
    return 1;
}

/* 在缓冲区中搜索 pattern (朴素匹配), 返回匹配位置 (相对 tail 的偏移) */
int ringbuf_strstr(RingBuf *rb, const char *pattern, uint16_t *pos)
{
    uint16_t avail = ringbuf_avail(rb);
    uint16_t plen  = (uint16_t)strlen(pattern);

    if (plen == 0 || avail < plen)
        return 0;

    for (uint16_t i = 0; i <= avail - plen; i++) {
        uint16_t j;
        for (j = 0; j < plen; j++) {
            uint8_t b;
            ringbuf_peek(rb, i + j, &b);
            if (b != (uint8_t)pattern[j])
                break;
        }
        if (j == plen) {
            *pos = i;
            return 1;
        }
    }
    return 0;
}

/* 丢弃前 n 字节 */
void ringbuf_discard(RingBuf *rb, uint16_t n)
{
    uint16_t avail = ringbuf_avail(rb);
    if (n > avail) n = avail;
    rb->tail = (rb->tail + n) & (RINGBUF_SIZE - 1);
}
