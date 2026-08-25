#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "cia418_reference.h"
#include "canopen_reference_od.h"

int main(void) {
    Cia418ReferenceState state;
    uint32_t value = 0U;

    Cia418Reference_Init(&state);
    assert(OD_APP.x6000_batteryStatus == CIA418_STATUS_READY);
    assert(!state.safe_fault);

    assert(Cia418Reference_UpdateMeasurements(&state, 48000U, 250, 75U, 120U));
    assert(OD_APP.x6060_batteryVoltage == 48000U);
    assert(OD_APP.x6081_batteryStateOfCharge == 75U);
    assert(OD_APP.x6010_temperature == 250);
    assert(Cia418Reference_ReadObject(&state, 0x6060U, 0U, &value) && value == 48000U);
    assert(Cia418Reference_ReadObject(&state, 0x6081U, 0U, &value) && value == 75U);
    assert(Cia418Reference_ReadObject(&state, 0x6010U, 0U, &value) && value == 250U);

    assert(!Cia418Reference_UpdateMeasurements(&state, 48000U, 250, 101U, 120U));
    assert(state.safe_fault);
    assert(OD_APP.x6070_chargeCurrentRequested == 0U);
    assert((OD_APP.x6000_batteryStatus & CIA418_STATUS_FAULT) != 0U);

    assert(Cia418Reference_WriteObject(&state, 0x6001U, 0U, 2U));
    assert(Cia418Reference_WriteObject(&state, 0x6080U, 0U, 80U));
    assert(Cia418Reference_WriteObject(&state, 0x6054U, 1U, 2026U));
    assert(!Cia418Reference_WriteObject(&state, 0x6081U, 0U, 50U));
    assert(!Cia418Reference_WriteObject(&state, 0x6080U, 0U, 101U));
    assert(Cia418Reference_ReadObject(&state, 0x6054U, 1U, &value) && value == 2026U);
    assert(!Cia418Reference_ReadObject(&state, 0x6054U, 3U, &value));

    OD_APP.x6030_batterySerialNumber[1] = 0x12345678U;
    OD_APP.x6041_vehicleId[1] = 0xABCDEF01U;
    assert(Cia418Reference_ReadObject(&state, 0x6030U, 1U, &value) && value == 0x12345678U);
    assert(Cia418Reference_ReadObject(&state, 0x6041U, 1U, &value) && value == 0xABCDEF01U);

    puts("cia418_reference: PASS");
    return 0;
}
