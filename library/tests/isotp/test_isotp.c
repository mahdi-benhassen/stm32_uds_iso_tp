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

static IsoTpCanFrame fc_frame(bool can_fd, uint32_t id, uint8_t status, uint8_t block_size,
                              uint8_t st_min) {
    IsoTpCanFrame frame = can_fd ? fd_frame(id) : (IsoTpCanFrame){0};
    frame.can_id = id;
    frame.dlc = can_fd ? 64U : 3U;
    frame.data[0] = (uint8_t)(0x30U | status);
    frame.data[1] = block_size;
    frame.data[2] = st_min;
    return frame;
}

static void start_long_transfer(IsoTpTx *transport, const IsoTpConfig *config, bool can_fd,
                                IsoTpCanFrame *first) {
    static uint8_t data[200] = {0};
    isotp_tx_init(transport, config, 0x7E0U, 0x7E8U);
    assert(isotp_tx_start(transport, data, sizeof(data), 0U, first) == ISOTP_TX_FRAME_READY);
    assert(isotp_tx_state(transport) == ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL);
    assert(first->is_fd == can_fd);
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
    while (isotp_tx_state(&tx) != ISOTP_TX_STATE_IDLE) {
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

static void test_flow_control_profile(bool can_fd) {
    IsoTpConfig config;
    IsoTpCanFrame frame;
    if (can_fd) {
        isotp_config_can_fd(&config, 64U, 64U);
    } else {
        isotp_config_classic_can(&config);
    }

    start_long_transfer(&tx, &config, can_fd, &frame);
    IsoTpCanFrame wrong_id = fc_frame(can_fd, 0x7E1U, ISOTP_FC_CTS, 2U, 0U);
    assert(isotp_tx_feed_flow_control(&tx, &wrong_id, 0U) == ISOTP_ERR_ARGUMENT);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL);

    IsoTpCanFrame short_fc = fc_frame(can_fd, 0x7E0U, ISOTP_FC_CTS, 2U, 0U);
    short_fc.dlc = 2U;
    assert(isotp_tx_feed_flow_control(&tx, &short_fc, 0U) ==
           (can_fd ? ISOTP_ERR_ARGUMENT : ISOTP_ERR_FORMAT));
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL);

    IsoTpCanFrame not_fc = fc_frame(can_fd, 0x7E0U, ISOTP_FC_CTS, 2U, 0U);
    not_fc.data[0] = 0x20U;
    assert(isotp_tx_feed_flow_control(&tx, &not_fc, 0U) == ISOTP_ERR_FORMAT);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL);

    IsoTpCanFrame invalid_st_min = fc_frame(can_fd, 0x7E0U, ISOTP_FC_CTS, 2U, 0x80U);
    assert(isotp_tx_feed_flow_control(&tx, &invalid_st_min, 0U) == ISOTP_ERR_FLOW_CONTROL);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_IDLE);

    start_long_transfer(&tx, &config, can_fd, &frame);
    IsoTpCanFrame cts = fc_frame(can_fd, 0x7E0U, ISOTP_FC_CTS, 2U, 0U);
    assert(isotp_tx_feed_flow_control(&tx, &cts, 0U) == ISOTP_OK);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_SEND_CONSECUTIVE);
    assert(tx.remote_block_size == 2U && tx.remote_st_min == 0U);
    assert(isotp_tx_next(&tx, 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_SEND_CONSECUTIVE);
    assert(isotp_tx_next(&tx, 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    IsoTpCanFrame block_cts = fc_frame(can_fd, 0x7E0U, ISOTP_FC_CTS, 0U, 0xF1U);
    assert(isotp_tx_feed_flow_control(&tx, &block_cts, 1U) == ISOTP_OK);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_SEND_CONSECUTIVE);

    start_long_transfer(&tx, &config, can_fd, &frame);
    IsoTpCanFrame wait = fc_frame(can_fd, 0x7E0U, ISOTP_FC_WAIT, 0U, 0U);
    config.max_wait_frames = 2U;
    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);
    assert(isotp_tx_start(&tx, (const uint8_t[200]){0}, 200U, 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert(isotp_tx_feed_flow_control(&tx, &wait, 0U) == ISOTP_OK);
    assert(isotp_tx_feed_flow_control(&tx, &wait, 1U) == ISOTP_OK);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL);
    assert(tx.wait_frames == 2U);
    assert(isotp_tx_feed_flow_control(&tx, &wait, 2U) == ISOTP_ERR_FLOW_CONTROL);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_IDLE);

    start_long_transfer(&tx, &config, can_fd, &frame);
    IsoTpCanFrame overflow = fc_frame(can_fd, 0x7E0U, ISOTP_FC_OVERFLOW, 0U, 0U);
    assert(isotp_tx_feed_flow_control(&tx, &overflow, 0U) == ISOTP_ERR_FLOW_OVERFLOW);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_IDLE);

    start_long_transfer(&tx, &config, can_fd, &frame);
    IsoTpCanFrame reserved_flow = fc_frame(can_fd, 0x7E0U, ISOTP_FC_CTS, 0U, 0U);
    reserved_flow.data[0] = 0x33U;
    assert(isotp_tx_feed_flow_control(&tx, &reserved_flow, 0U) == ISOTP_ERR_FLOW_CONTROL);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_IDLE);
}

static void start_classic_payload(IsoTpTx *transport, IsoTpConfig *config, uint8_t *data,
                                  uint32_t length, IsoTpCanFrame *frame) {
    isotp_config_classic_can(config);
    for (uint32_t index = 0U; index < length; ++index)
        data[index] = (uint8_t)index;
    isotp_tx_init(transport, config, 0x7E0U, 0x7E8U);
    assert(isotp_tx_start(transport, data, length, 0U, frame) == ISOTP_TX_FRAME_READY);
    assert((frame->data[0] >> 4U) == 1U);
    assert(isotp_tx_state(transport) == ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL);
}

static void accept_classic_cts(IsoTpTx *transport, uint8_t block_size, uint8_t st_min,
                               uint32_t now_ms) {
    IsoTpCanFrame flow_control = fc_frame(false, 0x7E0U, ISOTP_FC_CTS, block_size, st_min);
    assert(isotp_tx_feed_flow_control(transport, &flow_control, now_ms) == ISOTP_OK);
}

static void assert_classic_cf(IsoTpTx *transport, IsoTpCanFrame *frame, uint32_t now_ms,
                              uint8_t sequence) {
    assert(isotp_tx_next(transport, now_ms, frame) == ISOTP_TX_FRAME_READY);
    assert(frame->data[0] == (uint8_t)(0x20U | sequence));
}

static void test_tx_fc_bs_zero(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    uint8_t data[27];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 0U, 0U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert_classic_cf(&transport, &frame, 0U, 2U);
    assert_classic_cf(&transport, &frame, 0U, 3U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_SEND_CONSECUTIVE);
    assert(isotp_tx_next(&transport, 0U, &frame) == ISOTP_COMPLETE);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_IDLE);
}

static void test_tx_fc_bs_one(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    uint8_t data[20];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 1U, 0U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    assert(isotp_tx_next(&transport, 0U, &frame) == ISOTP_OK);
    accept_classic_cts(&transport, 1U, 0U, 1U);
    assert_classic_cf(&transport, &frame, 1U, 2U);
    assert(isotp_tx_next(&transport, 1U, &frame) == ISOTP_COMPLETE);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_IDLE);
}

static void test_tx_fc_bs_three(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    uint8_t data[27];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 3U, 0U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert_classic_cf(&transport, &frame, 0U, 2U);
    assert_classic_cf(&transport, &frame, 0U, 3U);
    assert(isotp_tx_state(&transport) != ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    assert(isotp_tx_next(&transport, 0U, &frame) == ISOTP_COMPLETE);
}

static void test_tx_fc_bs_larger_than_remaining(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    uint8_t data[27];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 255U, 0U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert_classic_cf(&transport, &frame, 0U, 2U);
    assert_classic_cf(&transport, &frame, 0U, 3U);
    assert(isotp_tx_next(&transport, 0U, &frame) == ISOTP_COMPLETE);
}

static void test_tx_fc_bs_changes_between_blocks(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    uint8_t data[48];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 3U, 0U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert_classic_cf(&transport, &frame, 0U, 2U);
    assert_classic_cf(&transport, &frame, 0U, 3U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    accept_classic_cts(&transport, 2U, 0U, 1U);
    assert_classic_cf(&transport, &frame, 1U, 4U);
    assert_classic_cf(&transport, &frame, 1U, 5U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    accept_classic_cts(&transport, 4U, 0U, 2U);
    assert_classic_cf(&transport, &frame, 2U, 6U);
    assert(isotp_tx_next(&transport, 2U, &frame) == ISOTP_COMPLETE);
}

static void test_tx_fc_wait_does_not_consume_bs(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    IsoTpCanFrame wait;
    uint8_t data[48];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 3U, 0U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert_classic_cf(&transport, &frame, 0U, 2U);
    assert_classic_cf(&transport, &frame, 0U, 3U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    assert(transport.block_count == 0U);
    wait = fc_frame(false, 0x7E0U, ISOTP_FC_WAIT, 0U, 0U);
    assert(isotp_tx_feed_flow_control(&transport, &wait, 1U) == ISOTP_OK);
    assert(isotp_tx_feed_flow_control(&transport, &wait, 2U) == ISOTP_OK);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    assert(transport.block_count == 0U);
    accept_classic_cts(&transport, 3U, 0U, 3U);
    assert(transport.block_count == 0U);
    assert_classic_cf(&transport, &frame, 3U, 4U);
    assert(transport.block_count == 1U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_SEND_CONSECUTIVE);
}

static void test_tx_fc_cts_resets_bs_counter(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    uint8_t data[48];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 3U, 0U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert_classic_cf(&transport, &frame, 0U, 2U);
    assert_classic_cf(&transport, &frame, 0U, 3U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    accept_classic_cts(&transport, 1U, 0U, 1U);
    assert(transport.block_count == 0U);
    assert(transport.remote_block_size == 1U);
    assert_classic_cf(&transport, &frame, 1U, 4U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
}

static void test_tx_bs_preserves_sequence_number(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    uint8_t data[34];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 2U, 0U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert_classic_cf(&transport, &frame, 0U, 2U);
    assert(isotp_tx_state(&transport) == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL);
    accept_classic_cts(&transport, 2U, 0U, 1U);
    assert_classic_cf(&transport, &frame, 1U, 3U);
    assert_classic_cf(&transport, &frame, 1U, 4U);
    assert(isotp_tx_next(&transport, 1U, &frame) == ISOTP_COMPLETE);
}

static void test_tx_bs_respects_stmin(void) {
    IsoTpConfig config;
    IsoTpTx transport;
    IsoTpCanFrame frame;
    uint8_t data[27];
    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 0U, 10U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert(isotp_tx_next(&transport, 9U, &frame) == ISOTP_OK);
    assert_classic_cf(&transport, &frame, 10U, 2U);
    assert(isotp_tx_next(&transport, 19U, &frame) == ISOTP_OK);
    assert_classic_cf(&transport, &frame, 20U, 3U);
    assert(isotp_tx_next(&transport, 30U, &frame) == ISOTP_COMPLETE);

    start_classic_payload(&transport, &config, data, sizeof(data), &frame);
    accept_classic_cts(&transport, 3U, 10U, 0U);
    assert_classic_cf(&transport, &frame, 0U, 1U);
    assert(isotp_tx_next(&transport, 9U, &frame) == ISOTP_OK);
    assert_classic_cf(&transport, &frame, 10U, 2U);
    assert(isotp_tx_next(&transport, 19U, &frame) == ISOTP_OK);
    assert_classic_cf(&transport, &frame, 20U, 3U);
    assert(isotp_tx_next(&transport, 30U, &frame) == ISOTP_COMPLETE);
}

static void test_flow_control(void) {
    test_flow_control_profile(false);
    test_flow_control_profile(true);
}

static void test_timeouts_and_sequence(void) {
    IsoTpConfig config;
    IsoTpCanFrame frame = {0};
    IsoTpRxEvent event;
    uint8_t data[32] = {0};
    isotp_config_classic_can(&config);
    config.tx_timeout_ms = 10U;
    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);
    assert(isotp_tx_start(&tx, data, sizeof(data), 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert(isotp_tx_tick(&tx, 9U) == ISOTP_OK);
    assert(isotp_tx_tick(&tx, 10U) == ISOTP_ERR_TIMEOUT);
    assert(isotp_tx_state(&tx) == ISOTP_TX_STATE_IDLE);

    config.rx_timeout_ms = 10U;
    isotp_rx_init(&rx, &config, 0x7E8U, 0x7E0U);
    frame.can_id = 0x7E8U;
    frame.dlc = 8U;
    frame.data[0] = 0x10U;
    frame.data[1] = 20U;
    assert(isotp_rx_feed(&rx, &frame, 0U, &event) == ISOTP_NEED_FLOW_CONTROL);
    assert(isotp_rx_tick(&rx, 10U) == ISOTP_ERR_TIMEOUT);
    assert(!rx.active);

    isotp_rx_init(&rx, &config, 0x7E8U, 0x7E0U);
    assert(isotp_rx_feed(&rx, &frame, 0U, &event) == ISOTP_NEED_FLOW_CONTROL);
    frame.data[0] = 0x22U;
    frame.dlc = 8U;
    assert(isotp_rx_feed(&rx, &frame, 1U, &event) == ISOTP_ERR_SEQUENCE);
    assert(!rx.active);
}

int main(void) {
    test_classic();
    test_fd_single_frames();
    test_fd_extended_first_frame();
    test_flow_control();
    test_tx_fc_bs_zero();
    test_tx_fc_bs_one();
    test_tx_fc_bs_three();
    test_tx_fc_bs_larger_than_remaining();
    test_tx_fc_bs_changes_between_blocks();
    test_tx_fc_wait_does_not_consume_bs();
    test_tx_fc_cts_resets_bs_counter();
    test_tx_bs_preserves_sequence_number();
    test_tx_bs_respects_stmin();
    test_timeouts_and_sequence();
    return 0;
}
