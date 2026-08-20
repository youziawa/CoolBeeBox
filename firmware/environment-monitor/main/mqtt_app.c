#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "mqtt_app.h"

static const char *TAG = "MQTT_APP";
static esp_mqtt_client_handle_t client = NULL;

#ifndef COOLBEE_MQTT_URI
#define COOLBEE_MQTT_URI "mqtt://YOUR_MQTT_HOST"
#endif
#ifndef COOLBEE_MQTT_USERNAME
#define COOLBEE_MQTT_USERNAME "YOUR_MQTT_USERNAME"
#endif
#ifndef COOLBEE_MQTT_PASSWORD
#define COOLBEE_MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
#endif

// 缓存最新的 BME680 数据，等到 SCD40 的 5 秒周期到达时，一并发送
static float cached_bme_temp = 0;
static float cached_bme_press = 0;
static float cached_bme_hum = 0;
static float cached_bme_gas = 0;
static bool bme_data_ready = false;

// MQTT 事件回调函数，用于监听连接成功、失败等状态
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "已成功连接到 MQTT Broker");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "已断开 MQTT 连接");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "消息发布成功, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT 发生错误");
            break;
        default:
            break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = COOLBEE_MQTT_URI,
        .credentials.username = COOLBEE_MQTT_USERNAME,
        .credentials.authentication.password = COOLBEE_MQTT_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT 客户端初始化失败");
        return;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_update_bme680_data(float temp, float press, float hum, float gas) {
    if (client == NULL) {
        return; // 未连接则不处理
    }
    // 缓存 BME680 数据
    cached_bme_temp = temp;
    cached_bme_press = press;
    cached_bme_hum = hum;
    cached_bme_gas = gas;
    bme_data_ready = true;
    
    ESP_LOGI(TAG, "本地已缓存 BME680 最新数据，等待 SCD40 同步发送");
}

void mqtt_publish_merged_sensor_data(uint16_t co2) {
    if (client == NULL || !bme_data_ready) {
        return; // 等待两者都有数据再发送
    }

    int64_t uptime_sec = esp_timer_get_time() / 1000000;

    // 使用 snprintf 将两个传感器的数据打包成一整条 JSON，只保留 BME680 的温湿度
    char payload[200];
    snprintf(payload, sizeof(payload), 
             "{\"temperature\": %.2f, \"pressure\": %.2f, \"humidity\": %.2f, \"gas\": %.2f, \"co2\": %u, \"uptime\": %lld}", 
             cached_bme_temp, cached_bme_press, cached_bme_hum, cached_bme_gas,
             co2, uptime_sec);

    // 发送到合成的数据主题 device/coolbee/sensors
    int msg_id = esp_mqtt_client_publish(client, "device/coolbee/sensors", payload, 0, 0, 0);
    if (msg_id >= 0) {
        ESP_LOGI(TAG, "发送合并传感器数据就绪, payload=%s", payload);
    } else {
        ESP_LOGE(TAG, "发送合并传感器数据失败");
    }
}
