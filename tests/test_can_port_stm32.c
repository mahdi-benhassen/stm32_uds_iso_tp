/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "can_port.h"

static uint32_t s_received_count;
static uint32_t s_last_id;
static uint8_t s_last_data[CAN_PORT_MAX_DLC];
static uint8_t s_last_len;

static void
on_frame(uint32_t id, uint8_t *data, uint8_t len) {
    ++s_received_count;
    s_last_id = id;
    s_last_len = len;
    (void)memcpy(s_last_data, data, len);
}

int
main(void) {
    CAN_HandleTypeDef hcan = {0};
    uint8_t payload[] = {0x11U, 0x22U, 0x33U};
    uint32_t base_received;
    uint32_t index;

    assert(can_port_stm32_bind(NULL) == -EINVAL);
    assert(can_port_stm32_bind(&hcan) == 0);
    assert(can_port_stm32_bind(&hcan) == -EBUSY);
    assert(can_port_init(500000U) == 0);
    assert(hcan.Init.Prescaler == 6U);
    assert(hcan.Init.TimeSeg1 == CAN_BS1_14TQ);
    assert(hcan.Init.TimeSeg2 == CAN_BS2_3TQ);
    assert(can_port_poll(5U) == 0);

    can_port_register_rx(on_frame);
    can_port_stm32_dispatch_rx_from_isr(0x123U, payload, (uint8_t)sizeof(payload));
    payload[0] = 0xFFU;
    assert(s_received_count == 0U);
    assert(can_port_poll(0U) == 1);
    assert(s_received_count == 1U);
    assert(s_last_id == 0x123U);
    assert(s_last_len == sizeof(payload));
    assert(s_last_data[0] == 0x11U);
    assert(s_last_data[1] == 0x22U);
    assert(s_last_data[2] == 0x33U);
    assert(can_port_poll(0U) == 0);

    can_port_stm32_dispatch_rx_from_isr(0x800U, payload, (uint8_t)sizeof(payload));
    can_port_stm32_dispatch_rx_from_isr(0x123U, NULL, (uint8_t)sizeof(payload));
    can_port_stm32_dispatch_rx_from_isr(0x123U, payload, CAN_PORT_MAX_DLC + 1U);
    assert(can_port_poll(0U) == 0);

    base_received = s_received_count;
    for (index = 0U; index < CAN_PORT_RX_QUEUE_CAPACITY - 1U; ++index) {
        payload[0] = (uint8_t)index;
        can_port_stm32_dispatch_rx_from_isr(0x200U + index, payload, 1U);
    }
    can_port_stm32_dispatch_rx_from_isr(0x3FFU, payload, 1U);
    assert(can_port_stm32_rx_dropped() == 1U);

    for (index = 0U; index < CAN_PORT_RX_QUEUE_CAPACITY - 1U; ++index) {
        assert(can_port_poll(0U) == 1);
        assert(s_last_id == 0x200U + index);
        assert(s_last_len == 1U);
        assert(s_last_data[0] == (uint8_t)index);
    }
    assert(s_received_count == base_received + CAN_PORT_RX_QUEUE_CAPACITY - 1U);
    assert(can_port_poll(0U) == 0);

    can_port_deinit();
    return 0;
}
