# ETMS — Environmental Threat Monitoring System

[![Platform](https://img.shields.io/badge/Platform-STM32F103C8T6-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green)](https://www.freertos.org/)
[![Protocol](https://img.shields.io/badge/Protocol-MQTT%203.1.1-orange)](https://mqtt.org/)
[![Tests](https://img.shields.io/badge/Tests-58%20passed-brightgreen)](Tests/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey)](LICENSE)

面向可燃气体与空气质量监测场景，独立完成从**外设驱动 → FreeRTOS 多任务架构 → MQTT 云端通信**的全链路嵌入式开发。

STM32F103C8T6 (Cortex-M3, 20KB SRAM, 64KB Flash) + ESP8266 WiFi 模组。

---

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                        云端 (MQTT Broker)                     │
│                   test.mosquitto.org:1883                    │
└──────────────────────────┬──────────────────────────────────┘
                           │ WiFi (TCP/IP)
                    ┌──────┴──────┐
                    │   ESP8266   │  ← AT 指令状态机 + 环形缓冲
                    └──────┬──────┘
                           │ UART3 (115200bps)
              ┌────────────┴────────────┐
              │   STM32F103C8T6 @72MHz  │
              │   FreeRTOS (6 Tasks)    │
              │   ┌──────────────────┐  │
              │   │ SensorTask   P3  │  │ ← ADC DMA 3通道 (MQ135/MQ2/PS)
              │   │ AlarmTask    P2  │  │ ← 告警等级 + LED/Buzzer
              │   │ MQTask       P2  │  │ ← 预热状态机 (180s/120s)
              │   │ WiFiTask     P2  │  │ ← MQTT 3.1.1 协议栈
              │   │ DisplayTask  P1  │  │ ← OLED (SSD1306 I2C)
              │   │ FlashTask    P1  │  │ ← W25Q64 双扇区存储
              │   └──────────────────┘  │
              └─────────────────────────┘
```

---

## 硬件连接

| 外设 | 接口 | 引脚 | 说明 |
|------|------|------|------|
| MQ135 | ADC1_CH1 | PA1 | 空气质量传感器 |
| MQ2 | ADC1_CH2 | PA2 | 可燃气体传感器 |
| DHT20 | I2C1 | PB6/PB7 | 温湿度传感器 |
| W25Q64 | SPI2 | PB13-PB15 | 8MB Flash 存储 |
| ESP8266 | USART3 | PB10/PB11 | WiFi AT 指令模组 |
| SSD1306 OLED | I2C1 | PB6/PB7 | 0.96" 显示面板 |
| Buzzer/LED | GPIO | PB0/PB1 | 告警输出 |

---

## 软件设计亮点

### 1. MQTT 3.1.1 协议栈 — 零第三方依赖

> 代码位置：[`User/wifi/mqtt.c`](User/wifi/mqtt.c) (135行)

- 实现 **CONNECT / PUBLISH / PINGREQ** 三种报文构建
- **剩余长度可变长编码**（1~4 字节），大端序主题/长度写入
- 通过 ESP8266 透传对接公网 MQTT Broker
- 纯 C 实现，零动态内存分配，无第三方 MQTT 库依赖

```c
// 剩余长度编码: 每字节低7位为数据，最高位=1表示后续还有字节
static int encode_remaining_length(uint8_t *buf, uint32_t len) {
    int i = 0;
    do {
        uint8_t byte = (uint8_t)(len % 128);
        len /= 128;
        if (len > 0) byte |= 0x80;
        buf[i++] = byte;
    } while (len > 0 && i < 4);
    return i;
}
```

### 2. MQ135/MQ2 非阻塞预热状态机

> 代码位置：[`User/mq/mq.c`](User/mq/mq.c)

- **三态切换**: PREHEAT → STABLE → FAULT
- MQ135 预热 180s / MQ2 预热 120s，基于系统 tick 计时，**不阻塞调度器**
- 连续异常 **5s** 才判故障（防 ADC 噪声误报），单次正常即恢复

```
PREHEAT ──预热超时──▶ STABLE ──ADC连续异常5s──▶ FAULT
                        ▲                          │
                        └──── ADC恢复正常 ──────────┘
```

### 3. W25Q64 双扇区轮换 + 掉电安全存储

> 代码位置：[`User/storage/adc_storage.c`](User/storage/adc_storage.c) (353行)

- 扇区头部三态**单向迁移**: `0xFF(Idle) → 0x7F(Writing) → 0x3F(Valid)`
- 利用 **Flash 只能 1→0** 的物理特性，无需电池备份
- 掉电后通过状态位 + CRC8 自动恢复至最近有效数据
- 版本号单调递增，支持扇区写中断时**自动回退**到旧扇区

```
┌──────────────────┐  ┌──────────────────┐
│   Sector A       │  │   Sector B       │
│ ┌──────────────┐ │  │ ┌──────────────┐ │
│ │ Header (12B) │ │  │ │ Header (12B) │ │
│ │ magic/ver/crc │ │  │ │ magic/ver/crc │ │
│ ├──────────────┤ │  │ ├──────────────┤ │
│ │ Record 1-340 │ │  │ │ Record 1-340 │ │
│ │ (12B each)   │ │  │ │ (12B each)   │ │
│ └──────────────┘ │  │ └──────────────┘ │
│ cur/old 轮换     │  │ cur/old 轮换     │
└──────────────────┘  └──────────────────┘
```

### 4. FreeRTOS 6 任务架构 + 栈水位监测

> 代码位置：[`User/main.c`](User/main.c)

| 任务 | 优先级 | 周期 | 栈/最小剩余 | 职责 |
|------|--------|------|-------------|------|
| SensorTask | **3** (最高) | 20ms | 200B | ADC DMA 采集 → 互斥锁写入共享数据 |
| AlarmTask | 2 | 20ms | 256B | 告警等级计算 + LED/Buzzer 驱动 |
| MQTask | 2 | 1s | 256B | MQ 传感器状态机更新 |
| WiFiTask | 2 | 500ms | 512B | ESP8266 AT + MQTT 协议 |
| DisplayTask | 1 | 200ms | 512B | OLED 数据显示 |
| FlashTask | 1 | 2s | 256B | W25Q64 数据持久化 |

- 编译期: `configCHECK_FOR_STACK_OVERFLOW = 2`
- 运行时: `uxTaskGetStackHighWaterMark()` 周期监测
- 硬件兜底: IWDG 独立看门狗（2s 超时自动复位）

### 5. ESP8266 非阻塞 AT 指令状态机 + 环形缓冲

> 代码位置：[`User/wifi/esp8266.c`](User/wifi/esp8266.c) + [`User/wifi/ringbuf.c`](User/wifi/ringbuf.c)

- AT 指令流程: RESET → INIT → SET_MODE → CONNECT_WIFI → CONNECT_SERVER → READY
- ISR 接收字节写入**环形缓冲（512B）**，任务级非阻塞解析
- 自定义二进制帧协议 `[0xAA 0x55] [TYPE] [LEN BE] [PAYLOAD] [CRC8]`

---

## PC 端单元测试（58 条）

采用 Unity 测试框架，对关键模块进行**剥离硬件的纯逻辑测试**:

| 模块 | 测试数 | 覆盖内容 |
|------|--------|---------|
| Alarm | 22 | CalcLevel 5级阈值 + SetLevel 配置 + Process 闪烁/常亮 |
| MQ | 11 | PREHEAT→STABLE→FAULT 全状态迁移 + 超时/恢复 |
| CRC8 | 8 | 已知向量验证 + Record CRC 防篡改 |
| MQTT | 9 | CONNECT/PUBLISH/PINGREQ 报文结构正确性 |
| Storage | 8 | 首次上电 + 扇区轮换 + 掉电恢复 + 格式化 |
| **合计** | **58** | |

```bash
cd Tests
make              # 编译并运行全部测试
make test_alarm   # 单独运行某模块
```

---

## UART 调试终端

连接 USART1 (PA9/PA10, 115200 8N1):

```
========================================
  ETMS Debug Console v1.0
  STM32F103C8T6 + FreeRTOS
  Type 'help' for commands
========================================
ETMS> help

Commands:
  stats    - CPU load, uptime, stack watermarks
  perf     - MQTT latency, reconnect stats
  storage  - Flash record count, sector usage
  tasks    - Per-task stack high water mark
  reset    - Reset performance counters
  help     - This list
```

---

## 快速开始

### 硬件要求

- STM32F103C8T6 最小系统板（Blue Pill）
- ESP8266-01S WiFi 模组
- MQ135 / MQ2 / DHT20 传感器
- W25Q64 Flash 模组（SPI）
- 0.96" OLED (SSD1306, I2C)
- USB-TTL 串口工具（用于调试终端）

### 编译与烧录

1. 安装 Keil MDK-ARM v5
2. 打开 `Project/ETMS.uvprojx`
3. 修改 `User/main.c` 中 `WiFiTask` 的 WiFi SSID/密码
4. 编译（F7）→ 下载（F8）

### PC 上位机

```bash
pip install paho-mqtt
python server.py
# 订阅 etms/sensor 主题，实时查看传感器数据
```

---

## 项目目录

```
ETMS/
├── User/              # 应用层代码（按模块分目录）
│   ├── adc/           # ADC DMA 多通道采样
│   ├── alarm/         # 告警等级判断 + LED/蜂鸣器驱动
│   ├── cli/           # UART 调试终端
│   ├── dma/           # DMA 配置
│   ├── fonts/         # OLED 字体
│   ├── gpio/          # GPIO 驱动
│   ├── i2c/           # I2C + DHT20 + OLED
│   ├── i2c_led/       # I2C LED 驱动
│   ├── i2c_scan/      # I2C 设备扫描
│   ├── mq/            # MQ 传感器预热状态机
│   ├── perf/          # 性能指标采集
│   ├── sensor/        # 传感器数据聚合
│   ├── spi/           # SPI 驱动
│   ├── spi_w25q64/    # W25Q64 Flash 驱动
│   ├── storage/       # 双扇区掉电安全存储
│   ├── tim/           # 定时器配置
│   ├── usart/         # 串口驱动
│   ├── watchdog/      # IWDG 独立看门狗
│   └── wifi/          # ESP8266 + MQTT + 环形缓冲
├── FreeRTOS/          # FreeRTOS 内核源码
├── Libraries/         # STM32 HAL 库
├── Tests/             # PC 端单元测试 (Unity + 58条)
│   ├── mocks/         # Mock 层 (Flash, HAL)
│   ├── stubs/         # 桩文件
│   └── unity/         # Unity 测试框架
├── Doc/               # 文档
└── Project/           # Keil MDK 工程文件
```

---

## License

MIT © Yang Jichong
