/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "canopen_reference_uds.h"

#include <assert.h>
#include <stdint.h>

int main(void) {
    CAN_HandleTypeDef hcan = {0};
    assert(CANopenReference_UDS_Init(&hcan, 0U) == 0);
    const uint8_t read_software_version[] = {0x03U, 0x22U, 0xF1U, 0x80U};
    CANopenReference_UDS_RxFromIsr(UDS_RX_CAN_ID, read_software_version,
                                   (uint8_t)sizeof(read_software_version));
    CANopenReference_UDS_Process(1U);

    UdsStm32CanStats stats = {0};
    CANopenReference_UDS_GetStats(&stats);
    assert(stats.rx_frames == 1U);
    assert(stats.tx_frames == 1U);
    assert(stats.rx_overflow == 0U);
    return 0;
}
