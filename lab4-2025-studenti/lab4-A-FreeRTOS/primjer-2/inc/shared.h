/**
 * @file shared.h
 * @brief Shared data types and declarations for Lab 4.
 * @author Marko Varga
 */

#ifndef __SHARED_H__
#define __SHARED_H__

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//---------------------------------- MACROS -----------------------------------
#define QUEUE_LENGTH (10)
#define MAX_MORSE_LEN (8)

//-------------------------------- DATA TYPES ---------------------------------
typedef struct
{
    uint8_t size;
    char msg_morse[MAX_MORSE_LEN];
} morse_msg_t;

typedef struct
{
    uint32_t encoded_count;
    uint32_t decoded_count;
    uint32_t queue_full_count;
} statistics_t;

//------------------------------- GLOBAL DATA ---------------------------------
extern QueueHandle_t data_queue;
extern SemaphoreHandle_t mutex;
extern statistics_t stats;

#ifdef __cplusplus
}
#endif

#endif // __SHARED_H__
