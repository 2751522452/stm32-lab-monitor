#!/usr/bin/env python3
"""
ETMS 串口调试终端 — 自动过滤 WiFi 调试刷屏，适合录屏演示

用法:
  python console.py COM3          # 连接 COM3 (波特率自动 115200)
  python console.py COM3 --raw    # 不过滤，显示全部输出

命令 (直接输入回车发送):
  stats     — CPU 占用 / 运行时长 / 栈水位
  tasks     — 各任务栈剩余空间
  storage   — Flash 记录数 / 扇区使用
  perf      — MQTT 延迟 / 重连统计
  help      — 帮助
  Ctrl+C    — 退出
"""

import sys
import time
import threading
import serial
import serial.tools.list_ports

FILTER_PATTERNS = [
    "[WiFi]",          # ESP8266 刷屏输出全部过滤
    "SEND OK",
    "AT probe",
    "ERROR -> wait",
]


def list_ports():
    print("可用串口:")
    for p in serial.tools.list_ports.comports():
        print(f"  {p.device} — {p.description}")


def filter_line(line: str) -> bool:
    """返回 True 表示该行应显示"""
    for pat in FILTER_PATTERNS:
        if pat in line:
            return False
    return True


def reader_thread(ser: serial.Serial, raw: bool):
    """后台线程持续读取串口数据并打印"""
    buf = b""
    while True:
        try:
            data = ser.read(1)
            if not data:
                continue
            buf += data
            if data == b"\n":
                line = buf.decode("utf-8", errors="replace").rstrip()
                buf = b""
                if raw or filter_line(line):
                    print(f"\r{line}")
                    # 重新显示提示符
                    sys.stdout.write("ETMS> ")
                    sys.stdout.flush()
        except Exception:
            break


def main():
    if len(sys.argv) < 2:
        list_ports()
        print("\n用法: python console.py <COM口> [--raw]")
        return

    port = sys.argv[1]
    raw = "--raw" in sys.argv

    ser = serial.Serial(port, 115200, timeout=0.05)
    print(f"\n已连接 {port} @ 115200 8N1")
    print("输入命令后回车，Ctrl+C 退出\n")
    print("-" * 60)

    threading.Thread(target=reader_thread, args=(ser, raw), daemon=True).start()

    try:
        while True:
            cmd = input("ETMS> ").strip()
            if cmd:
                ser.write((cmd + "\r\n").encode())
    except (KeyboardInterrupt, EOFError):
        print(f"\n\n断开 {port}")
        ser.close()


if __name__ == "__main__":
    main()
