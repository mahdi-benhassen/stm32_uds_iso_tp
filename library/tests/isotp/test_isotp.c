#include "uds_iso_tp/isotp.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static IsoTpRx rx;
static IsoTpTx tx;
static uint8_t payload[5000];

static IsoTpCanFrame fd_frame(uint32_t id) {
    IsoTpCanFrame frame = {0};
    frame.can_id = id;
    frame.is_fd = true;
    frame.dlc = 64U;
    return frame;
}

static void test_classic(void) {
    IsoTpConfig config;
    IsoTpCanFrame frame;
    IsoTpRxEvent event;
    uint8_t short_payload[] = {0x22U, 0xF1U, 0x90U};
    isotp_config_classic_can(&config);
    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);
    isotp_rx_init(&rx, &config, 0x7E8U, 0x7E0U);
    assert(isotp_tx_start(&tx, short_payload, sizeof(short_payload), 0U, &frame) ==
           ISOTP_TX_FRAME_READY);
    assert(!frame.is_fd && frame.dlc == 4U && frame.data[0] == 3U);
    assert(isotp_rx_feed(&rx, &frame, 0U, &event) == ISOTP_COMPLETE);
    assert(event.length == sizeof(short_payload));
    assert(memcmp(event.payload, short_payload, sizeof(short_payload)) == 0);

    memset(payload, 0xA5, 4095U);
    assert(isotp_tx_start(&tx, payload, 4095U, 1U, &frame) == ISOTP_TX_FRAME_READY);
    assert(!frame.is_fd && frame.dlc == 8U && frame.data[0] == 0x1FU && frame.data[1] == 0xFFU);
}

static void test_fd_single_frames(void) {
    IsoTpConfig config;
    IsoTpCanFrame frame;
    uint8_t twelve[12] = {0};
    isotp_config_can_fd(&config, 64U, 64U);
    config.bit_rate_switch = true;
    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);
    assert(isotp_tx_start(&tx, twelve, sizeof(twelve), 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert(frame.is_fd && frame.bit_rate_switch && frame.dlc == 16U && frame.data[0] == 0U &&
           frame.data[1] == 12U);
    memset(payload, 0x5A, 62U);
    assert(isotp_tx_start(&tx, payload, 62U, 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert(frame.is_fd && frame.dlc == 64U && frame.data[0] == 0U && frame.data[1] == 62U);
}

static void test_fd_extended_first_frame(void) {
    IsoTpConfig config;
    IsoTpCanFrame frame;
    IsoTpRxEvent event;
    uint32_t now = 10U;
    isotp_config_can_fd(&config, 64U, 64U);
    config.block_size = 0U;
    for (uint32_t index = 0U; index < 5000U; ++index)
        payload[index] = (uint8_t)index;
    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);
    isotp_rx_init(&rx, &config, 0x7E8U, 0x7E0U);
    assert(isotp_tx_start(&tx, payload, 5000U, now, &frame) == ISOTP_TX_FRAME_READY);
    assert(frame.is_fd && frame.dlc == 64U && frame.data[0] == 0x10U && frame.data[1] == 0U);
    assert(frame.data[2] == 0U && frame.data[3] == 0U && frame.data[4] == 0x13U &&
           frame.data[5] == 0x88U);
    assert(isotp_rx_feed(&rx, &frame, now, &event) == ISOTP_NEED_FLOW_CONTROL);
    assert(event.has_flow_control && event.flow_control.is_fd && event.flow_control.dlc == 64U);
    assert(rx.received_len == 58U);
    assert(isotp_tx_feed_flow_control(&tx, &event.flow_control, now) == ISOTP_OK);
    while (tx.active) {
        IsoTpStatus tx_status = isotp_tx_next(&tx, now, &frame);
        if (tx_status == ISOTP_TX_FRAME_READY) {
            IsoTpStatus rx_status = isotp_rx_feed(&rx, &frame, now, &event);
            assert((rx_status == ISOTP_OK) || (rx_status == ISOTP_COMPLETE));
            now += 1U;
        } else {
            assert(tx_status == ISOTP_COMPLETE);
        }
    }
    assert(event.payload != NULL && event.length == 5000U);
    assert(memcmp(event.payload, payload, 5000U) == 0);
}

static void test_flow_control(void) {
    IsoTpConfig config;
    IsoTpCanFrame frame;
    uint8_t data[100] = {0};
    isotp_config_can_fd(&config, 64U, 64U);
    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);
    assert(isotp_tx_start(&tx, data, sizeof(data), 0U, &frame) == ISOTP_TX_FRAME_READY);
    frame = fd_frame(0x7E0U);
    frame.data[0] = 0x33U;
    assert(isotp_tx_feed_flow_control(&tx, &frame, 0U) == ISOTP_ERR_FLOW_CONTROL);
}

int main(void) {
    test_classic();
    test_fd_single_frames();
    test_fd_extended_first_frame();
    test_flow_control();
    return 0;
}
