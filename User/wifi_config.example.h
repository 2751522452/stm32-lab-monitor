/**
 * @brief  WiFi 配置文件模板
 *
 * 复制为 wifi_config.h 并填入实际值:
 *   cp wifi_config.example.h wifi_config.h
 *
 * wifi_config.h 已加入 .gitignore，不会被提交。
 */

#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"
#define MQTT_BROKER     "test.mosquitto.org"
#define MQTT_PORT       1883

#endif /* __WIFI_CONFIG_H__ */
