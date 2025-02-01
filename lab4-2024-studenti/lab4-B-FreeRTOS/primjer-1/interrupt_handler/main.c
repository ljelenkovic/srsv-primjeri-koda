#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define REQUEST_PIN_INPUT 4  // GPIO pin to receive request signal
#define RESPONSE_PIN_OUTPUT 5 // GPIO pin to send response signal

#define WORKLOAD_INTENSITY 7

static const char *TAG = "Request_Handler";

static void IRAM_ATTR request_handler_isr(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR((TaskHandle_t)arg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void request_handler_task(void *arg) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int64_t request_received_time = esp_timer_get_time();
        ESP_LOGI(TAG, "Request received at: %lld us", request_received_time);

        vTaskDelay(pdMS_TO_TICKS(100));

        int64_t processing_done_time = esp_timer_get_time();
        ESP_LOGI(TAG, "Processing done at: %lld us", processing_done_time);
        
        gpio_set_level(RESPONSE_PIN_OUTPUT, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(RESPONSE_PIN_OUTPUT, 0);
    }
}

void workload_task(void *arg) {
    int task_id = (int)arg;

    while (1) {

        ESP_LOGI(TAG, "Workload task %d is running...", task_id);
        for (volatile int i = 0; i < 1000000; i++) {
            
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << REQUEST_PIN_INPUT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << RESPONSE_PIN_OUTPUT);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    TaskHandle_t handler_task_handle;
    xTaskCreate(request_handler_task, "request_handler_task", 2048, NULL, 10, &handler_task_handle);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(REQUEST_PIN_INPUT, request_handler_isr, (void *)handler_task_handle);

    for (int i = 0; i < WORKLOAD_INTENSITY; i++) {
        char task_name[20];
        snprintf(task_name, sizeof(task_name), "workload_task_%d", i + 1);
        xTaskCreate(workload_task, task_name, 2048, (void *)i, configMAX_PRIORITIES - 1, NULL);
    }
}
