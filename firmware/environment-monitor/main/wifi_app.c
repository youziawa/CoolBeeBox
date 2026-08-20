#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"       // Wi-Fi 核心库
#include "esp_event.h"      // 事件循环库，用于处理 Wi-Fi 状态变化
#include "esp_log.h"
#include "nvs_flash.h"      // NVS 闪存库，Wi-Fi 底层需要用到 NVS 来存储配置
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi_app.h"       // 引入我们自己定义的的 WIFI 头文件

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;  // 用于阻塞等待连接成功的事件组
static int s_retry_num = 0;                    // 记录重连次数
static const char *TAG = "wifi_app";

// 核心回调：系统网络状态发生改变时会触发该函数
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    // 事件1：Wi-Fi Station 模式已经启动
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // 触发底层的连接动作
    } 
    // 事件2：Wi-Fi 断开连接（包括连接失败）
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MY_MAXIMUM_RETRY) {
            esp_wifi_connect(); // 尝试重新连接
            s_retry_num++;
            ESP_LOGI(TAG, "正在重连到 AP...");
        } else {
            // 超过重试次数，向事件组发送失败信号
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"连接 AP 失败");
    } 
    // 事件3：成功获取到 IP 地址
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "成功获取 IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0; // 清零重试次数
        // 向事件组发送成功信号，解除 init 函数的阻塞
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    // 1. 创建事件组，用于后续等待连接结果
    s_wifi_event_group = xEventGroupCreate();

    // 2. 初始化底层 TCP/IP 堆栈
    ESP_ERROR_CHECK(esp_netif_init());

    // 3. 创建系统默认事件循环器 (非常重要，后面注册 event_handler 依赖它)
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 4. 创建默认的 WIFI Station 网络接口
    esp_netif_create_default_wifi_sta();

    // 5. 初始化 WiFi 驱动（使用默认配置）
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    // 6. 注册事件回调：监听全部有关 WIFI_EVENT 的事件 (如启动、断开)
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    // 7. 注册事件回调：监听获取 IP 成功的事件
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // 8. 配置 WiFi 参数（账号、密码等）
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MY_WIFI_SSID,
            .password = MY_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // 设置最低安全认证阈值
        },
    };
    
    // 9. 设置模式为 Station (终端)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // 10. 将配置应用到接口
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // 11. 启动 WiFi 驱动 (此时会触发 WIFI_EVENT_STA_START 事件)
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta 启动完成，等待连接结果...");

    // 12. 阻塞等待：等待 event_handler 中抛出“成功”或“失败”的事件标志
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, // 调用后不自动清除比特位
            pdFALSE, // 满足任意一个比特位即可返回
            portMAX_DELAY); // 永远等待

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "已成功连接到 SSID:%s", MY_WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "无法连接到 SSID:%s", MY_WIFI_SSID);
    }
}

