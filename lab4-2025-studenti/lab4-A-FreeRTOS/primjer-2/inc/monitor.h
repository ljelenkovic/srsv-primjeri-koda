/**
 * @file monitor.h
 * @brief See the source file.
 * @author Marko Varga
 */

#ifndef __MONITOR_H__
#define __MONITOR_H__

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "shared.h"

//---------------------------------- MACROS -----------------------------------
#define PRIORITY_MONITOR (tskIDLE_PRIORITY + 2)
#define PERIOD_MONITOR_MS pdMS_TO_TICKS(1000)

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------
/** @brief Prints system status periodically. */
void monitor_task(void *params);

#ifdef __cplusplus
}
#endif

#endif // __MONITOR_H__
