/**
 * @file encoder.c
 * @brief Encoder task - converts ASCII lyrics to Morse code.
 * @author Marko Varga
 */

//--------------------------------- INCLUDES ----------------------------------
#include "encoder.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static const char* char_to_morse(char c);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static const char *lyrics =
    "So when you're near me, darling\n"
    "Can't you hear me, S.O.S.?\n"
    "And the love you gave me\n"
    "Nothing else can save me, S.O.S.";

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

void encoder_task(void *params)
{
    TickType_t last_wake_time;
    morse_msg_t msg;
    BaseType_t status;
    static uint32_t char_index = 0;
    uint32_t lyrics_len = strlen(lyrics);

    (void)params;

    last_wake_time = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&last_wake_time, PERIOD_ENCODER_MS);

        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
            printf("[ENCODER] >>> Start iteration (t=%d ms)\r\n", (int)xTaskGetTickCount());
            xSemaphoreGive(mutex);
        }

        uint32_t dummy = 0;
        for (volatile uint32_t i = 0; i < 5000000; i++)
        {
            dummy = i % 7;
        }

        if (char_index >= lyrics_len)
        {
            char_index = 0;
        }

        char current_char = lyrics[char_index];
        const char *morse = char_to_morse(current_char);

        if (morse != NULL)
        {
            msg.size = strlen(morse);
            strncpy(msg.msg_morse, morse, MAX_MORSE_LEN - 1);
            msg.msg_morse[MAX_MORSE_LEN - 1] = '\0';

            status = xQueueSend(data_queue, &msg, 0);

            if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
            {
                if (status == pdPASS)
                {
                    stats.encoded_count++;
                    printf("[ENCODER]     '%c' -> \"%s\"\r\n",
                           isprint(current_char) ? current_char : ' ', morse);
                }
                else
                {
                    stats.queue_full_count++;
                    printf("[ENCODER]     WARNING: Queue full!\r\n");
                }
                xSemaphoreGive(mutex);
            }
        }

        for (volatile uint32_t i = 0; i < 5000000; i++)
        {
            dummy = i % 7;
        }

        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
            printf("[ENCODER] <<< End iteration, going to sleep %d\r\n", dummy);
            xSemaphoreGive(mutex);
        }

        char_index++;
    }
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static const char* char_to_morse(char c)
{
    c = toupper(c);

    if (c >= 'A' && c <= 'Z')
    {
        return morse_table[c - 'A'];
    }
    else if (c >= '0' && c <= '9')
    {
        return morse_digits[c - '0'];
    }
    else
    {
        switch (c)
        {
        case ' ':
        case '\n':
            return "/";
        case '.':
            return ".-.-.-";
        case ',':
            return "--..--";
        case '?':
            return "..--..";
        case '\'':
            return ".----.";
        default:
            return NULL;
        }
    }
}

//---------------------------- INTERRUPT HANDLERS -----------------------------
