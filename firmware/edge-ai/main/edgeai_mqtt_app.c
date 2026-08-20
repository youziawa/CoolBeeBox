#include <stdio.h>

#include "edgeai_mqtt_app.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"

static const char *TAG = "EDGEAI_MQTT";
static const char *EDGEAI_TOPIC = "device/coolbee/edgeai";
static const char *EDGEAI_STATUS_TOPIC = "device/coolbee/edgeai/status";
static esp_mqtt_client_handle_t client = NULL;
static bool mqtt_connected = false;

#ifndef COOLBEE_MQTT_URI
#define COOLBEE_MQTT_URI "mqtt://YOUR_MQTT_HOST"
#endif
#ifndef COOLBEE_MQTT_USERNAME
#define COOLBEE_MQTT_USERNAME "YOUR_MQTT_USERNAME"
#endif
#ifndef COOLBEE_MQTT_PASSWORD
#define COOLBEE_MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
#endif

static void edgeai_mqtt_publish_online_status(void)
{
    if (client == NULL || !mqtt_connected) {
        return;
    }

    int64_t uptime_sec = esp_timer_get_time() / 1000000;
    char payload[96];
    snprintf(payload, sizeof(payload), "{\"status\":\"online\",\"uptime\":%lld}", uptime_sec);
    esp_mqtt_client_publish(client, EDGEAI_STATUS_TOPIC, payload, 0, 0, 0);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            mqtt_connected = true;
            ESP_LOGI(TAG, "Connected to MQTT broker");
            edgeai_mqtt_publish_online_status();
            break;
        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "Published EdgeAI message, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;
        default:
            break;
    }
}

void edgeai_mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = COOLBEE_MQTT_URI,
        .credentials.username = COOLBEE_MQTT_USERNAME,
        .credentials.authentication.password = COOLBEE_MQTT_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void edgeai_mqtt_publish_state(const char *label, float confidence, bool stress)
{
    if (client == NULL || !mqtt_connected) {
        ESP_LOGW(TAG, "MQTT is not connected, skip EdgeAI publish");
        return;
    }

    int64_t uptime_sec = esp_timer_get_time() / 1000000;
    char payload[192];
    snprintf(payload, sizeof(payload),
             "{\"label\":\"%s\",\"confidence\":%.3f,\"stress\":%s,\"is_stress\":%s,\"uptime\":%lld}",
             label ? label : "unknown",
             confidence,
             stress ? "true" : "false",
             stress ? "true" : "false",
             uptime_sec);

    int msg_id = esp_mqtt_client_publish(client, EDGEAI_TOPIC, payload, 0, 0, 0);
    if (msg_id >= 0) {
        ESP_LOGI(TAG, "Published EdgeAI state: %s", payload);
    } else {
        ESP_LOGE(TAG, "Failed to publish EdgeAI state");
    }
}
