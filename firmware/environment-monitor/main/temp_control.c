#include "temp_control.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

// ======================= 引脚定义 =======================
#define HEATER_GPIO       GPIO_NUM_36   // 接继电器 IN1，控制加热片

// 预留 IO (全部配置为输入下拉)
#define RESERVED_IO_MASK  ((1ULL << GPIO_NUM_35) | \
                           (1ULL << GPIO_NUM_37) | \
                           (1ULL << GPIO_NUM_38) | \
                           (1ULL << GPIO_NUM_39) | \
                           (1ULL << GPIO_NUM_40) | \
                           (1ULL << GPIO_NUM_41) | \
                           (1ULL << GPIO_NUM_42))

// ======================= 温控参数 =======================
#define TARGET_TEMP       25.0f         // 目标适宜温度 (°C)
#define HYSTERESIS         0.5f         // 回差 (°C)，防止继电器反复跳变

// ======================= 内部状态 =======================
static const char *TAG = "TEMP_CTRL";
static volatile float g_current_temp = -999.0f;
static volatile bool  g_heating = false;

// ======================= 温控任务 =======================
static void temp_control_task(void *pvParameters)
{
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        float temp = g_current_temp;

        if (temp > -100.0f) {
            if (!g_heating && temp < TARGET_TEMP - HYSTERESIS / 2.0f) {
                gpio_set_level(HEATER_GPIO, 1);
                g_heating = true;
                ESP_LOGI(TAG, "开始加热 (当前 %.2f°C < 下限 %.2f°C)",
                         temp, TARGET_TEMP - HYSTERESIS / 2.0f);
            } else if (g_heating && temp > TARGET_TEMP + HYSTERESIS / 2.0f) {
                gpio_set_level(HEATER_GPIO, 0);
                g_heating = false;
                ESP_LOGI(TAG, "停止加热 (当前 %.2f°C >= 上限 %.2f°C)",
                         temp, TARGET_TEMP + HYSTERESIS / 2.0f);
            }
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(2000));
    }
}

// ======================= 初始化函数 =======================
void temp_control_init(void)
{
    // 1. 配置继电器控制引脚 (输出)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << HEATER_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(HEATER_GPIO, 0);  // 初始：停止加热

    // 2. 预留 IO 配置为输入下拉 (省电、防悬空、防误触发)
    io_conf.pin_bit_mask = RESERVED_IO_MASK;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // 3. 创建温控任务
    xTaskCreate(temp_control_task, "temp_ctrl",
                configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);

    ESP_LOGI(TAG, "温控模块初始化完成");
    ESP_LOGI(TAG, "  目标温度: %.1f°C, 回差: ±%.2f°C", TARGET_TEMP, HYSTERESIS / 2.0f);
    ESP_LOGI(TAG, "  加热控制: GPIO%d → 继电器 IN1", HEATER_GPIO);
    ESP_LOGI(TAG, "  预留 IO: GPIO35, 37, 38, 39, 40, 41, 42 (输入下拉)");
}

// ======================= 温度更新 (由 BME680 任务调用) =======================
void temp_control_update_temperature(float temperature)
{
    g_current_temp = temperature;
}
