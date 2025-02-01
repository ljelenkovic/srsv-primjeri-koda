#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"

// Three different processing delays
#define PROCESSING_200_MS 200
#define PROCESSING_500_MS 500
#define PROCESSING_1000_MS 1000

// Choose the processing delay here:
#define PROCCESING_DELAY_MS PROCESSING_200_MS

// Choose the number of dummy taks:
#define DUMMY_TASKS_NUMBER 10

#define INPUT_PIN 11  // GPIO pin receiving requests from Wio
#define OUTPUT_PIN 10 // GPIO pin sending responses to Wio

static const char *TAG = "response_handler";
static QueueHandle_t gpio_evt_queue = NULL;

// Structure to pass timing info
typedef struct
{
    uint32_t trigger_time; // Time when request was received
} gpio_event_t;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    gpio_event_t evt = {
        .trigger_time = esp_timer_get_time() // Microsecond timer
    };
    xQueueSendFromISR(gpio_evt_queue, &evt, NULL);
}

// Task that handles the processing and response
static void gpio_handler_task(void *arg)
{
    gpio_event_t evt;

    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &evt, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Request received @%u!", (unsigned int)evt.trigger_time);

            // Busy-waiting instead of vTaskDelay
            uint32_t start_time = esp_timer_get_time();                  // Get start time in microseconds
            uint32_t end_time = start_time + PROCESSING_200_MS * 1000; // Convert ms to us

            // Busy-wait loop to simulate processing
            while (esp_timer_get_time() < end_time)
            {
            }

            // Send response pulse
            gpio_set_level(OUTPUT_PIN, 1);
            // Busy-wait for 100us pulse (replace vTaskDelay with busy-wait loop)
            uint32_t pulse_end_time = esp_timer_get_time() + 100; // 100us pulse
            while (esp_timer_get_time() < pulse_end_time)
            {
                // No operation, just wait for 100us to pass
            }
            gpio_set_level(OUTPUT_PIN, 0);
            uint32_t end_time_proc = esp_timer_get_time(); 
            ESP_LOGI(TAG, "Request processed in %.4f ms", ((float)end_time_proc-(float)start_time)/1000.);
        }
    }
}

// Dummy task handler to simulate system load
static void dummy_handler_task(void *arg)
{
    while (1)
    {
        // simulate load for 1 second
        uint32_t start_time = esp_timer_get_time();
        uint32_t end_time = start_time + PROCESSING_200_MS * 1000;
        while (esp_timer_get_time() < end_time)
        {
            // busy waiting, just wait for 1s to pass
        }

        // task is now in blocked state, gives CPU to other tasks 
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing GPIOs for request-response measurement");

    // Create queue for passing GPIO events
    gpio_evt_queue = xQueueCreate(10, sizeof(gpio_event_t));

    // Configure output pin
    gpio_config_t output_conf = {
        .pin_bit_mask = (1ULL << OUTPUT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&output_conf);

    // Configure input pin with interrupt
    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << INPUT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE // Trigger on rising edge
    };
    gpio_config(&input_conf);

    gpio_install_isr_service(0);

    // Add handler for GPIO interrupt
    gpio_isr_handler_add(INPUT_PIN, gpio_isr_handler, NULL);

    // Create task to handle GPIO events
    xTaskCreate(gpio_handler_task, "gpio_handler", 2048, NULL, 10, NULL);

    // Create dummy tasks to simulate load
    for (int i = 0; i < DUMMY_TASKS_NUMBER; i++)
        xTaskCreate(dummy_handler_task, "dummy_handler", 2048, NULL, rand() % 10, NULL);

    ESP_LOGI(TAG, "GPIO setup complete, waiting for requests...");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}