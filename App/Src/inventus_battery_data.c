/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include <stddef.h>

#include "inventus_battery_data.h"

#include "canopen_reference_config.h"

#include "canopen_reference_co.h"

#if (CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY != 0U)

#include "canopen_reference_od.h"

static bool
inventus_read_io(uint16_t index, uint8_t sub_index, uint32_t *value) {
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

void
InventusBatteryData_Init(InventusBatteryDataState *state) {
    if (state == NULL) {
        return;
    }
    state->initialized = true;
}

bool
InventusBatteryData_UpdateMeasurements(InventusBatteryDataState *state,
                                        uint32_t battery_voltage,
                                        int16_t temperature,
                                        uint8_t battery_state_of_charge,
                                        uint16_t charge_current_requested) {
    if (state == NULL || !state->initialized || battery_state_of_charge > 100U) {
        return false;
    }
    OD_APP.x6060_inventus_6060_battery_voltage = battery_voltage;
    OD_APP.x6010_inventus_6010_temperature = temperature;
    OD_APP.x6081_inventus_6081_battery_state_of_charge = battery_state_of_charge;
    OD_APP.x6070_inventus_6070_charge_current_requested = charge_current_requested;
    return true;
}

bool
InventusBatteryData_Read(const InventusBatteryDataState *state,
                          uint16_t index,
                          uint8_t sub_index,
                          uint32_t *value) {
    if (state == NULL || !state->initialized) {
        return false;
    }
    return inventus_read_io(index, sub_index, value);
}

#else

void
InventusBatteryData_Init(InventusBatteryDataState *state) {
    if (state != NULL) {
        state->initialized = false;
    }
}

bool
InventusBatteryData_UpdateMeasurements(InventusBatteryDataState *state,
                                        uint32_t battery_voltage,
                                        int16_t temperature,
                                        uint8_t battery_state_of_charge,
                                        uint16_t charge_current_requested) {
    (void)state;
    (void)battery_voltage;
    (void)temperature;
    (void)battery_state_of_charge;
    (void)charge_current_requested;
    return false;
}

bool
InventusBatteryData_Read(const InventusBatteryDataState *state,
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
