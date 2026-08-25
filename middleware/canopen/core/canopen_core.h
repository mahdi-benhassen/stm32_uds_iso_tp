/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_MIDDLEWARE_CORE_CANOPEN_CORE_H
#define CANOPEN_MIDDLEWARE_CORE_CANOPEN_CORE_H

#include <stdint.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include "CO_app_STM32.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the one CANopenNode instance owned by the STM32 runtime. */
int canopen_core_init(CANopenNodeSTM32 *instance);

/** Execute non-real-time CANopen work: NMT, heartbeat, SDO, and reset commands. */
void canopen_core_process(void);

/** Execute bounded, fixed-cycle SYNC/RPDO/application/TPDO work. */
void canopen_core_process_cycle(void);

/** Force profile hardware hooks into their conservative safe state. */
void canopen_core_force_safe_state(void);

/** Return a compact diagnostic status: 0 when initialized, negative otherwise. */
int canopen_core_status(void);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_MIDDLEWARE_CORE_CANOPEN_CORE_H */
