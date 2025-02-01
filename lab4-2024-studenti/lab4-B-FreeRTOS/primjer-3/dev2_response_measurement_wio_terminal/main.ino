#include <Seeed_Arduino_FreeRTOS.h>
#include "TFT_eSPI.h"
#include "Free_Fonts.h"

// Pin definitions
#define REQUEST_PIN BCM23  // Pin 16 output to ESP32
#define RESPONSE_PIN BCM24 // Pin 18 input from ESP32
#define BUTTON_STATS WIO_KEY_C

// Display/Testing configuration
#define MAX_SAMPLES 1000
#define MAX_LINES 12
#define LINE_HEIGHT 20
#define TEXT_START_Y 40

#define MIN_INTERVAL_MS 300 // Minimum time between requests
#define MAX_INTERVAL_MS 500 // Maximum time between requests
#define TIMEOUT_MS 204      // Timeout: 200ms processing + 20ms margin

// Display setup
TFT_eSPI tft;
int currentLine = TEXT_START_Y;

char buffer[50];

// Task handles
TaskHandle_t Handle_requestTask;
TaskHandle_t Handle_displayTask;
TaskHandle_t Handle_timeoutTask;

// Shared state
volatile bool simulationRunning = true;
volatile uint32_t requestTime = 0;
volatile bool waitingResponse = false;
volatile bool gotResponse = false;

// Statistics storage
struct Statistics
{
    uint32_t minResponse = UINT32_MAX;
    uint32_t maxResponse = 0;
    uint32_t totalResponse = 0;
    uint32_t count = 0;
    uint32_t responseTimes[MAX_SAMPLES];
    uint32_t timeouts = 0;
} stats;

void printLine(const char *text, uint16_t color)
{
    tft.fillRect(0, currentLine, 320, LINE_HEIGHT, TFT_BLACK);
    tft.setTextColor(color);
    tft.drawString(text, 10, currentLine);

    currentLine += LINE_HEIGHT;
    if (currentLine >= (TEXT_START_Y + (MAX_LINES * LINE_HEIGHT)))
    {
        currentLine = TEXT_START_Y;
        tft.fillRect(0, TEXT_START_Y, 320, 320 - TEXT_START_Y, TFT_BLACK);
    }
}

void responseISR()
{
    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    if (waitingResponse)
    {
        uint32_t responseTime = micros() - requestTime;

        if (responseTime < stats.minResponse)
            stats.minResponse = responseTime;
        if (responseTime > stats.maxResponse)
            stats.maxResponse = responseTime;
        stats.totalResponse += responseTime;

        if (stats.count < MAX_SAMPLES)
        {
            stats.responseTimes[stats.count] = responseTime;
            stats.count++;
        }

        waitingResponse = false;
        gotResponse = true;
        snprintf(buffer, sizeof(buffer), "Got response @%lu ms", millis());
        printLine(buffer, TFT_GREEN);
    }
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

static void timeoutTask(void *pvParameters)
{
    while (simulationRunning)
    {
        if (waitingResponse && !gotResponse)
        {
            if ((micros() - requestTime) >= (TIMEOUT_MS * 1000))
            {
                taskENTER_CRITICAL();
                stats.timeouts++;
                waitingResponse = false;
                taskEXIT_CRITICAL();
                Serial.println("timeout!");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
    vTaskDelete(NULL);
}

static void requestTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t randomInterval;

    while (simulationRunning)
    {
        if (!waitingResponse)
        {
            uint32_t currentTime = millis();

            digitalWrite(REQUEST_PIN, HIGH);
            delayMicroseconds(100);
            digitalWrite(REQUEST_PIN, LOW);
            requestTime = micros();
            waitingResponse = true;
            gotResponse = false;

            snprintf(buffer, sizeof(buffer), "Send Req @%lu ms", currentTime);
            printLine(buffer, TFT_YELLOW);

            // Generate random interval for next request
            randomInterval = random(MIN_INTERVAL_MS, MAX_INTERVAL_MS);
            vTaskDelay(pdMS_TO_TICKS(randomInterval));
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Give other tasks a chance to run
    }
    vTaskDelete(NULL);
}

static void displayTask(void *pvParameters)
{
    while (1)
    {
        if (digitalRead(BUTTON_STATS) == LOW)
        {
            // Stop simulation
            simulationRunning = false;
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for tasks to finish

            // Display final statistics
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_WHITE);

            char buffer[50];
            tft.drawString("Final Statistics:", 10, 10);

            snprintf(buffer, sizeof(buffer), "Min: %lu ms", stats.minResponse / 1000);
            tft.drawString(buffer, 10, 40);

            snprintf(buffer, sizeof(buffer), "Max: %lu ms", stats.maxResponse / 1000);
            tft.drawString(buffer, 10, 70);

            if (stats.count > 0)
            {
                snprintf(buffer, sizeof(buffer), "Avg: %lu ms", (stats.totalResponse / stats.count) / 1000);
                tft.drawString(buffer, 10, 100);

                snprintf(buffer, sizeof(buffer), "Total Requests: %lu", stats.count);
                tft.drawString(buffer, 10, 130);

                snprintf(buffer, sizeof(buffer), "Timeouts: %lu", stats.timeouts);
                tft.drawString(buffer, 10, 160);

                float timeoutPercentage = (float)stats.timeouts / stats.count * 100;
                snprintf(buffer, sizeof(buffer), "Timeout %%: %.1f%%", timeoutPercentage);
                tft.drawString(buffer, 10, 190);
            }

            tft.drawString("Test Complete!", 10, 220);

            // End task
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void setup()
{
    Serial.begin(115200);
    vNopDelayMS(1000);
    while (!Serial)
        ;
    randomSeed(analogRead(0));

    // Initialize display
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(FSS9);
    tft.drawString("Response Time Test", 10, 10);

    // Setup GPIO
    pinMode(REQUEST_PIN, OUTPUT);
    pinMode(RESPONSE_PIN, INPUT);
    pinMode(BUTTON_STATS, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(RESPONSE_PIN), responseISR, RISING);

    // Create tasks
    xTaskCreate(requestTask, "Request Task", 256, NULL, tskIDLE_PRIORITY + 3, &Handle_requestTask);
    xTaskCreate(timeoutTask, "Timeout Task", 256, NULL, tskIDLE_PRIORITY + 2, &Handle_timeoutTask);
    xTaskCreate(displayTask, "Display Task", 256, NULL, tskIDLE_PRIORITY + 1, &Handle_displayTask);

    vTaskStartScheduler();
}

void loop()
{
    // Nothing here - FreeRTOS scheduler handles everything
}