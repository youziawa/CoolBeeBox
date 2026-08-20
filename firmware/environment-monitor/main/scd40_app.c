#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "scd4x.h" // 引用 SCD40/SCD41 组件库头文件
#include "scd40_app.h"
#include "mqtt_app.h"

// 假设和 BME680 共用同一个 I2C 总线引脚: SCL 4, SDA 5
#define I2C_SDA_PIN GPIO_NUM_5 
#define I2C_SCL_PIN GPIO_NUM_4 

static const char *TAG = "SCD40_APP";

static void scd40_test_task(void *pvParameters)
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    // i2cdev_init() 即使在这里重复调用也是安全的，库内做了保护
    ESP_ERROR_CHECK(i2cdev_init());

    // 初始化传感器描述符。SCD4X I2C 地址通常是固定的 0x62，使用 I2C_NUM_0 端口
    ESP_ERROR_CHECK(scd4x_init_desc(&dev, 0, I2C_SDA_PIN, I2C_SCL_PIN));

    // 根据数据手册，上电后唤醒、停止可能正在进行的测量、重新初始化
    ESP_LOGI(TAG, "正在唤醒接通 SCD40...");
    scd4x_wake_up(&dev);
    scd4x_stop_periodic_measurement(&dev);
    scd4x_reinit(&dev);

    // 开始周期性测量 (SCD40大概每5秒出一次数据)
    esp_err_t res = scd4x_start_periodic_measurement(&dev);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "启动 SCD40 周期测量失败: %s", esp_err_to_name(res));
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "SCD40 初始化成功并已开始测量!");

    while (1) {
        // 等待5秒
        vTaskDelay(pdMS_TO_TICKS(5000));

        bool data_ready = false;
        // 检查数据是否准备好
        if (scd4x_get_data_ready_status(&dev, &data_ready) == ESP_OK && data_ready) {
            uint16_t co2;
            float temperature;
            float humidity;
            
            // 读取测量结果
            if (scd4x_read_measurement(&dev, &co2, &temperature, &humidity) == ESP_OK) {
                ESP_LOGI(TAG, "CO2: %u ppm, 温度: %.2f °C, 湿度: %.2f %%", co2, temperature, humidity);
                
                // 仅将最新 CO2 数据与本地缓存的 BME680 数据合并发送到云端
                mqtt_publish_merged_sensor_data(co2);
            } else {
                ESP_LOGE(TAG, "读取 SCD40 传感器数据失败...");
            }
        }
    }
}

void scd40_app_init(void)
{
    // 创建 SCD40 数据读取后台任务
    xTaskCreate(scd40_test_task, "scd40_test_task", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
}