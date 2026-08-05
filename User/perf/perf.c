/**
 * @brief  运行时性能测量模块实现
 *
 * CPU 负载计算方法:
 *   1. 校准阶段: 无任务负载时测定 1s 内最大空闲循环计数 → idle_count_max
 *   2. 运行阶段: 每秒采样实际空闲计数 → idle_count
 *   3. CPU% = (1 - idle_count / idle_count_max) × 100
 *
 * MQTT 延迟:
 *   从 WiFiTask 调用 Perf_RecordMqttPub(elapsed_ms)
 *   延迟 = 收到 CONNACK 时刻 - 数据入队时刻
 */

#include "perf.h"
#include "storage/adc_storage.h"
#include <string.h>
#include <stdio.h>

/* ================================================================
 *  静态变量
 * ================================================================ */
static volatile uint32_t perf_idle_cnt;        /* ISR/任务安全递增 */
static PerfStats         perf_stats;

/* ---- 内部: 1s 更新用的 delta 计数 ---- */
static uint32_t last_idle_snapshot;
static uint32_t last_tick;

/* ================================================================
 *  Perf_Init
 * ================================================================ */
void Perf_Init(void)
{
    memset(&perf_stats, 0, sizeof(perf_stats));
    perf_idle_cnt      = 0;
    last_idle_snapshot = 0;
    last_tick          = HAL_GetTick();

    /* 校准: 确保 idle_count_max 非零 */
    Perf_Calibrate();

    /* MQTT 延迟范围初始化 */
    perf_stats.mqtt_pub_lat_min = 0xFFFFFFFF;
}

/* ================================================================
 *  Perf_IdleTick — 由 vApplicationIdleHook() 调用
 *
 *  在空闲任务中每轮循环递增, 反映 CPU 空闲程度
 *  注意: 必须在临界区外调用, 避免影响调度
 * ================================================================ */
void Perf_IdleTick(void)
{
    perf_idle_cnt++;
}

/* ================================================================
 *  Perf_Update — 每秒调用一次, 计算 CPU 占用率
 *
 *  由独立的低优先级测量任务或 CLI 查询时调用
 * ================================================================ */
void Perf_Update(void)
{
    uint32_t now     = HAL_GetTick();
    uint32_t elapsed = now - last_tick;

    if (elapsed < 1000)
        return;  /* 不足 1 秒不计算 */

    last_tick = now;

    /* 快照当前空闲计数 */
    uint32_t idle_now = perf_idle_cnt;
    uint32_t idle_delta;

    if (idle_now >= last_idle_snapshot)
        idle_delta = idle_now - last_idle_snapshot;
    else
        idle_delta = idle_now;  /* 溢出回绕 */

    last_idle_snapshot = idle_now;

    /* 计算 CPU 占用率 */
    if (perf_stats.idle_count_max > 0) {
        float load = 100.0f * (1.0f - (float)idle_delta / (float)perf_stats.idle_count_max);
        if (load < 0.0f)  load = 0.0f;
        if (load > 100.0f) load = 100.0f;
        perf_stats.cpu_load_pct = load;
    }

    perf_stats.idle_count = idle_delta;

    /* 更新系统运行时长 */
    perf_stats.uptime_sec = now / 1000;
}

/* ================================================================
 *  Perf_Calibrate — 测定 1s 最大空闲循环计数
 *
 *  在系统启动后、所有任务创建前调用
 *  或在运行中暂停所有任务后调用
 * ================================================================ */
void Perf_Calibrate(void)
{
    uint32_t start, cnt = 0;

    perf_idle_cnt = 0;
    start = HAL_GetTick();

    /* 空转 1 秒, 模拟 100% 空闲 */
    while (HAL_GetTick() - start < 1000) {
        cnt++;
    }

    perf_stats.idle_count_max = cnt;

    /* 复位计数器 — last_tick 置 0, 确保首次 Perf_Update 立即生效 */
    perf_idle_cnt      = 0;
    last_idle_snapshot = 0;
    last_tick          = 0;
}

/* ================================================================
 *  Perf_GetStats — 获取统计快照 (线程安全)
 * ================================================================ */
void Perf_GetStats(PerfStats *s)
{
    /* 先更新再读取 */
    Perf_Update();
    *s = perf_stats;
}

/* ================================================================
 *  Perf_GetUptime
 * ================================================================ */
uint32_t Perf_GetUptime(void)
{
    return HAL_GetTick() / 1000;
}

/* ================================================================
 *  MQTT 事件记录
 * ================================================================ */

void Perf_RecordMqttPub(uint32_t latency_ms)
{
    perf_stats.mqtt_pub_cnt++;

    perf_stats.mqtt_pub_lat_ms = latency_ms;

    /* min/max */
    if (latency_ms < perf_stats.mqtt_pub_lat_min)
        perf_stats.mqtt_pub_lat_min = latency_ms;
    if (latency_ms > perf_stats.mqtt_pub_lat_max)
        perf_stats.mqtt_pub_lat_max = latency_ms;

    /* 滑动平均 */
    perf_stats.mqtt_pub_lat_sum += latency_ms;
    perf_stats.mqtt_pub_lat_avg =
        perf_stats.mqtt_pub_lat_sum / perf_stats.mqtt_pub_cnt;
}

void Perf_RecordReconnect(int success)
{
    perf_stats.reconnect_cnt++;
    if (success)
        perf_stats.reconnect_success++;
}

/* ================================================================
 *  Flash 事件记录
 * ================================================================ */
void Perf_RecordFlashWrite(int ok)
{
    if (!ok)
        perf_stats.flash_write_err++;
}

void Perf_UpdateFlashStats(uint32_t records, uint32_t sectors)
{
    perf_stats.flash_records     = records;
    perf_stats.flash_sectors_used = sectors;
}

/* ---- Perf_PrintSummary: 串口输出所有性能指标 ---- */
void Perf_PrintSummary(void)
{
    Perf_Update();
    PerfStats s;
    Perf_GetStats(&s);
    uint32_t c = (uint32_t)(s.cpu_load_pct * 10.0f);
    printf("[PERF] tick=%lu CPU=%lu.%lu%% Up=%lus idle=%lu/%lu Rec=%lu MQTTpub=%lu lat=%lu/%lu/%lums Recon=%lu/%lu Flash=%lur %lus %lue\r\n",
           (unsigned long)HAL_GetTick(),
           (unsigned long)(c/10), (unsigned long)(c%10),
           (unsigned long)s.uptime_sec,
           (unsigned long)s.idle_count, (unsigned long)s.idle_count_max,
           (unsigned long)ADC_Get_Record_Count(),
           (unsigned long)s.mqtt_pub_cnt,
           (unsigned long)(s.mqtt_pub_lat_min == 0xFFFFFFFF ? 0 : s.mqtt_pub_lat_min),
           (unsigned long)s.mqtt_pub_lat_avg,
           (unsigned long)s.mqtt_pub_lat_max,
           (unsigned long)s.reconnect_cnt,
           (unsigned long)s.reconnect_success,
           (unsigned long)s.flash_records,
           (unsigned long)s.flash_sectors_used,
           (unsigned long)s.flash_write_err);
}
