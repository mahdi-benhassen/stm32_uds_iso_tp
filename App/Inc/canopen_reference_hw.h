/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_REFERENCE_HW_H
#define CANOPEN_REFERENCE_HW_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Hardware adapter contract. The reference supplies weak fail-safe defaults in
 * canopen_reference_hw.c. A product must replace these functions with board and
 * safety-layer implementations; no CANopen state transition is proof that a
 * physical actuator can be energized safely.
 */

uint8_t CANopenReferenceHw_ReadDigitalInputs(void);
void CANopenReferenceHw_WriteDigitalOutputs(uint8_t value);
int16_t CANopenReferenceHw_ReadAnalogInput(uint8_t channel);
void CANopenReferenceHw_WriteAnalogOutput(uint8_t channel, int16_t value);

bool CANopenReferenceHw_DriveInterlocksHealthy(void);
void CANopenReferenceHw_DriveSetEnable(bool enable);
void CANopenReferenceHw_DriveCommand(int8_t mode, int32_t position, int32_t velocity, int16_t torque);
void CANopenReferenceHw_DriveReadFeedback(int32_t *position, int32_t *velocity, int16_t *torque,
                                          uint16_t *errorCode, bool *faultActive);

#endif /* CANOPEN_REFERENCE_HW_H */
