/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "canopen_reference_hw.h"

#include <stddef.h>

/* These weak definitions provide a linkable, fail-safe reference. Replace every
 * applicable hook in the board project; a weak definition is not a hardware
 * implementation. */
#if defined(__GNUC__)
#define CANOPEN_REFERENCE_WEAK __attribute__((weak))
#else
#define CANOPEN_REFERENCE_WEAK
#endif

CANOPEN_REFERENCE_WEAK uint8_t
CANopenReferenceHw_ReadDigitalInputs(void) {
    return 0U;
}

CANOPEN_REFERENCE_WEAK void
CANopenReferenceHw_WriteDigitalOutputs(uint8_t value) {
    (void)value;
}

CANOPEN_REFERENCE_WEAK int16_t
CANopenReferenceHw_ReadAnalogInput(uint8_t channel) {
    (void)channel;
    return 0;
}

CANOPEN_REFERENCE_WEAK void
CANopenReferenceHw_WriteAnalogOutput(uint8_t channel, int16_t value) {
    (void)channel;
    (void)value;
}

CANOPEN_REFERENCE_WEAK bool
CANopenReferenceHw_DriveInterlocksHealthy(void) {
    return false;
}

CANOPEN_REFERENCE_WEAK void
CANopenReferenceHw_DriveSetEnable(bool enable) {
    (void)enable;
}

CANOPEN_REFERENCE_WEAK void
CANopenReferenceHw_DriveCommand(int8_t mode, int32_t position, int32_t velocity, int16_t torque) {
    (void)mode;
    (void)position;
    (void)velocity;
    (void)torque;
}

CANOPEN_REFERENCE_WEAK void
CANopenReferenceHw_DriveReadFeedback(int32_t *position, int32_t *velocity, int16_t *torque,
                                     uint16_t *errorCode, bool *faultActive) {
    if (position != NULL) {
        *position = 0;
    }
    if (velocity != NULL) {
        *velocity = 0;
    }
    if (torque != NULL) {
        *torque = 0;
    }
    if (errorCode != NULL) {
        *errorCode = 0;
    }
    if (faultActive != NULL) {
        *faultActive = false;
    }
}
