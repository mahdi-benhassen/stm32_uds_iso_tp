/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Bounded CiA 418 battery-module personality adapter.
 */
#ifndef CIA418_REFERENCE_H
#define CIA418_REFERENCE_H

#include <stdbool.h>
#include <stdint.h>


#define CIA418_STATUS_READY 0x01U
#define CIA418_STATUS_FAULT 0x80U

/** Runtime safety state; application values live in the generated OD. */
typedef struct {
    /** True when the adapter has forced a safe fault condition. */
    bool safe_fault;
} Cia418ReferenceState;

/** Initialize the live CiA 418 OD and the adapter safety state. */
void Cia418Reference_Init(Cia418ReferenceState *state);
/** Update measured battery values directly in the live OD. */
bool Cia418Reference_UpdateMeasurements(Cia418ReferenceState *state, uint32_t battery_voltage,
                                        int16_t temperature, uint8_t battery_state_of_charge,
                                        uint16_t charge_current_requested);
/** Write one supported OD object through the CANopenNode OD interface. */
bool Cia418Reference_WriteObject(Cia418ReferenceState *state, uint16_t index, uint8_t sub_index,
                                 uint32_t value);
/** Read one supported OD object through the CANopenNode OD interface. */
bool Cia418Reference_ReadObject(const Cia418ReferenceState *state, uint16_t index,
                                uint8_t sub_index, uint32_t *value);
/** Force the adapter and live OD into its safe-fault state. */
void Cia418Reference_ForceSafe(Cia418ReferenceState *state);

#endif /* CIA418_REFERENCE_H */
