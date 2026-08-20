#include <stdio.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include "app_bme680.h"
#include "scd40_app.h"
#include "wifi_app.h"
#include "mqtt_app.h"
#include "temp_control.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    // 核心前提 1：初始化 NVS (Non-Volatile Storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // 如果 NVS 分区损坏或版本变更，先擦除再重新初始化
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 核心前提 2：启动上文的 Wi-Fi 逻辑
    wifi_init_sta();
    
    // 连接成功后，启动 MQTT 连云服务
    mqtt_app_start();
    
    // 初始化 BME680 传感器读取任务
    app_bme680_init();
    
    // 初始化 SCD40 二氧化碳传感器读取任务
    scd40_app_init();

    // 初始化温控模块 (继电器控制加热片)
    temp_control_init();

    ESP_LOGI(TAG, "系统主线就绪");
}