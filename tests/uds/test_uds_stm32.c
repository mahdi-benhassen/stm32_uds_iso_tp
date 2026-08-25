/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "uds_stm32.h"

#include <assert.h>
#include <stdint.h>

int main(void) {
    CAN_HandleTypeDef hcan = {0};
    UdsStm32Can adapter = {0};
    assert(uds_stm32_can_bind(&adapter, &hcan, 0x700U, 0x708U) == 0);
    assert(uds_stm32_can_request_id(&adapter) == 0x700U);
    assert(uds_stm32_can_response_id(&adapter) == 0x708U);
    assert(uds_stm32_can_filter_match(&adapter, 0x700U));
    assert(uds_stm32_can_filter_match(&adapter, 0x708U));
    assert(!uds_stm32_can_filter_match(&adapter, 0x701U));

    uint8_t data[8] = {0x03U, 0x22U, 0xF1U, 0x90U, 0U, 0U, 0U, 0U};
    uds_stm32_can_rx_from_isr(&adapter, 0x701U, data, 4U);
    uds_stm32_can_rx_from_isr(&adapter, 0x700U, data, 4U);
    IsoTpCanFrame frame = {0};
    assert(uds_stm32_can_rx_pop(&adapter, &frame) == 1);
    assert(frame.can_id == 0x700U && frame.dlc == 4U && frame.data[1] == 0x22U);
    assert(uds_stm32_can_rx_pop(&adapter, &frame) == 0);

    for (uint32_t index = 0U; index < UDS_STM32_RX_QUEUE_CAPACITY; ++index) {
        uds_stm32_can_rx_from_isr(&adapter, 0x700U, data, 4U);
    }
    UdsStm32CanStats stats;
    uds_stm32_can_get_stats(&adapter, &stats);
    assert(stats.rx_overflow == 1U);

    frame.can_id = 0x708U;
    frame.dlc = 2U;
    frame.data[0] = 0x02U;
    frame.data[1] = 0x7EU;
    assert(uds_stm32_can_tx_queue(&adapter, &frame) == 0);
    assert(uds_stm32_can_process_tx(&adapter, 1U) == 1);
    uds_stm32_can_get_stats(&adapter, &stats);
    assert(stats.tx_frames == 1U);
    assert(uds_stm32_can_process_tx(&adapter, 1U) == 0);

    uds_stm32_can_note_rx_timeout(&adapter);
    uds_stm32_can_note_tx_timeout(&adapter);
    uds_stm32_can_note_error(&adapter, 1U, true);
    uds_stm32_can_get_stats(&adapter, &stats);
    assert(stats.rx_timeouts == 1U && stats.tx_timeouts == 1U);
    assert(stats.bus_errors == 1U && stats.bus_off_events == 1U);
    return 0;
}
