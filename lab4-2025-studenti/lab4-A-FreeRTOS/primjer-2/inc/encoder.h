/**
 * @file encoder.h
 * @brief See the source file.
 * @author Marko Varga
 */

#ifndef __ENCODER_H__
#define __ENCODER_H__

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "shared.h"

//---------------------------------- MACROS -----------------------------------
#define PRIORITY_ENCODER (tskIDLE_PRIORITY + 1)
#define PERIOD_ENCODER_MS pdMS_TO_TICKS(200)

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------
/** @brief Encodes ASCII to Morse code. */
void encoder_task(void *params);

#ifdef __cplusplus
}
#endif

#endif // __ENCODER_H__
