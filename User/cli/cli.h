/**
 * @brief  UART 调试命令行 — 非阻塞字符接收 + 命令解析
 *
 * 使用 USART1 (PA9/PA10) 作为调试控制台
 * 通过 UART RX 中断逐字节接收, 环形缓冲暂存, 换行触发解析
 *
 * 支持命令:
 *   stats    — 性能统计总览 (CPU/运行时长/任务栈)
 *   perf     — MQTT 延迟细节
 *   storage  — Flash 存储统计
 *   tasks    — 任务栈水位 + 状态
 *   help     — 命令列表
 *   reset    — 重置性能计数器
 */

#ifndef __CLI_H__
#define __CLI_H__

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 配置 ---- */
#define CLI_RX_BUF_SIZE   64        /* 命令行缓冲区 */
#define CLI_MAX_ARGS      4         /* 最大参数数 */
#define CLI_PROMPT        "ETMS> "  /* 命令提示符 */

/* ---- API ---- */

/**
 * @brief  CLI 初始化 — 打印欢迎信息 + 提示符
 */
void CLI_Init(void);

/**
 * @brief  CLI 处理循环 — 非阻塞, 每轮主循环或独立任务调用
 *
 * 读取环形缓冲中的字符, 检测 '\r' 或 '\n' 触发解析
 */
void CLI_Process(void);

/**
 * @brief  UART RX 中断回调 — 将收到字节写入 CLI 缓冲区
 * @param  ch 接收到的单字节
 */
void CLI_RX_IRQ(uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_H__ */
