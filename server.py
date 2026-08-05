#!/usr/bin/env python3
"""
ETMS MQTT Monitor — PC 端上位机
订阅 etms/sensor 主题，实时显示传感器数据
"""

import json
import time
import paho.mqtt.client as mqtt

BROKER = "test.mosquitto.org"
PORT = 1883
TOPIC = "etms/sensor"


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[OK] Connected to {BROKER}")
        client.subscribe(TOPIC)
        print(f"[OK] Subscribed to '{TOPIC}'")
        print("-" * 60)
        print(f"{'Time':<12} {'PS(V)':<8} {'MQ135(V)':<10} {'MQ2(V)':<8} {'Temp(C)':<9} {'Hum(%)':<8}")
        print("-" * 60)
    else:
        print(f"[ERR] Connection failed, rc={rc}")


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload)
        ts = time.strftime("%H:%M:%S", time.localtime(data.get("ts", 0) / 1000))
        print(f"{ts:<12} {data.get('ps', 0):<8.2f} {data.get('mq135', 0):<10.2f} "
              f"{data.get('mq2', 0):<8.2f} {data.get('t', 0):<9.1f} {data.get('h', 0):<8.1f}")
    except json.JSONDecodeError:
        print(f"[RAW] {msg.payload}")


def main():
    client = mqtt.Client(client_id="etms-monitor-pc")
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"ETMS Monitor — Connecting to {BROKER}:{PORT}...")
    client.connect(BROKER, PORT, keepalive=60)

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n[BYE] Disconnected")
        client.disconnect()


if __name__ == "__main__":
    main()
