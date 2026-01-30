/**
 * @file decoder.h
 * @brief See the source file.
 * @author Marko Varga
 */

#ifndef __DECODER_H__
#define __DECODER_H__

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "shared.h"

//---------------------------------- MACROS -----------------------------------
#define PRIORITY_DECODER (tskIDLE_PRIORITY + 1)
#define PERIOD_DECODER_MS pdMS_TO_TICKS(100)

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------
/** @brief Decodes Morse code to ASCII. */
void decoder_task(void *params);

#ifdef __cplusplus
}
#endif

#endif // __DECODER_H__
