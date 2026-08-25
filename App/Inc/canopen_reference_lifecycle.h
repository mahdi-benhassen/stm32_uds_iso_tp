/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_REFERENCE_LIFECYCLE_H
#define CANOPEN_REFERENCE_LIFECYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CANOPEN_REFERENCE_RUNTIME_INIT = 0,
    CANOPEN_REFERENCE_RUNTIME_RUNNING,
    CANOPEN_REFERENCE_RUNTIME_RESET_REQUESTED,
    CANOPEN_REFERENCE_RUNTIME_REINITIALIZING,
    CANOPEN_REFERENCE_RUNTIME_SAFE_FAULT
} CANopenReferenceRuntimeState;

/** Return the current project-owned runtime lifecycle state. */
CANopenReferenceRuntimeState CANopenReference_RuntimeState(void);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_REFERENCE_LIFECYCLE_H */
