/**
 * @brief  UART 调试命令行实现
 *
 * 命令解析 + 各命令执行函数
 * 输出均通过 USART1 printf 重定向
 */

#include "cli/cli.h"
#include "perf/perf.h"
#include "storage/adc_storage.h"
#include <stdio.h>
#include <string.h>

/* FreeRTOS 头文件 */
#include "FreeRTOS.h"
#include "task.h"

/* ================================================================
 *  CLI 内部状态
 * ================================================================ */

static struct {
    char     buf[CLI_RX_BUF_SIZE];
    uint8_t  pos;
    uint8_t  ready;           /* 1=收到完整行, 待处理 */
} cli_rx;

/* ---- 外部任务句柄 (从 main.c 引用) ---- */
extern TaskHandle_t hSensor, hAlarm, hMQ, hDisplay, hFlash, hWiFi, hWatchdog;

/* ---- 命令处理函数前向声明 ---- */
static void cmd_help(void);
static void cmd_stats(void);
static void cmd_perf(void);
static void cmd_storage(void);
static void cmd_tasks(void);
static void cmd_reset(void);

/* ================================================================
 *  CLI_Init
 * ================================================================ */
void CLI_Init(void)
{
    memset(&cli_rx, 0, sizeof(cli_rx));

    /* 启用 USART1 RXNE 中断 (NVIC 已在 usart.c 中配置) */
    USART1->CR1 |= USART_CR1_RXNEIE;

    printf("\r\n\r\n");
    printf("========================================\r\n");
    printf("  ETMS Debug Console v1.0\r\n");
    printf("  STM32F103C8T6 + FreeRTOS\r\n");
    printf("  Type 'help' for commands\r\n");
    printf("========================================\r\n");
    printf(CLI_PROMPT);
}

/* ================================================================
 *  CLI_RX_IRQ — 中断回调 (UART RX ISR 调用)
 * ================================================================ */
void CLI_RX_IRQ(uint8_t ch)
{
    /* 回显 */
    printf("%c", ch);

    if (ch == '\r' || ch == '\n') {
        if (cli_rx.pos > 0) {
            cli_rx.buf[cli_rx.pos] = '\0';
            cli_rx.ready = 1;
        }
        printf("\r\n");
        return;
    }

    /* 退格处理 */
    if (ch == 0x08 || ch == 0x7F) {
        if (cli_rx.pos > 0) {
            cli_rx.pos--;
            printf("\b \b");  /* 回退一格 */
        }
        return;
    }

    /* 缓冲未满则存入 */
    if (cli_rx.pos < CLI_RX_BUF_SIZE - 1) {
        cli_rx.buf[cli_rx.pos++] = ch;
    }
}

/* ================================================================
 *  CLI_Process — 主循环 (非阻塞)
 * ================================================================ */
void CLI_Process(void)
{
    if (!cli_rx.ready)
        return;

    cli_rx.ready = 0;
    cli_rx.pos   = 0;

    char *cmd = cli_rx.buf;

    /* 跳过前导空白 */
    while (*cmd == ' ' || *cmd == '\t') cmd++;

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    }
    else if (strcmp(cmd, "stats") == 0) {
        cmd_stats();
    }
    else if (strcmp(cmd, "perf") == 0) {
        cmd_perf();
    }
    else if (strcmp(cmd, "storage") == 0) {
        cmd_storage();
    }
    else if (strcmp(cmd, "tasks") == 0) {
        cmd_tasks();
    }
    else if (strcmp(cmd, "reset") == 0) {
        cmd_reset();
    }
    else if (strlen(cmd) == 0) {
        /* 空行, 不报错 */
    }
    else {
        printf("Unknown: '%s'. Type 'help'\r\n", cmd);
    }

    printf(CLI_PROMPT);
}

/* ================================================================
 *  help — 命令列表
 * ================================================================ */
static void cmd_help(void)
{
    printf("\r\nCommands:\r\n");
    printf("  stats    - CPU load, uptime, stack watermarks\r\n");
    printf("  perf     - MQTT latency, reconnect stats\r\n");
    printf("  storage  - Flash record count, sector usage\r\n");
    printf("  tasks    - Per-task stack high water mark\r\n");
    printf("  reset    - Reset performance counters\r\n");
    printf("  help     - This list\r\n");
}

/* ================================================================
 *  stats — 总体性能统计
 * ================================================================ */
static void cmd_stats(void)
{
    PerfStats s;
    Perf_GetStats(&s);

    uint32_t days  = s.uptime_sec / 86400;
    uint32_t hours = (s.uptime_sec % 86400) / 3600;
    uint32_t mins  = (s.uptime_sec % 3600) / 60;
    uint32_t secs  = s.uptime_sec % 60;

    printf("\r\n===== System Stats =====\r\n");
    printf("Uptime:      %lud %02lu:%02lu:%02lu\r\n",
           (unsigned long)days, (unsigned long)hours,
           (unsigned long)mins,  (unsigned long)secs);
    printf("CPU Load:    %.1f%%  (idle delta=%lu, max=%lu)\r\n",
           s.cpu_load_pct,
           (unsigned long)s.idle_count,
           (unsigned long)s.idle_count_max);
    printf("Task Switch: ~%u us  (Cortex-M3 @72MHz)\r\n",
           (unsigned)s.task_switch_us);

    /* 各任务栈高水位 */
    printf("\r\n--- Task Stack (bytes free) ---\r\n");
    printf("Sensor:   %u\r\n",
           (unsigned)uxTaskGetStackHighWaterMark(hSensor));
    printf("Alarm:    %u\r\n",
           (unsigned)uxTaskGetStackHighWaterMark(hAlarm));
    printf("MQ:       %u\r\n",
           (unsigned)uxTaskGetStackHighWaterMark(hMQ));
    printf("Display:  %u\r\n",
           (unsigned)uxTaskGetStackHighWaterMark(hDisplay));
    printf("Flash:    %u\r\n",
           (unsigned)uxTaskGetStackHighWaterMark(hFlash));
    printf("WiFi:     %u\r\n",
           (unsigned)uxTaskGetStackHighWaterMark(hWiFi));
    printf("Watchdog: %u\r\n",
           (unsigned)uxTaskGetStackHighWaterMark(hWatchdog));
}

/* ================================================================
 *  perf — MQTT 通信性能
 * ================================================================ */
static void cmd_perf(void)
{
    PerfStats s;
    Perf_GetStats(&s);

    printf("\r\n===== MQTT Performance =====\r\n");
    printf("Publish count:   %lu\r\n", (unsigned long)s.mqtt_pub_cnt);
    printf("Last latency:    %lu ms\r\n", (unsigned long)s.mqtt_pub_lat_ms);
    printf("Min latency:     %lu ms\r\n",
           s.mqtt_pub_lat_min == 0xFFFFFFFF ? 0UL :
           (unsigned long)s.mqtt_pub_lat_min);
    printf("Max latency:     %lu ms\r\n", (unsigned long)s.mqtt_pub_lat_max);
    printf("Avg latency:     %lu ms\r\n", (unsigned long)s.mqtt_pub_lat_avg);
    printf("---\r\n");
    printf("Reconnect count: %lu\r\n", (unsigned long)s.reconnect_cnt);
    printf("Reconnect OK:    %lu\r\n", (unsigned long)s.reconnect_success);

    if (s.reconnect_cnt > 0) {
        float rate = 100.0f * (float)s.reconnect_success / (float)s.reconnect_cnt;
        printf("Success rate:    %.1f%%\r\n", rate);
    }
}

/* ================================================================
 *  storage — Flash 存储统计
 * ================================================================ */
static void cmd_storage(void)
{
    uint32_t records = ADC_Get_Record_Count();
    uint32_t addr    = ADC_Get_Write_Address();

    printf("\r\n===== Flash Storage =====\r\n");

    /* 扇区映射 */
    uint32_t sector_addr = (addr / 4096) * 4096;
    uint32_t offset      = addr - sector_addr;

    printf("W25Q64:       8 MB (8192 KB)\r\n");
    printf("Active:        2 x 4KB sectors (dual rotation)\r\n");
    printf("Record size:   12 bytes (+CRC8)\r\n");
    printf("Records/sector: ~340\r\n");
    printf("---\r\n");
    printf("Total records: %lu\r\n", (unsigned long)records);
    printf("Write addr:    0x%06lX  (sector=0x%06lX, off=%lu)\r\n",
           (unsigned long)addr, (unsigned long)sector_addr,
           (unsigned long)offset);
    printf("Sector usage:  %.1f%%\r\n",
           100.0f * (float)(offset) / 4096.0f);

    /* 估算存储时长 */
    float hours_stored = (float)records * 1.5f / 3600.0f;
    printf("Data span:     ~%.1f hours  (@1 record/1.5s)\r\n", hours_stored);
}

/* ================================================================
 *  tasks — 详细任务列表
 * ================================================================ */
static void cmd_tasks(void)
{
    /* 获取系统 tick 计数 */
    TickType_t tick = xTaskGetTickCount();

    printf("\r\n===== Task Details =====\r\n");
    printf("Tick count: %lu  (%lu s)\r\n",
           (unsigned long)tick,
           (unsigned long)(tick / configTICK_RATE_HZ));

    printf("\r\n%-10s %4s %6s %8s\r\n",
           "Name", "Prio", "Stack", "Free");
    printf("---------- ---- ------ --------\r\n");

    printf("%-10s %4d %6u %8u\r\n",
           "Sensor",   3, 200,
           (unsigned)uxTaskGetStackHighWaterMark(hSensor));
    printf("%-10s %4d %6u %8u\r\n",
           "Alarm",    5, 256,
           (unsigned)uxTaskGetStackHighWaterMark(hAlarm));
    printf("%-10s %4d %6u %8u\r\n",
           "MQ",       4, 256,
           (unsigned)uxTaskGetStackHighWaterMark(hMQ));
    printf("%-10s %4d %6u %8u\r\n",
           "WiFi",     3, 512,
           (unsigned)uxTaskGetStackHighWaterMark(hWiFi));
    printf("%-10s %4d %6u %8u\r\n",
           "Flash",    2, 256,
           (unsigned)uxTaskGetStackHighWaterMark(hFlash));
    printf("%-10s %4d %6u %8u\r\n",
           "Watchdog", 2, 128,
           (unsigned)uxTaskGetStackHighWaterMark(hWatchdog));
    printf("%-10s %4d %6u %8u\r\n",
           "Display",  1, 512,
           (unsigned)uxTaskGetStackHighWaterMark(hDisplay));

    /* 堆使用情况 */
    printf("\r\nHeap: %lu / %lu bytes free\r\n",
           (unsigned long)xPortGetFreeHeapSize(),
           (unsigned long)configTOTAL_HEAP_SIZE);
}

/* ================================================================
 *  reset — 重置性能计数器
 * ================================================================ */
static void cmd_reset(void)
{
    Perf_Init();
    printf("Performance counters reset.\r\n");
}
