#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "driver/i2s_std.h"
#include "edgeai_mqtt_app.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "model-parameters/model_metadata.h"
#include "nvs_flash.h"
#include "wifi_app.h"

static const char *TAG = "EdgeAI_Deploy";

#define I2S_BCLK_PIN (4)
#define I2S_WS_PIN   (5)
#define I2S_DIN_PIN  (6)

#define SAMPLE_RATE         EI_CLASSIFIER_FREQUENCY
#define AUDIO_BUFFER_SIZE   (1024)
#define TARGET_SAMPLE_COUNT EI_CLASSIFIER_RAW_SAMPLE_COUNT
#define MIC_GAIN            (1.0f)

#define PCM_DUMP_MODE  (0)
#define PCM_DUMP_LIMIT (10)

static i2s_chan_handle_t rx_chan;
static int16_t *g_audio_buffer = NULL;

typedef struct {
    int16_t pcm_min;
    int16_t pcm_max;
    float pcm_rms;
    float pcm_peak;
} audio_stats_t;

static inline int16_t inmp441_raw_to_s16(int32_t raw)
{
    int32_t s24 = raw >> 8;
    float scaled = (float)(s24 >> 8) * MIC_GAIN;
    if (scaled > 32767.0f) scaled = 32767.0f;
    if (scaled < -32768.0f) scaled = -32768.0f;
    return (int16_t)scaled;
}

static void calculate_audio_stats(const int16_t *audio_data, size_t audio_len, audio_stats_t *stats)
{
    stats->pcm_min = INT16_MAX;
    stats->pcm_max = INT16_MIN;
    stats->pcm_peak = 0.0f;
    long long sum_sq = 0;

    for (size_t i = 0; i < audio_len; i++) {
        int16_t v = audio_data[i];
        if (v < stats->pcm_min) stats->pcm_min = v;
        if (v > stats->pcm_max) stats->pcm_max = v;
        float av = fabsf((float)v);
        if (av > stats->pcm_peak) stats->pcm_peak = av;
        int32_t s = (int32_t)v;
        sum_sq += (long long)s * (long long)s;
    }

    stats->pcm_rms = sqrtf((float)sum_sq / (float)audio_len);
}

static int edge_impulse_audio_get_data(size_t offset, size_t length, float *out_ptr)
{
    if (!g_audio_buffer || (offset + length) > TARGET_SAMPLE_COUNT) {
        return EIDSP_OUT_OF_MEM;
    }

    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (float)g_audio_buffer[offset + i];
    }

    return EIDSP_OK;
}

void i2s_init()
{
    i2s_chan_config_t rb_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&rb_chan_cfg, NULL, &rx_chan));

    i2s_std_config_t rb_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_BCLK_PIN,
            .ws = (gpio_num_t)I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = (gpio_num_t)I2S_DIN_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    rb_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &rb_std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    ESP_LOGI(TAG, "I2S INMP441 initialized at %d Hz", SAMPLE_RATE);
}

static void dump_pcm_window_as_hex(const int16_t *audio_data, size_t audio_len, int dump_index)
{
    printf("PCM_DUMP_BEGIN index=%d sample_rate=%d samples=%u bits=16 channels=1\n",
           dump_index,
           SAMPLE_RATE,
           (unsigned int)audio_len);

    for (size_t i = 0; i < audio_len; i += 32) {
        printf("PCM_DUMP_HEX ");
        size_t end = i + 32;
        if (end > audio_len) {
            end = audio_len;
        }
        for (size_t j = i; j < end; j++) {
            uint16_t v = (uint16_t)audio_data[j];
            printf("%02x%02x", v & 0xff, (v >> 8) & 0xff);
        }
        printf("\n");
    }

    printf("PCM_DUMP_END index=%d\n", dump_index);
}

void audio_inference_task(void *arg)
{
    ESP_LOGI(TAG,
             "Edge Impulse model: %s, labels=%d, window=%u samples, DSP input=%u",
             EI_CLASSIFIER_PROJECT_NAME,
             EI_CLASSIFIER_LABEL_COUNT,
             (unsigned int)EI_CLASSIFIER_RAW_SAMPLE_COUNT,
             (unsigned int)EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);

    int32_t *raw_audio_buffer = (int32_t *)malloc(AUDIO_BUFFER_SIZE * sizeof(int32_t));
    g_audio_buffer = (int16_t *)heap_caps_malloc(TARGET_SAMPLE_COUNT * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!g_audio_buffer) {
        g_audio_buffer = (int16_t *)malloc(TARGET_SAMPLE_COUNT * sizeof(int16_t));
    }

    size_t current_accumulated = 0;
    size_t bytes_read = 0;
    int dump_count = 0;

    if (!raw_audio_buffer || !g_audio_buffer) {
        ESP_LOGE(TAG, "Audio buffer allocation failed");
        if (raw_audio_buffer) free(raw_audio_buffer);
        if (g_audio_buffer) free(g_audio_buffer);
        g_audio_buffer = NULL;
        vTaskDelete(NULL);
    }

    signal_t signal;
    signal.total_length = TARGET_SAMPLE_COUNT;
    signal.get_data = [](size_t offset, size_t length, float *out_ptr) {
        return edge_impulse_audio_get_data(offset, length, out_ptr);
    };

    ESP_LOGI(TAG, "Starting audio capture and Edge Impulse inference loop");

    while (1) {
        esp_err_t res = i2s_channel_read(
            rx_chan,
            raw_audio_buffer,
            AUDIO_BUFFER_SIZE * sizeof(int32_t),
            &bytes_read,
            portMAX_DELAY);

        if (res != ESP_OK || bytes_read == 0) {
            continue;
        }

        size_t samples_read = bytes_read / sizeof(int32_t);
        size_t copy_count = samples_read;
        if (current_accumulated + copy_count > TARGET_SAMPLE_COUNT) {
            copy_count = TARGET_SAMPLE_COUNT - current_accumulated;
        }

        for (size_t i = 0; i < copy_count; i++) {
            g_audio_buffer[current_accumulated + i] = inmp441_raw_to_s16(raw_audio_buffer[i]);
        }
        current_accumulated += copy_count;

        if (current_accumulated < TARGET_SAMPLE_COUNT) {
            continue;
        }

#if PCM_DUMP_MODE
        if (dump_count < PCM_DUMP_LIMIT) {
            dump_pcm_window_as_hex(g_audio_buffer, TARGET_SAMPLE_COUNT, dump_count);
            dump_count++;
        }
        current_accumulated = 0;
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
#endif

        audio_stats_t stats = {};
        calculate_audio_stats(g_audio_buffer, TARGET_SAMPLE_COUNT, &stats);
        ESP_LOGI(TAG,
                 "[Audio] PCM[min=%d max=%d rms=%.1f peak=%.1f]",
                 stats.pcm_min,
                 stats.pcm_max,
                 stats.pcm_rms,
                 stats.pcm_peak);

        ei_impulse_result_t result = {};
        EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
        if (err != EI_IMPULSE_OK) {
            ESP_LOGE(TAG, "run_classifier failed: %d", err);
            current_accumulated = 0;
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        float best_value = 0.0f;
        const char *best_label = "unknown";
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            const char *label = result.classification[i].label;
            float value = result.classification[i].value;
            ESP_LOGI(TAG, "[Result] %s=%.3f", label, value);
            if (i == 0 || value > best_value) {
                best_value = value;
                best_label = label;
            }
        }

        ESP_LOGI(TAG,
                 "[Timing] dsp=%d ms classification=%d ms anomaly=%d ms",
                 result.timing.dsp,
                 result.timing.classification,
                 result.timing.anomaly);

        bool is_stress = strcmp(best_label, "stress") == 0 && best_value >= EI_CLASSIFIER_THRESHOLD;

        static bool stress_history[3] = {false, false, false};
        static int stress_hist_idx = 0;
        stress_history[stress_hist_idx] = is_stress;
        stress_hist_idx = (stress_hist_idx + 1) % 3;
        bool filtered_stress = stress_history[0] && stress_history[1] && stress_history[2];

        edgeai_mqtt_publish_state(best_label, best_value, filtered_stress);

        if (filtered_stress) {
            ESP_LOGW(TAG, "State: STRESS confirmed (%.3f)", best_value);
        } else {
            ESP_LOGI(TAG, "State: %s (%.3f)", best_label, best_value);
        }

        current_accumulated = 0;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "EdgeAI CoolBeeBox Edge Impulse initialization");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    edgeai_mqtt_app_start();

    i2s_init();
    xTaskCreate(audio_inference_task, "audio_inference_task", 8192, NULL, 5, NULL);
}
