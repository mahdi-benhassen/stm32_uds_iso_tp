/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "cia418_reference.h"

#include <string.h>

#include "canopen_reference_co.h"
#include "canopen_reference_config.h"

#if (CANOPEN_REFERENCE_ENABLE_CIA418 != 0U)
#include "canopen_reference_od.h"

static bool
cia418_read_io(uint16_t index, uint8_t sub_index, uint32_t *value) {
    OD_entry_t *entry;
    OD_IO_t io;
    OD_size_t count_read = 0U;
    uint32_t raw = 0U;

    if (value == NULL || OD == NULL) {
        return false;
    }
    entry = OD_find(OD, index);
    if (entry == NULL || OD_getSub(entry, sub_index, &io, false) != ODR_OK
        || io.stream.dataLength == 0U || io.stream.dataLength > sizeof(raw)
        || io.read == NULL) {
        return false;
    }
    if (io.read(&io.stream, &raw, io.stream.dataLength, &count_read) != ODR_OK
        || count_read != io.stream.dataLength) {
        return false;
    }
    *value = raw;
    return true;
}

static bool
cia418_write_io(uint16_t index, uint8_t sub_index, uint32_t value) {
    OD_entry_t *entry;
    OD_IO_t io;
    OD_size_t count_written = 0U;

    if (OD == NULL) {
        return false;
    }
    entry = OD_find(OD, index);
    if (entry == NULL || OD_getSub(entry, sub_index, &io, false) != ODR_OK
        || io.stream.dataLength == 0U || io.stream.dataLength > sizeof(value)
        || io.write == NULL || (io.stream.attribute & ODA_SDO_W) == 0U) {
        return false;
    }
    return io.write(&io.stream, &value, io.stream.dataLength, &count_written) == ODR_OK
           && count_written == io.stream.dataLength;
}

void
Cia418Reference_Init(Cia418ReferenceState *state) {
    if (state == NULL) {
        return;
    }
    state->safe_fault = false;
    (void)memset(&OD_APP, 0, sizeof(OD_APP));
    OD_APP.x6020_batteryParameters.highestSub_indexSupported = 4U;
    OD_APP.x6030_batterySerialNumber[0] = 3U;
    OD_APP.x6031_batteryId[0] = 5U;
    OD_APP.x6040_vehicleSerialNumber[0] = 5U;
    OD_APP.x6041_vehicleId[0] = 5U;
    OD_APP.x6054_dateOfLastEqualization[0] = 2U;
    OD_APP.x6000_batteryStatus = CIA418_STATUS_READY;
}

void
Cia418Reference_ForceSafe(Cia418ReferenceState *state) {
    if (state == NULL) {
        return;
    }
    state->safe_fault = true;
    OD_APP.x6000_batteryStatus |= CIA418_STATUS_FAULT;
    OD_APP.x6070_chargeCurrentRequested = 0U;
    OD_APP.x6001_chargerStatus = 0U;
}

bool
Cia418Reference_UpdateMeasurements(Cia418ReferenceState *state,
                                   uint32_t battery_voltage,
                                   int16_t temperature,
                                   uint8_t battery_state_of_charge,
                                   uint16_t charge_current_requested) {
    if (state == NULL || battery_state_of_charge > 100U) {
        Cia418Reference_ForceSafe(state);
        return false;
    }
    OD_APP.x6060_batteryVoltage = battery_voltage;
    OD_APP.x6010_temperature = temperature;
    OD_APP.x6081_batteryStateOfCharge = battery_state_of_charge;
    OD_APP.x6070_chargeCurrentRequested = charge_current_requested;
    state->safe_fault = false;
    OD_APP.x6000_batteryStatus &= (uint8_t)~CIA418_STATUS_FAULT;
    return true;
}

bool
Cia418Reference_WriteObject(Cia418ReferenceState *state,
                            uint16_t index,
                            uint8_t sub_index,
                            uint32_t value) {
    if (state == NULL) {
        return false;
    }
    if (index == 0x6080U && value > 100U) {
        return false;
    }
    return cia418_write_io(index, sub_index, value);
}

bool
Cia418Reference_ReadObject(const Cia418ReferenceState *state,
                           uint16_t index,
                           uint8_t sub_index,
                           uint32_t *value) {
    if (state == NULL) {
        return false;
    }
    return cia418_read_io(index, sub_index, value);
}

#else

void
Cia418Reference_Init(Cia418ReferenceState *state) {
    if (state != NULL) {
        state->safe_fault = false;
    }
}

void
Cia418Reference_ForceSafe(Cia418ReferenceState *state) {
    if (state != NULL) {
        state->safe_fault = true;
    }
}

bool
Cia418Reference_UpdateMeasurements(Cia418ReferenceState *state,
                                   uint32_t battery_voltage,
                                   int16_t temperature,
                                   uint8_t battery_state_of_charge,
                                   uint16_t charge_current_requested) {
    (void)battery_voltage;
    (void)temperature;
    (void)battery_state_of_charge;
    (void)charge_current_requested;
    Cia418Reference_ForceSafe(state);
    return false;
}

bool
Cia418Reference_WriteObject(Cia418ReferenceState *state,
                            uint16_t index,
                            uint8_t sub_index,
                            uint32_t value) {
    (void)state;
    (void)index;
    (void)sub_index;
    (void)value;
    return false;
}

bool
Cia418Reference_ReadObject(const Cia418ReferenceState *state,
                           uint16_t index,
                           uint8_t sub_index,
                           uint32_t *value) {
    (void)state;
    (void)index;
    (void)sub_index;
    (void)value;
    return false;
}

#endif
