# ============================================================
#  ETMS 单元测试 Makefile — PC (gcc) 编译
#
#  用法:
#    make            — 编译并运行所有测试
#    make build      — 仅编译
#    make clean      — 清理
#    make test_alarm — 编译并运行单个测试模块
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers \
          -g -O0 -std=c11

# ---- 包含路径 (stubs 优先于真实源文件) ----
INCS    = -I. \
          -Iunity \
          -Imocks \
          -Istubs \
          -I../User \
          -I../User/alarm \
          -I../User/sensor \
          -I../User/storage \
          -I../User/wifi

# ---- 被测源文件 ----
SRC_ALARM   = ../User/alarm/alarm.c
SRC_SENSOR  = ../User/sensor/sensor.c
SRC_STORAGE = ../User/storage/adc_storage.c
SRC_MQTT    = ../User/wifi/mqtt.c

# ---- Mock 文件 ----
MOCK_OBJS   = build/mock_hal.o build/mock_flash.o

# ---- Unity 框架 ----
UNITY_OBJS  = build/unity.o

# ---- 构建目录 ----
BUILD_DIR   = build

.PHONY: all build clean test_alarm test_sensor_warmup test_crc8 test_storage test_mqtt

all: build test_alarm test_sensor_warmup test_crc8 test_storage test_mqtt
	@echo ""
	@echo "=== All test suites completed ==="

build: $(BUILD_DIR) $(MOCK_OBJS) $(UNITY_OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---- Unity ----
build/unity.o: unity/unity.c unity/unity.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

# ---- Mocks ----
build/mock_hal.o: mocks/mock_hal.c mocks/mock_hal.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

build/mock_flash.o: mocks/mock_flash.c mocks/mock_flash.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

# ---- 告警模块测试 ----
build/test_alarm.o: test_alarm.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

build/alarm_ut.o: $(SRC_ALARM)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

test_alarm: build build/test_alarm.o build/alarm_ut.o $(MOCK_OBJS) $(UNITY_OBJS)
	$(CC) $(CFLAGS) build/test_alarm.o build/alarm_ut.o $(MOCK_OBJS) $(UNITY_OBJS) -o build/$@
	@echo "--- test_alarm ---"
	./build/$@

# ---- 传感器预热状态机测试 ----
build/test_sensor_warmup.o: test_sensor_warmup.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

build/sensor_ut.o: $(SRC_SENSOR)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

test_sensor_warmup: build build/test_sensor_warmup.o build/sensor_ut.o $(MOCK_OBJS) $(UNITY_OBJS)
	$(CC) $(CFLAGS) build/test_sensor_warmup.o build/sensor_ut.o $(MOCK_OBJS) $(UNITY_OBJS) -o build/$@
	@echo "--- test_sensor_warmup ---"
	./build/$@

# ---- CRC8 + MQTT 协议测试 ----
build/test_crc8.o: test_crc8.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

build/mqtt_ut.o: $(SRC_MQTT)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

test_crc8: build build/test_crc8.o build/mqtt_ut.o $(UNITY_OBJS)
	$(CC) $(CFLAGS) build/test_crc8.o build/mqtt_ut.o $(UNITY_OBJS) -o build/$@
	@echo "--- test_crc8 ---"
	./build/$@

# ---- Flash 存储测试 ----
build/test_storage.o: test_storage.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

build/storage_ut.o: $(SRC_STORAGE)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

test_storage: build build/test_storage.o build/storage_ut.o $(MOCK_OBJS) $(UNITY_OBJS)
	$(CC) $(CFLAGS) build/test_storage.o build/storage_ut.o $(MOCK_OBJS) $(UNITY_OBJS) -o build/$@
	@echo "--- test_storage ---"
	./build/$@

# ---- MQTT 协议测试 ----
build/test_mqtt.o: test_mqtt.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

test_mqtt: build build/test_mqtt.o build/mqtt_ut.o $(UNITY_OBJS)
	$(CC) $(CFLAGS) build/test_mqtt.o build/mqtt_ut.o $(UNITY_OBJS) -o build/$@
	@echo "--- test_mqtt ---"
	./build/$@

# ---- 清理 ----
clean:
	rm -rf build
