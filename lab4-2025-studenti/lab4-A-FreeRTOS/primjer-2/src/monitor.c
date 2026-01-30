/**
 * @file monitor.c
 * @brief Monitor task - periodically prints system status.
 * @author Marko Varga
 */

//--------------------------------- INCLUDES ----------------------------------
#include "monitor.h"
#include "shared.h"
#include <stdio.h>

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static void print_stats(void);

//------------------------- STATIC DATA & CONSTANTS ---------------------------

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

void monitor_task(void *params)
{
    TickType_t last_wake_time;

    (void)params;

    last_wake_time = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&last_wake_time, PERIOD_MONITOR_MS);

        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
            printf("[MONITOR] >>> Start iteration (t=%d ms)\r\n", (int)xTaskGetTickCount());
            xSemaphoreGive(mutex);
        }

        volatile uint32_t dummy = 0;
        for (volatile uint32_t i = 0; i < 2000000; i++)
        {
            dummy = i % 7;
        }

        print_stats();

        for (volatile uint32_t i = 0; i < 2000000; i++)
        {
            dummy = i % 7;
        }
        (void)dummy;

        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
            printf("[MONITOR] <<< End iteration, going to sleep\r\n");
            xSemaphoreGive(mutex);
        }
    }
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void print_stats(void)
{
    statistics_t local_stats;
    UBaseType_t queue_items;

    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
    {
        local_stats = stats;
        queue_items = uxQueueMessagesWaiting(data_queue);

        printf("\r\n----------------------------------------\r\n");
        printf("System status (t=%d ms)\r\n", (int)xTaskGetTickCount());
        printf("  Chars encoded: %d\r\n", (int)local_stats.encoded_count);
        printf("  Chars decoded: %d\r\n", (int)local_stats.decoded_count);
        printf("  Dropped (queue full): %d\r\n", (int)local_stats.queue_full_count);
        printf("  Messages in queue: %d\r\n", (int)queue_items);
        printf("----------------------------------------\r\n\r\n");

        xSemaphoreGive(mutex);
    }
}

//---------------------------- INTERRUPT HANDLERS -----------------------------
