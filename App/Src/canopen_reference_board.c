/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "canopen_reference_board.h"

#include "canopen_reference_hw.h"

/*
 * These weak hooks deliberately do not select a GPIO: the exact transceiver
 * standby/enable pin, polarity, and power-stage safety topology are board
 * properties. A product port overrides these symbols with verified GPIO and
 * independent safety hardware control.
 */
__attribute__((weak)) void
CANopenReferenceBoard_SetCanTransceiverEnabled(bool enabled) {
    (void)enabled;
}

__attribute__((weak)) void
CANopenReferenceBoard_InitSafe(void) {
    CANopenReferenceBoard_SetCanTransceiverEnabled(false);
    CANopenReferenceHw_WriteDigitalOutputs(0U);
    CANopenReferenceHw_WriteAnalogOutput(0U, 0);
    CANopenReferenceHw_DriveSetEnable(false);
}

__attribute__((weak)) void
CANopenReferenceBoard_ForceSafe(void) {
    CANopenReferenceBoard_SetCanTransceiverEnabled(false);
    CANopenReferenceHw_WriteDigitalOutputs(0U);
    CANopenReferenceHw_WriteAnalogOutput(0U, 0);
    CANopenReferenceHw_DriveSetEnable(false);
}

__attribute__((weak)) void
CANopenReferenceBoard_OnCanopenReady(void) {
    /* A board may leave the transceiver in standby until its own power and
     * safety checks pass. The default stays de-energized. */
}
