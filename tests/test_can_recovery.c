/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "canopen_reference_can_recovery.h"

int
main(void) {
    CANopenReferenceCanRecovery recovery;
    CANopenReferenceCanRecovery_Init(NULL, 1U, 1U);
    CANopenReferenceCanRecovery_Request(NULL, 0U);
    assert(!CANopenReferenceCanRecovery_Ready(NULL, 0U));
    CANopenReferenceCanRecovery_Complete(NULL, true, 0U);
    assert(CANopenReferenceCanRecovery_State(NULL) == CANOPEN_REFERENCE_CAN_FAULT);

    CANopenReferenceCanRecovery_Init(&recovery, 10U, 2U);
    assert(CANopenReferenceCanRecovery_State(&recovery) == CANOPEN_REFERENCE_CAN_RUNNING);

    CANopenReferenceCanRecovery_Request(&recovery, 100U);
    assert(CANopenReferenceCanRecovery_State(&recovery) == CANOPEN_REFERENCE_CAN_WAIT_RECOVERY);
    assert(!CANopenReferenceCanRecovery_Ready(&recovery, 109U));
    assert(CANopenReferenceCanRecovery_Ready(&recovery, 110U));

    CANopenReferenceCanRecovery_Complete(&recovery, false, 110U);
    assert(recovery.attempts == 1U);
    assert(!CANopenReferenceCanRecovery_Ready(&recovery, 119U));
    assert(CANopenReferenceCanRecovery_Ready(&recovery, 120U));

    CANopenReferenceCanRecovery_Complete(&recovery, false, 120U);
    assert(CANopenReferenceCanRecovery_State(&recovery) == CANOPEN_REFERENCE_CAN_FAULT);
    CANopenReferenceCanRecovery_Request(&recovery, 130U);
    assert(CANopenReferenceCanRecovery_State(&recovery) == CANOPEN_REFERENCE_CAN_FAULT);
    assert(!CANopenReferenceCanRecovery_Ready(&recovery, 130U));

    CANopenReferenceCanRecovery_Init(&recovery, 10U, 0U);
    CANopenReferenceCanRecovery_Request(&recovery, 200U);
    assert(CANopenReferenceCanRecovery_Ready(&recovery, 210U));
    CANopenReferenceCanRecovery_Complete(&recovery, false, 210U);
    assert(CANopenReferenceCanRecovery_State(&recovery) == CANOPEN_REFERENCE_CAN_WAIT_RECOVERY);
    assert(recovery.attempts == 1U);
    CANopenReferenceCanRecovery_Complete(&recovery, true, 211U);
    assert(CANopenReferenceCanRecovery_State(&recovery) == CANOPEN_REFERENCE_CAN_RUNNING);
    assert(recovery.recovery_due_tick == 0U);

    CANopenReferenceCanRecovery_Init(&recovery, 10U, 3U);
    CANopenReferenceCanRecovery_Request(&recovery, 0xFFFFFFF0U);
    assert(!CANopenReferenceCanRecovery_Ready(&recovery, 0xFFFFFFF9U));
    assert(CANopenReferenceCanRecovery_Ready(&recovery, 4U));
    CANopenReferenceCanRecovery_Complete(&recovery, true, 4U);
    assert(CANopenReferenceCanRecovery_State(&recovery) == CANOPEN_REFERENCE_CAN_RUNNING);
    assert(recovery.attempts == 0U);

    return 0;
}
