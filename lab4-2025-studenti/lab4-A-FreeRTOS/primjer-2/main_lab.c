/**
 * @file main_lab.c
 * @brief SRSV Lab - FreeRTOS Morse Code Demo
 * @author Marko Varga
 */

//--------------------------------- INCLUDES ----------------------------------
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "shared.h"
#include "encoder.h"
#include "decoder.h"
#include "monitor.h"

#include <stdio.h>

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------

//------------------------------- GLOBAL DATA ---------------------------------
QueueHandle_t data_queue = NULL;
SemaphoreHandle_t mutex = NULL;
statistics_t stats = {0, 0, 0};

//------------------------------ PUBLIC FUNCTIONS -----------------------------

void main_lab(void)
{
    data_queue = xQueueCreate(QUEUE_LENGTH, sizeof(morse_msg_t));
    if (data_queue == NULL)
    {
        printf("ERROR: Failed to create queue!\r\n");
        for (;;)
            ;
    }
    printf("[INIT] Queue created (capacity: %d)\r\n", QUEUE_LENGTH);

    mutex = xSemaphoreCreateMutex();
    if (mutex == NULL)
    {
        printf("ERROR: Failed to create mutex!\r\n");
        for (;;)
            ;
    }
    printf("[INIT] Mutex created\r\n");

    xTaskCreate(encoder_task,
                "Encoder",
                configMINIMAL_STACK_SIZE * 2,
                NULL,
                PRIORITY_ENCODER,
                NULL);
    printf("[INIT] Encoder task created (priority: %d)\r\n", PRIORITY_ENCODER);

    xTaskCreate(decoder_task,
                "Decoder1",
                configMINIMAL_STACK_SIZE * 2,
                (void *)1,
                PRIORITY_DECODER,
                NULL);
    printf("[INIT] Decoder-1 task created (priority: %d)\r\n", PRIORITY_DECODER);

    xTaskCreate(decoder_task,
                "Decoder2",
                configMINIMAL_STACK_SIZE * 2,
                (void *)2,
                PRIORITY_DECODER,
                NULL);
    printf("[INIT] Decoder-2 task created (priority: %d)\r\n", PRIORITY_DECODER);

    xTaskCreate(monitor_task,
                "Monitor",
                configMINIMAL_STACK_SIZE * 2,
                NULL,
                PRIORITY_MONITOR,
                NULL);
    printf("[INIT] Monitor task created (priority: %d)\r\n", PRIORITY_MONITOR);

    printf("\r\n[INIT] Starting scheduler...\r\n\r\n");

    vTaskStartScheduler();

    for (;;)
        ;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------
