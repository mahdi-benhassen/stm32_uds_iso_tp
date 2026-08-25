/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_REFERENCE_CAN_RECOVERY_H
#define CANOPEN_REFERENCE_CAN_RECOVERY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CANOPEN_REFERENCE_CAN_RUNNING = 0,
    CANOPEN_REFERENCE_CAN_WAIT_RECOVERY = 1,
    CANOPEN_REFERENCE_CAN_REINITIALIZING = 2,
    CANOPEN_REFERENCE_CAN_FAULT = 3
} CANopenReferenceCanRecoveryState;

typedef struct {
    CANopenReferenceCanRecoveryState state;
    uint32_t recovery_due_tick;
    uint32_t attempts;
    uint32_t max_attempts;
    uint32_t wait_ms;
} CANopenReferenceCanRecovery;

void CANopenReferenceCanRecovery_Init(CANopenReferenceCanRecovery *context,
                                       uint32_t wait_ms, uint32_t max_attempts);
void CANopenReferenceCanRecovery_Request(CANopenReferenceCanRecovery *context,
                                          uint32_t now_tick);
bool CANopenReferenceCanRecovery_Ready(CANopenReferenceCanRecovery *context,
                                       uint32_t now_tick);
void CANopenReferenceCanRecovery_Complete(CANopenReferenceCanRecovery *context,
                                           bool success, uint32_t now_tick);
CANopenReferenceCanRecoveryState
CANopenReferenceCanRecovery_State(const CANopenReferenceCanRecovery *context);

#endif /* CANOPEN_REFERENCE_CAN_RECOVERY_H */
