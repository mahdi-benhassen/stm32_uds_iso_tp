/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Test-only application data seam for the Inventus battery OD personality.
 * Generated OD files remain disposable artifacts; board/BMS integrations should
 * call this API to publish measurements into the OD.
 */
#ifndef INVENTUS_BATTERY_DATA_H
#define INVENTUS_BATTERY_DATA_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool initialized;
} InventusBatteryDataState;

void InventusBatteryData_Init(InventusBatteryDataState *state);

bool InventusBatteryData_UpdateMeasurements(InventusBatteryDataState *state,
                                            uint32_t battery_voltage,
                                            int16_t temperature,
                                            uint8_t battery_state_of_charge,
                                            uint16_t charge_current_requested);

bool InventusBatteryData_Read(const InventusBatteryDataState *state,
                              uint16_t index,
                              uint8_t sub_index,
                              uint32_t *value);

#endif
