/**
 * @brief  UART 调试命令行实现
 */

#include "cli/cli.h"
#include "perf/perf.h"
#include "storage/adc_storage.h"
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

static struct {
    char    buf[CLI_RX_BUF_SIZE];
    uint8_t pos;
    uint8_t ready;
} cli_rx;

extern TaskHandle_t hSensor, hAlarm, hDisplay, hFlash, hWiFi;

static void cmd_help(void);
static void cmd_stats(void);
static void cmd_perf(void);
static void cmd_storage(void);
static void cmd_tasks(void);
static void cmd_reset(void);

void CLI_Init(void)
{
    memset(&cli_rx, 0, sizeof(cli_rx));
    USART1->CR1 |= USART_CR1_RXNEIE;

    printf("\r\n========================================\r\n");
    printf("  ETMS Debug Console v1.0\r\n");
    printf("  STM32F103C8T6 + FreeRTOS\r\n");
    printf("  Type 'help' for commands\r\n");
    printf("========================================\r\n");
    printf(CLI_PROMPT);
}

void CLI_RX_IRQ(uint8_t ch)
{
    printf("%c", ch);

    if (ch == '\r' || ch == '\n') {
        if (cli_rx.pos > 0) {
            cli_rx.buf[cli_rx.pos] = '\0';
            cli_rx.ready = 1;
        }
        printf("\r\n");
        return;
    }

    if (ch == 0x08 || ch == 0x7F) {
        if (cli_rx.pos > 0) {
            cli_rx.pos--;
            printf("\b \b");
        }
        return;
    }

    if (cli_rx.pos < CLI_RX_BUF_SIZE - 1) {
        cli_rx.buf[cli_rx.pos++] = ch;
    }
}

void CLI_Process(void)
{
    if (!cli_rx.ready) return;
    cli_rx.ready = 0;
    cli_rx.pos   = 0;

    char *cmd = cli_rx.buf;
    while (*cmd == ' ' || *cmd == '\t') cmd++;

    if (strcmp(cmd, "help") == 0)       cmd_help();
    else if (strcmp(cmd, "stats") == 0)  cmd_stats();
    else if (strcmp(cmd, "perf") == 0)   cmd_perf();
    else if (strcmp(cmd, "storage") == 0) cmd_storage();
    else if (strcmp(cmd, "tasks") == 0)  cmd_tasks();
    else if (strcmp(cmd, "reset") == 0)  cmd_reset();
    else if (strlen(cmd) == 0)           {}
    else printf("Unknown: '%s'. Type 'help'\r\n", cmd);

    printf(CLI_PROMPT);
}

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
    printf("CPU Load:    %.1f%%\r\n", s.cpu_load_pct);
    printf("Task Switch: ~%u us  (Cortex-M3 @72MHz)\r\n",
           (unsigned)s.task_switch_us);

    printf("\r\n--- Task Stack (bytes free) ---\r\n");
    printf("Sensor:  %u\r\n", (unsigned)uxTaskGetStackHighWaterMark(hSensor));
    printf("Alarm:   %u\r\n", (unsigned)uxTaskGetStackHighWaterMark(hAlarm));
    printf("Display: %u\r\n", (unsigned)uxTaskGetStackHighWaterMark(hDisplay));
    printf("Flash:   %u\r\n", (unsigned)uxTaskGetStackHighWaterMark(hFlash));
    printf("WiFi:    %u\r\n", (unsigned)uxTaskGetStackHighWaterMark(hWiFi));
}

static void cmd_perf(void)
{
    PerfStats s;
    Perf_GetStats(&s);

    printf("\r\n===== MQTT Performance =====\r\n");
    printf("Publish count: %lu\r\n", (unsigned long)s.mqtt_pub_cnt);
    printf("Last latency:  %lu ms\r\n", (unsigned long)s.mqtt_pub_lat_ms);
    printf("Max latency:   %lu ms\r\n", (unsigned long)s.mqtt_pub_lat_max);
    printf("Avg latency:   %lu ms\r\n", (unsigned long)s.mqtt_pub_lat_avg);
    printf("---\r\n");
    printf("Reconnects:    %lu\r\n", (unsigned long)s.reconnect_cnt);
    printf("Reconnect OK:  %lu\r\n", (unsigned long)s.reconnect_success);
}

static void cmd_storage(void)
{
    uint32_t records = ADC_Get_Record_Count();
    uint32_t addr    = ADC_Get_Write_Address();
    uint32_t sector_addr = (addr / 4096) * 4096;
    uint32_t offset      = addr - sector_addr;

    printf("\r\n===== Flash Storage =====\r\n");
    printf("Chip:          W25Q64 (8 MB)\r\n");
    printf("Scheme:        2 x 4KB sectors, dual rotation\r\n");
    printf("Record:        12 bytes + CRC8\r\n");
    printf("Records/sector: ~340\r\n");
    printf("---\r\n");
    printf("Total records: %lu\r\n", (unsigned long)records);
    printf("Write addr:    0x%06lX\r\n", (unsigned long)addr);
    printf("Sector usage:  %.1f%%\r\n",
           100.0f * (float)offset / 4096.0f);
}

static void cmd_tasks(void)
{
    TickType_t tick = xTaskGetTickCount();

    printf("\r\n===== Task Details =====\r\n");
    printf("Tick: %lu  (%lu s)\r\n",
           (unsigned long)tick,
           (unsigned long)(tick / configTICK_RATE_HZ));

    printf("\r\n%-10s %4s %6s %8s\r\n", "Name", "Prio", "Stack", "Free");
    printf("---------- ---- ------ --------\r\n");
    printf("%-10s %4d %6u %8u\r\n", "Sensor",  3, 200,
           (unsigned)uxTaskGetStackHighWaterMark(hSensor));
    printf("%-10s %4d %6u %8u\r\n", "Alarm",   2, 256,
           (unsigned)uxTaskGetStackHighWaterMark(hAlarm));
    printf("%-10s %4d %6u %8u\r\n", "WiFi",    2, 512,
           (unsigned)uxTaskGetStackHighWaterMark(hWiFi));
    printf("%-10s %4d %6u %8u\r\n", "Flash",   1, 256,
           (unsigned)uxTaskGetStackHighWaterMark(hFlash));
    printf("%-10s %4d %6u %8u\r\n", "Display", 1, 512,
           (unsigned)uxTaskGetStackHighWaterMark(hDisplay));

    printf("\r\nHeap: %lu / %lu bytes free\r\n",
           (unsigned long)xPortGetFreeHeapSize(),
           (unsigned long)configTOTAL_HEAP_SIZE);
}

static void cmd_reset(void)
{
    Perf_Init();
    printf("Performance counters reset.\r\n");
}
