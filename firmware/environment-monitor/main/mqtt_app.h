#ifndef _MQTT_APP_H_
#define _MQTT_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

// 初始化并启动 MQTT 客户端
void mqtt_app_start(void);

// 将 BME680 数据更新到本地缓存
void mqtt_update_bme680_data(float temp, float press, float hum, float gas);

// 将 SCD40 数据和缓存的 BME680 数据一起合并发布到 MQTT Broker
void mqtt_publish_merged_sensor_data(uint16_t co2);

#ifdef __cplusplus
}
#endif

#endif // _MQTT_APP_H_