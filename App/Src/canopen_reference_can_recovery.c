/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "canopen_reference_can_recovery.h"

#include <stddef.h>

void
CANopenReferenceCanRecovery_Init(CANopenReferenceCanRecovery *context,
                                 uint32_t wait_ms, uint32_t max_attempts) {
    if (context == NULL) {
        return;
    }
    context->state = CANOPEN_REFERENCE_CAN_RUNNING;
    context->recovery_due_tick = 0U;
    context->attempts = 0U;
    context->max_attempts = max_attempts;
    context->wait_ms = wait_ms;
}

void
CANopenReferenceCanRecovery_Request(CANopenReferenceCanRecovery *context,
                                     uint32_t now_tick) {
    if (context == NULL || context->state == CANOPEN_REFERENCE_CAN_FAULT) {
        return;
    }
    context->state = CANOPEN_REFERENCE_CAN_WAIT_RECOVERY;
    context->recovery_due_tick = now_tick + context->wait_ms;
}

bool
CANopenReferenceCanRecovery_Ready(CANopenReferenceCanRecovery *context,
                                   uint32_t now_tick) {
    if (context == NULL || context->state != CANOPEN_REFERENCE_CAN_WAIT_RECOVERY) {
        return false;
    }
    return (int32_t)(now_tick - context->recovery_due_tick) >= 0;
}

void
CANopenReferenceCanRecovery_Complete(CANopenReferenceCanRecovery *context,
                                      bool success, uint32_t now_tick) {
    if (context == NULL) {
        return;
    }
    if (success) {
        context->state = CANOPEN_REFERENCE_CAN_RUNNING;
        context->recovery_due_tick = 0U;
        return;
    }
    context->attempts++;
    if (context->max_attempts != 0U && context->attempts >= context->max_attempts) {
        context->state = CANOPEN_REFERENCE_CAN_FAULT;
        return;
    }
    context->state = CANOPEN_REFERENCE_CAN_WAIT_RECOVERY;
    context->recovery_due_tick = now_tick + context->wait_ms;
}

CANopenReferenceCanRecoveryState
CANopenReferenceCanRecovery_State(const CANopenReferenceCanRecovery *context) {
    return context == NULL ? CANOPEN_REFERENCE_CAN_FAULT : context->state;
}
