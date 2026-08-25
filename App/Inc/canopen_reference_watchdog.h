/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_REFERENCE_WATCHDOG_H
#define CANOPEN_REFERENCE_WATCHDOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the optional IWDG and reset the dual-rate progress state. */
void CANopenReferenceWatchdog_Init(void);

/** Record one TIM7 tick; safe to call from the 1 ms interrupt callback. */
void CANopenReferenceWatchdog_TickISR(void);

/** Observe timer progress and refresh IWDG only from the mainline. */
void CANopenReferenceWatchdog_Process(void);

/** Return the number of timer ticks observed by the mainline. */
uint32_t CANopenReferenceWatchdog_MainlineTicks(void);

/** Return reset flags captured before HAL initialization. */
uint32_t CANopenReferenceWatchdog_ResetFlags(void);

/** Persist a fatal-fault code in RTC backup registers before halting.
 *
 * The code survives a warm reset so service tooling can recover the cause of
 * an unrecoverable Error_Handler stop even when the independent watchdog is
 * disabled. Safe to call from any context before interrupts are disabled. */
void CANopenReferenceWatchdog_RecordFatalFault(uint32_t code);

/** Return the fatal-fault code recorded by the previous run, or 0.
 *
 * The stored code is consumed once during initialization; subsequent calls
 * return the same value until the next power-on or recorded fault. */
uint32_t CANopenReferenceWatchdog_PreviousFault(void);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_REFERENCE_WATCHDOG_H */
