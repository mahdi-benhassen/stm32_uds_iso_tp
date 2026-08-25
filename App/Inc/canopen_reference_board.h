/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_REFERENCE_BOARD_H
#define CANOPEN_REFERENCE_BOARD_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Configure board outputs in a safe, de-energized state before CAN startup. */
void CANopenReferenceBoard_InitSafe(void);

/** Control the physical CAN transceiver standby/enable circuit when the board defines one. */
void CANopenReferenceBoard_SetCanTransceiverEnabled(bool enabled);

/** Force transceiver, I/O, power stage, and diagnostics into their fail-safe state. */
void CANopenReferenceBoard_ForceSafe(void);

/** Called after a successful CANopen communication initialization. */
void CANopenReferenceBoard_OnCanopenReady(void);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_REFERENCE_BOARD_H */
