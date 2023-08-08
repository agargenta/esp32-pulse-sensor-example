#include <stdio.h>
#include <assert.h>
#include <esp_err.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
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
        pulse_sensor_rate_t rate = pulse_sensor_get_total_rate(pulse_sensor);
        ESP_LOGI(TAG,
                 "Pulse sensor totals: pulses=%llu, duration=%llu (usec), cycles=%lu, rate=%.3Lf pulses/second",
                 pulse_sensor_get_total_pulses(pulse_sensor),
                 pulse_sensor_get_total_duration(pulse_sensor),
                 pulse_sensor_get_total_cycles(pulse_sensor),
                 rate.duration == 0
                     ? 0.0L
                     : rate.pulses / ((long double)rate.duration / 1000000));
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

static void pulse_sensor_rate_reporter(void *args)
{
    const pulse_sensor_h pulse_sensor = (pulse_sensor_h)args;
    while (true)
    {
        pulse_sensor_rate_t rate = pulse_sensor_get_current_rate(pulse_sensor);
        if (rate.pulses > 0)
        {
            ESP_LOGI(TAG, "Pulse sensor rate: %.3Lf pulses/second",
                     rate.pulses / ((long double)rate.duration / 1000000));
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    pulse_sensor_notifications_queue = xQueueCreate(10, sizeof(pulse_sensor_notification_t));
    assert(pulse_sensor_notifications_queue != NULL);

    assert(xTaskCreate(pulse_sensor_notifications_consumer,
                       "pulse sensor notification consumer task",
                       2048, NULL, 1, NULL) == pdPASS);

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    pulse_sensor_config_t pulse_sensor_config = {
        .gpio_num = GPIO_NUM_16,
        .min_cycle_pulses = 20,
        .notification_queue = pulse_sensor_notifications_queue};
    pulse_sensor_h pulse_sensor = NULL;
    ESP_ERROR_CHECK(pulse_sensor_open(pulse_sensor_config, &pulse_sensor));

    assert(xTaskCreate(pulse_sensor_rate_reporter,
                       "pulse sensor rate reporter task",
                       2048, (void *)pulse_sensor, 1, NULL) == pdPASS);

    assert(xTaskCreate(pulse_sensor_totals_reporter,
                       "pulse sensor totals reporter task",
                       2048, (void *)pulse_sensor, 1, NULL) == pdPASS);
}
