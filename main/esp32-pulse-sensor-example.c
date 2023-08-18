#include <stdio.h>
#include <assert.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include "pulse_sensor.h"

static const char *TAG = "pulse-sensor-example";
static QueueHandle_t pulse_sensor_notifications_queue;

static void pulse_sensor_notifications_consumer(void *args)
{
    pulse_sensor_notification_t m;
    ESP_LOGD(TAG, "Consuming pulse notification messages");
    while (true)
    {
        if (!xQueueReceive(pulse_sensor_notifications_queue, &m, portMAX_DELAY))
        {
            ESP_LOGW(TAG, "Timeout while waiting on pulse message a notification.");
            break; // This should never happen
        }
        switch (m.type)
        {
        case PULSE_SENSOR_CYCLE_STARTED:
            ESP_LOGI(TAG, "Cycle started at %lld", m.timestamp);
            break;
        case PULSE_SENSOR_CYCLE_STOPPED:
            ESP_LOGI(TAG, "Cycle stopped at %lld", m.timestamp);
            break;
        case PULSE_SENSOR_CYCLE_IGNORED:
            ESP_LOGI(TAG, "Cycle ignored at %lld", m.timestamp);
            break;
        default:
            ESP_LOGW(TAG, "Unexpected notification type: %d. Ignoring.", m.type);
        }
    }
}

static void pulse_sensor_data_reporter(void *args)
{
    const pulse_sensor_t pulse_sensor = (pulse_sensor_t)args;
    pulse_sensor_data_t data;
    int64_t t = 0, c = 0;
    while (true)
    {
        ESP_ERROR_CHECK(pulse_sensor_get_data(pulse_sensor, &data));
        float rate = pulse_sensor_get_current_rate(&data);
        if (rate > 0)
        {
            ESP_LOGI(TAG, "Current: %.3f pulses/second", rate);
        }
        const int64_t now = esp_timer_get_time();
        if (now - c > 1000000) // 1 second
        {
            c = now;
            if (pulse_sensor_is_in_cycle(&data))
            {
                ESP_LOGI(TAG,
                         "Cycle: pulses=%llu, duration=%llu (µs), rate=%.3f pulses/second",
                         data.current_cycle_pulses,
                         pulse_sensor_get_current_cycle_duration(&data),
                         pulse_sensor_get_current_cycle_rate(&data));
            }
        }
        if (now - t > 5000000) // 5 seconds
        {
            t = now;
            ESP_LOGI(TAG,
                     "Total: pulses=%llu, duration=%llu (µs), cycles=%lu"
                     ", partial_cycles=%lu, rate=%.3f pulses/second",
                     data.total_pulses,
                     data.total_duration,
                     data.cycles,
                     data.partial_cycles,
                     pulse_sensor_get_total_rate(&data));
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // To debug, use 'make menuconfig' to set default Log level to DEBUG, then uncomment:
    // esp_log_level_set("pulse-sensor", ESP_LOG_DEBUG);

    pulse_sensor_notifications_queue = xQueueCreate(10, sizeof(pulse_sensor_notification_t));
    assert(pulse_sensor_notifications_queue != NULL);

    assert(xTaskCreate(pulse_sensor_notifications_consumer,
                       "pulse sensor notification consumer task",
                       2048, NULL, 1, NULL) == pdPASS);

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    pulse_sensor_config_t pulse_sensor_config = PULSE_SENSOR_CONFIG_DEFAULT();
    pulse_sensor_config.gpio_num = GPIO_NUM_16;
    pulse_sensor_config.min_cycle_pulses = 20;
    pulse_sensor_config.notification_queue = pulse_sensor_notifications_queue;
    pulse_sensor_t pulse_sensor = NULL;
    ESP_ERROR_CHECK(pulse_sensor_open(&pulse_sensor_config, &pulse_sensor));

    assert(xTaskCreate(pulse_sensor_data_reporter,
                       "pulse sensor data reporter task",
                       2048, (void *)pulse_sensor, 1, NULL) == pdPASS);
}
