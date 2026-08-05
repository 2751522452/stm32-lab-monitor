/**
 * @brief  运行时性能测量模块
 *
 * 测量指标:
 *   - CPU 占用率 (空闲任务钩子, DMA vs 轮询对比)
 *   - 任务切换延迟 (~μs 级, GPIO 翻转 + 逻辑分析仪)
 *   - 系统连续运行时长 (s)
 *   - MQTT 通信延迟 (ms) + 重连次数/成功率
 *   - Flash 存储统计 (记录数 / 扇区使用率)
 */

#ifndef __PERF_H__
#define __PERF_H__

#include "main.h"
#include <stdint.h>

/* ================================================================
 *  性能统计数据结构
 * ================================================================ */
typedef struct {
    /* ---- CPU ---- */
    float    cpu_load_pct;          /* CPU 占用率 (%) */
    uint32_t idle_count;            /* 空闲计数 (调试用) */
    uint32_t idle_count_max;        /* 校准: 1s 最大空闲计数 */

    /* ---- 时间 ---- */
    uint32_t uptime_sec;            /* 系统运行时长 (s) */
    uint32_t tick_1s;               /* 1s 基准计数 */

    /* ---- 任务 ---- */
    uint16_t task_switch_us;        /* 任务切换延迟 (μs, 需示波器实测) */

    /* ---- MQTT ---- */
    uint32_t mqtt_pub_cnt;          /* 累计发布次数 */
    uint32_t mqtt_pub_lat_ms;       /* 最近一次发布延迟 (ms) */
    uint32_t mqtt_pub_lat_min;      /* 最小延迟 */
    uint32_t mqtt_pub_lat_max;      /* 最大延迟 */
    uint32_t mqtt_pub_lat_avg;      /* 平均延迟 (累计/次数) */
    uint32_t mqtt_pub_lat_sum;      /* 延迟累计 (算平均) */
    uint32_t reconnect_cnt;         /* 断线重连次数 */
    uint32_t reconnect_success;     /* 重连成功次数 */

    /* ---- Flash ---- */
    uint32_t flash_records;         /* 当前存储记录数 */
    uint32_t flash_sectors_used;    /* 已写扇区数 */
    uint32_t flash_write_err;       /* 写入错误次数 */
} PerfStats;

/* ================================================================
 *  API
 * ================================================================ */

void Perf_Init(void);

/* 必须在 vApplicationIdleHook() 中调用 */
void Perf_IdleTick(void);

/* 定时调用 (1s 周期), 计算 CPU 负载 */
void Perf_Update(void);

/* 获取统计快照 */
void Perf_GetStats(PerfStats *s);

/* MQTT 事件记录 */
void Perf_RecordMqttPub(uint32_t latency_ms);
void Perf_RecordReconnect(int success);

/* Flash 事件记录 */
void Perf_RecordFlashWrite(int ok);

/* 系统运行时长 */
uint32_t Perf_GetUptime(void);

/* 校准: 在空闲循环中运行, 测定最大空闲计数 */
void Perf_Calibrate(void);

/* Flash 状态更新 (由 FlashTask 调用) */
void Perf_UpdateFlashStats(uint32_t records, uint32_t sectors);

/* 串口输出所有性能指标摘要 */
void Perf_PrintSummary(void);

#endif /* __PERF_H__ */
