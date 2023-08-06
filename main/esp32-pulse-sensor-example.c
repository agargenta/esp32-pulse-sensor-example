#include "pulse_sensor.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <esp_err.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>

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
            ESP_LOGI(TAG, "Pulse cycle started at %lld", m.timestamp);
            break;
        case PULSE_SENSOR_CYCLE_STOPPED:
            ESP_LOGI(TAG, "Pulse cycle stopped at %lld; pulses: %llu, duration: %llu (usec)",
                     m.timestamp, m.pulses, m.duration);
            break;
        default:
            ESP_LOGW(TAG, "Unexpected pulse cycle notification type: %d. Ignoring.", m.type);
        }
    }
}

static void pulse_sensor_totals_reporter(void *args)
{
    const pulse_sensor_h pulse_sensor = (pulse_sensor_h)args;
    while (true)
    {
        ESP_LOGI(TAG, "Pulse sensor totals: pulses=%llu, duration=%llu (usec), cycles=%lu",
                 pulse_sensor_get_total_pulses(pulse_sensor),
                 pulse_sensor_get_total_duration(pulse_sensor),
                 pulse_sensor_get_total_cycles(pulse_sensor));
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    pulse_sensor_notifications_queue = xQueueCreate(10, sizeof(pulse_sensor_notification_t));
    if (!pulse_sensor_notifications_queue)
    {
        // handle error
    }
    BaseType_t r = xTaskCreate(pulse_sensor_notifications_consumer,
                               "pulse sensor notification consumer task",
                               2048, NULL, 1, NULL);
    if (r != pdPASS)
    {
        // handle error
    }

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    pulse_sensor_config_t pulse_sensor_config = {
        .gpio_num = GPIO_NUM_16,
        .min_cycle_pulses = 20,
        .notification_queue = pulse_sensor_notifications_queue};
    pulse_sensor_h pulse_sensor = NULL;
    ESP_ERROR_CHECK(pulse_sensor_open(pulse_sensor_config, &pulse_sensor));

    r = xTaskCreate(pulse_sensor_totals_reporter,
                    "pulse sensor totals reporter task",
                    2048, (void *)pulse_sensor, 1, NULL);
    if (r != pdPASS)
    {
        // handle error
    }
}
