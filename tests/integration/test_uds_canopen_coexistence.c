/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "can_acceptance_filter.h"
#include "uds_stm32.h"

#include <assert.h>
#include <stdint.h>

static bool contains(const uint16_t *ids, uint32_t count, uint16_t id) {
    for (uint32_t index = 0U; index < count; ++index) {
        if (ids[index] == id) {
            return true;
        }
    }
    return false;
}

int main(void) {
    uint16_t ids[20U] = {0};
    uint32_t count = 0U;
    const uint16_t canopen_ids[] = {
        0x000U, 0x080U, 0x180U, 0x280U, 0x380U, 0x480U, 0x580U, 0x600U, 0x700U, 0x7E4U, 0x7E5U,
    };
    for (uint32_t index = 0U; index < (sizeof(canopen_ids) / sizeof(canopen_ids[0])); ++index) {
        assert(CANopenAcceptanceFilter_Add(ids, 20U, &count, canopen_ids[index]));
    }
    assert(CANopenAcceptanceFilter_Add(ids, 20U, &count, 0x7E0U));
    assert(CANopenAcceptanceFilter_Add(ids, 20U, &count, 0x7E8U));
    assert(count == 13U);
    assert(contains(ids, count, 0x000U));
    assert(contains(ids, count, 0x600U));
    assert(contains(ids, count, 0x7E0U));
    assert(contains(ids, count, 0x7E8U));
    assert(CANopenAcceptanceFilter_Add(ids, 20U, &count, 0x7E0U));
    assert(count == 13U);

    CAN_HandleTypeDef hcan = {0};
    UdsStm32Can adapter = {0};
    assert(uds_stm32_can_bind(&adapter, &hcan, 0x7E0U, 0x7E8U) == 0);
    uint8_t data[8] = {0x02U, 0x3EU, 0x00U, 0U, 0U, 0U, 0U, 0U};
    uint32_t uds_received = 0U;
    for (uint32_t sequence = 0U; sequence < 1000U; ++sequence) {
        uint32_t canopen_id =
            canopen_ids[sequence % (sizeof(canopen_ids) / sizeof(canopen_ids[0]))];
        uds_stm32_can_rx_from_isr(&adapter, canopen_id, data, 3U);
        uds_stm32_can_rx_from_isr(&adapter, 0x7E0U, data, 3U);
        IsoTpCanFrame frame = {0};
        if (uds_stm32_can_rx_pop(&adapter, &frame) == 1) {
            ++uds_received;
            assert(frame.can_id == 0x7E0U);
            assert(frame.data[1] == 0x3EU);
        }
    }
    assert(uds_received == 1000U);
    UdsStm32CanStats stats;
    uds_stm32_can_get_stats(&adapter, &stats);
    assert(stats.rx_overflow == 0U);
    assert(stats.rx_frames == 1000U);
    assert(stats.tx_frames == 0U);
    return 0;
}
