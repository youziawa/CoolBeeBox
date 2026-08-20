#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "bme680.h" // 引用官方的库头文件 bme680.h
#include "app_bme680.h"
#include "mqtt_app.h"
#include "temp_control.h"

// 根据你的硬件定义I2C引脚，比如用GPIO 5和GPIO 4
#define I2C_SDA_PIN GPIO_NUM_5 
#define I2C_SCL_PIN GPIO_NUM_4 

static const char *TAG = "APP_BME680";

static void bme680_test_task(void *pvParameters)
{
    bme680_t sensor;
    memset(&sensor, 0, sizeof(bme680_t));

    // 1. 初始化 i2cdev 库
    ESP_ERROR_CHECK(i2cdev_init());

    // 2. 初始化传感器描述符。BME680 I2C地址通常是 BME680_I2C_ADDR_0 (0x76) 或者是 0x77
    ESP_ERROR_CHECK(bme680_init_desc(&sensor, BME680_I2C_ADDR_0, 0, I2C_SDA_PIN, I2C_SCL_PIN));

    // 3. 初始化传感器，读取出厂校准数据
    esp_err_t res = bme680_init_sensor(&sensor);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "初始化 BME680 失败: %s", esp_err_to_name(res));
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "BME680 初始化成功!");

    // 配置过采样率 (Oversampling)
    bme680_set_oversampling_rates(&sensor, BME680_OSR_4X, BME680_OSR_2X, BME680_OSR_2X);
    bme680_set_filter_size(&sensor, BME680_IIR_SIZE_7);

    // 配置内部加热器参数以进行TVOC气体检测 (PROFILE 0, 加热到 320 度, 持续 150ms)
    bme680_set_heater_profile(&sensor, 0, 320, 150);
    bme680_use_heater_profile(&sensor, 0);

    bme680_values_float_t values;

    while (1) {
        // 4. 实时触发一次测量并获取浮点格式的检测结果
        if (bme680_measure_float(&sensor, &values) == ESP_OK) {
            ESP_LOGI(TAG, "温度: %.2f °C, 气压: %.2f hPa, 湿度: %.2f%%, 气体电阻: %.2f Ohm",
                     values.temperature, values.pressure, values.humidity, values.gas_resistance);
            // 缓存 BME680 数据，等到 SCD40 的周期到达时由对方一同发出
            mqtt_update_bme680_data(values.temperature, values.pressure, values.humidity, values.gas_resistance);
            // 同时更新温控模块的温度数据
            temp_control_update_temperature(values.temperature);
        } else {
            ESP_LOGE(TAG, "读取传感器数据失败...");
        }

        // 可以适当增加延迟（例如 2000 毫秒一次采集避免频繁刷屏）
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_bme680_init(void)
{
    // 创建后台任务去处理传感器读取
    xTaskCreate(bme680_test_task, "bme680_test_task", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
}