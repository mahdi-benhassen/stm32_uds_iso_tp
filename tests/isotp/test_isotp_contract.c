/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "isotp.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static IsoTpCanFrame make_frame(uint32_t id, const uint8_t *data, uint8_t dlc) {
    IsoTpCanFrame frame = {0};
    frame.can_id = id;
    frame.dlc = dlc;
    (void)memcpy(frame.data, data, dlc);
    return frame;
}

int main(void) {
    IsoTpConfig config;
    IsoTpRx rx;
    IsoTpTx tx;
    IsoTpRxEvent event;
    IsoTpCanFrame frame;
    const uint8_t ff[] = {0x10U, 0x0AU, 1U, 2U, 3U, 4U, 5U, 6U};
    const uint8_t cf[] = {0x21U, 7U, 8U, 9U, 10U};
    const uint8_t payload[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U};
    const uint8_t fc[] = {0x30U, 1U, 0xF1U};

    isotp_config_default(&config);
    config.block_size = 1U;
    config.rx_timeout_ms = 20U;
    config.tx_timeout_ms = 20U;

    isotp_rx_init(&rx, &config, 0x7E0U, 0x7E8U);
    frame = make_frame(0x7E0U, ff, sizeof(ff));
    assert(isotp_rx_feed(&rx, &frame, 0U, &event) == ISOTP_NEED_FLOW_CONTROL);
    assert(event.has_flow_control && event.flow_control.data[0] == 0x30U);
    frame = make_frame(0x7E0U, cf, sizeof(cf));
    assert(isotp_rx_feed(&rx, &frame, 1U, &event) == ISOTP_COMPLETE);
    assert(event.payload != NULL && event.length == sizeof(payload));
    assert(memcmp(event.payload, payload, sizeof(payload)) == 0);

    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);
    assert(isotp_tx_start(&tx, payload, sizeof(payload), 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert((frame.data[0] >> 4U) == 1U);
    frame = make_frame(0x7E0U, fc, sizeof(fc));
    assert(isotp_tx_feed_flow_control(&tx, &frame, 0U) == ISOTP_OK);
    assert(isotp_tx_next(&tx, 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert((frame.data[0] >> 4U) == 2U && (frame.data[0] & 0x0FU) == 1U);
    assert(isotp_tx_next(&tx, 1U, &frame) == ISOTP_COMPLETE ||
           isotp_tx_next(&tx, 1U, &frame) == ISOTP_OK);

    isotp_rx_init(&rx, &config, 0x7E0U, 0x7E8U);
    frame = make_frame(0x7E0U, ff, sizeof(ff));
    assert(isotp_rx_feed(&rx, &frame, 0U, &event) == ISOTP_NEED_FLOW_CONTROL);
    assert(isotp_rx_tick(&rx, 21U) == ISOTP_ERR_TIMEOUT);
    return 0;
}
