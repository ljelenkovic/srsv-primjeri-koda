/**
* @file decoder.c
* @brief Decoder task - converts Morse code to ASCII.
* @author Marko Varga
*/

//--------------------------------- INCLUDES ----------------------------------
#include "decoder.h"
#include "shared.h"
#include <stdio.h>
#include <string.h>

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static char morse_to_char(const char *morse);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static const char *morse_table[] = {
    ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", // A-H
    "..",   ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.", // I-P
    "--.-", ".-.",  "...",  "-",    "..-",  "...-", ".--",  "-..-", // Q-X
    "-.--", "--.."                                                   // Y-Z
};

static const char *morse_digits[] = {
    "-----", ".----", "..---", "...--", "....-",  // 0-4
    ".....", "-....", "--...", "---..", "----."   // 5-9
};

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

void decoder_task(void *params)
{
    TickType_t last_wake_time;
    morse_msg_t received_msg;
    BaseType_t status;
    int decoder_id;

    decoder_id = (int)(uint32_t)params;

    last_wake_time = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&last_wake_time, PERIOD_DECODER_MS);

        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
            printf("[DECODER-%d] >>> Start iteration (t=%d ms)\r\n",
                    decoder_id, (int)xTaskGetTickCount());
            xSemaphoreGive(mutex);
        }

        volatile uint32_t dummy = 0;
        for (volatile uint32_t i = 0; i < 3000000; i++)
        {
            dummy = i % 7;
        }

        status = xQueueReceive(data_queue, &received_msg, 0);

        if (status == pdPASS)
        {
            char decoded_char = morse_to_char(received_msg.msg_morse);

            if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
            {
                printf("[DECODER-%d]     \"%s\" -> '%c'\r\n",
                        decoder_id,
                        received_msg.msg_morse,
                        decoded_char);
                stats.decoded_count++;
                xSemaphoreGive(mutex);
            }
        }
        else
        {
            if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
            {
                printf("[DECODER-%d]     Queue empty, no data\r\n", decoder_id);
                xSemaphoreGive(mutex);
            }
        }

        for (volatile uint32_t i = 0; i < 3000000; i++)
        {
            dummy = i % 7;
        }
        (void)dummy;

        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
            printf("[DECODER-%d] <<< End iteration, going to sleep\r\n", decoder_id);
            xSemaphoreGive(mutex);
        }
    }
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static char morse_to_char(const char *morse)
{
    if (strcmp(morse, "/") == 0)
    {
        return ' ';
    }

    for (int i = 0; i < 26; i++)
    {
        if (strcmp(morse, morse_table[i]) == 0)
        {
            return 'A' + i;
        }
    }

    for (int i = 0; i < 10; i++)
    {
        if (strcmp(morse, morse_digits[i]) == 0)
        {
            return '0' + i;
        }
    }

    if (strcmp(morse, ".-.-.-") == 0) return '.';
    if (strcmp(morse, "--..--") == 0) return ',';
    if (strcmp(morse, "..--..") == 0) return '?';
    if (strcmp(morse, ".----.") == 0) return '\'';

    return '?';
}

//---------------------------- INTERRUPT HANDLERS -----------------------------
