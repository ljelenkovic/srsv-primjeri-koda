#include <stdio.h>
#include <pthread.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define QUEUELEN 10
#define SENZORINTERVAL pdMS_TO_TICKS(1000)
#define CITACINTERVAL pdMS_TO_TICKS(900)
#define OBRADA pdMS_TO_TICKS(500)

QueueHandle_t xDataQueue;
SemaphoreHandle_t xConsoleMutex;
SemaphoreHandle_t xCounterMutex;
int zadnjiObraden;

void printfWithMutex(const char* toPrint, int data) {
    xSemaphoreTake(xConsoleMutex, portMAX_DELAY);
    printf(toPrint, data);
    xSemaphoreGive(xConsoleMutex);

}

void vSenzor(void *pvParameters) {
    int counter = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = SENZORINTERVAL;

    for (;;) {
        xSemaphoreTake(xCounterMutex, portMAX_DELAY);
        if (zadnjiObraden != counter)
            printf("Senzor - nije uspjesno obraden podatak %d\n", counter);
        xSemaphoreGive(xCounterMutex);
        counter %= (1 << 10);
        counter++;
        
        if (xQueueSend(xDataQueue, &counter, 0) == pdPASS) {
            printfWithMutex("Senzor - podatak %d ocitan i poslan.\n", counter);
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

void vCitac(void *pvParameters) {
    int receivedData;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = CITACINTERVAL;

    for (;;) {
        if (xQueueReceive(xDataQueue, &receivedData, 0) == pdPASS) {
            printfWithMutex("Citac - podatak %d ocitan i pokrecem obradu\n", receivedData);
            vTaskDelay(OBRADA);
            printfWithMutex("Citac - gotova obrada podatka %d\n", receivedData);
            xSemaphoreTake(xCounterMutex, portMAX_DELAY);
            zadnjiObraden = receivedData;
            xSemaphoreGive(xCounterMutex);
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

int main(void) {
    zadnjiObraden = 0;
    xDataQueue = xQueueCreate(QUEUELEN, sizeof(int));
    xConsoleMutex = xSemaphoreCreateMutex();
    xCounterMutex = xSemaphoreCreateMutex();

    if (xDataQueue != NULL && xConsoleMutex != NULL) {
        xTaskCreate(vSenzor, "Sensor", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
        xTaskCreate(vCitac, "Obrada", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

        printf("Pokrecem program\n");
        vTaskStartScheduler();
    }

    for (;;);
    return 0;
}
