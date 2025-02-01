#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define REQUEST_PIN_OUTPUT 4  // GPIO pin to send request signal
#define RESPONSE_PIN_INPUT 5  // GPIO pin to receive completion signal

static const char *TAG = "Request_Generator";

static int64_t shortest_response_time = INT64_MAX;
static int64_t longest_response_time = 0;
static int64_t total_response_time = 0;
static int response_count = 0;

void request_generator_task(void *arg) {
    while (1) {
        int64_t request_time = esp_timer_get_time();
        ESP_LOGI(TAG, "Request generated at: %lld us", request_time);

        gpio_set_level(REQUEST_PIN_OUTPUT, 1);

        while (gpio_get_level(RESPONSE_PIN_INPUT) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        int64_t response_time = esp_timer_get_time();
        ESP_LOGI(TAG, "Response received at: %lld us", response_time);

        int64_t response_duration = response_time - request_time;
        ESP_LOGI(TAG, "Response time: %lld us", response_duration);

        if (response_duration > 9999) {
            if (response_duration < shortest_response_time) {
                shortest_response_time = response_duration;
            }
            if (response_duration > longest_response_time) {
                longest_response_time = response_duration;
            }
            total_response_time += response_duration;
            response_count++;
        } 

        gpio_set_level(REQUEST_PIN_OUTPUT, 0);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void stats_printer_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(15000));

        int64_t average_response_time = response_count > 0 ? total_response_time / response_count : 0;
        ESP_LOGI(TAG, "========== Statistics ==========");
        ESP_LOGI(TAG, "Shortest Response Time: %lld us", shortest_response_time);
        ESP_LOGI(TAG, "Longest Response Time: %lld us", longest_response_time);
        ESP_LOGI(TAG, "Average Response Time: %lld us", average_response_time);
        ESP_LOGI(TAG, "Total Requests: %d", response_count);
        ESP_LOGI(TAG, "================================");

        // shortest_response_time = INT64_MAX;
        // longest_response_time = 0;
        // total_response_time = 0;
        // response_count = 0;
    }
}

void app_main(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << REQUEST_PIN_OUTPUT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << RESPONSE_PIN_INPUT);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    xTaskCreate(request_generator_task, "request_gen_task", 2048, NULL, 10, NULL);
    xTaskCreate(stats_printer_task, "stats_printer_task", 2048, NULL, 5, NULL);
}
