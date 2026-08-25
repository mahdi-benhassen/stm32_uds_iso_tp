/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds_iso_tp/isotp.h"
#include "uds_iso_tp/uds.h"

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static UdsCallbackResult read_did(void *context, uint16_t did, uint8_t *data, uint16_t *length,
                                  uint16_t capacity) {
    (void)context;
    if (did != 0xF190U) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    static const uint8_t value[] = {'F', '7', '6', '7', 'R', 'E', 'F'};
    if (capacity < sizeof(value)) {
        return UDS_RESULT_RESPONSE_TOO_LONG;
    }
    memcpy(data, value, sizeof(value));
    *length = (uint16_t)sizeof(value);
    return UDS_RESULT_OK;
}

static UdsCallbackResult security_seed(void *context, uint8_t level, uint8_t *seed,
                                       uint16_t *length, uint16_t capacity) {
    (void)context;
    if ((level != 1U) || (capacity < 2U)) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    seed[0] = 0x12U;
    seed[1] = 0x34U;
    *length = 2U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult security_key(void *context, uint8_t level, const uint8_t *key,
                                      uint16_t length) {
    (void)context;
    if ((level != 1U) || (length != 2U)) {
        return UDS_RESULT_INVALID_KEY;
    }
    return ((key[0] == 0xCAU) && (key[1] == 0xFEU)) ? UDS_RESULT_OK : UDS_RESULT_INVALID_KEY;
}

static UdsCallbackResult ecu_reset(void *context, uint8_t subfunction) {
    (void)context;
    return (subfunction == 1U) ? UDS_RESULT_OK : UDS_RESULT_OUT_OF_RANGE;
}

static UdsCallbackResult communication_control(void *context, uint8_t subfunction,
                                               uint8_t communication_type) {
    (void)context;
    return ((subfunction == 0U) && (communication_type == 0x01U)) ? UDS_RESULT_OK
                                                                  : UDS_RESULT_OUT_OF_RANGE;
}

static UdsCallbackResult routine_control(void *context, uint8_t subfunction, uint16_t routine_id,
                                         const uint8_t *request, uint16_t request_len,
                                         uint8_t *response, uint16_t *response_len,
                                         uint16_t capacity) {
    (void)context;
    (void)request;
    if ((subfunction != 1U) || (routine_id != 0x0203U) || (request_len != 1U) || (capacity < 1U)) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    response[0] = 0xAAU;
    *response_len = 1U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult request_download(void *context, uint32_t address, uint32_t length,
                                          uint16_t *max_block_length) {
    (void)context;
    if ((address != 0x08080000UL) || (length != 0x1000UL)) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    *max_block_length = 0x0100U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult transfer_data(void *context, uint8_t block, const uint8_t *data,
                                       uint16_t length) {
    (void)context;
    (void)data;
    return ((block != 0U) && (length > 0U)) ? UDS_RESULT_OK : UDS_RESULT_OUT_OF_RANGE;
}

static UdsCallbackResult transfer_exit(void *context, const uint8_t *request, uint16_t request_len,
                                       uint8_t *response, uint16_t *response_len,
                                       uint16_t capacity) {
    (void)context;
    (void)request;
    if ((request_len != 0U) || (capacity < 1U)) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    response[0] = 0x55U;
    *response_len = 1U;
    return UDS_RESULT_OK;
}

static void test_isotp(void) {
    IsoTpConfig config;
    isotp_config_default(&config);
    config.block_size = 2U;
    IsoTpRx rx;
    IsoTpTx tx;
    isotp_rx_init(&rx, &config, 0x7E8U, 0x7E0U);
    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);

    IsoTpCanFrame frame;
    uint8_t payload[] = {0x22U, 0xF1U, 0x90U};
    assert(isotp_tx_start(&tx, payload, sizeof(payload), 0U, &frame) == ISOTP_TX_FRAME_READY);
    assert(frame.can_id == 0x7E8U && frame.dlc == 4U && frame.data[0] == 3U);
    IsoTpRxEvent event;
    assert(isotp_rx_feed(&rx, &frame, 0U, &event) == ISOTP_COMPLETE);
    assert(event.length == sizeof(payload));
    assert(memcmp(event.payload, payload, sizeof(payload)) == 0);

    uint8_t long_payload[20];
    for (size_t index = 0U; index < sizeof(long_payload); ++index) {
        long_payload[index] = (uint8_t)index;
    }
    assert(isotp_tx_start(&tx, long_payload, sizeof(long_payload), 0U, &frame) ==
           ISOTP_TX_FRAME_READY);
    assert((frame.data[0] >> 4U) == 1U);
    assert(isotp_rx_feed(&rx, &frame, 0U, &event) == ISOTP_NEED_FLOW_CONTROL);
    assert(event.has_flow_control && event.flow_control.data[0] == 0x30U);
    assert(isotp_tx_feed_flow_control(&tx, &event.flow_control, 0U) == ISOTP_OK);
    uint8_t expected_sequence = 1U;
    while (isotp_tx_state(&tx) != ISOTP_TX_STATE_IDLE) {
        IsoTpStatus status = isotp_tx_next(&tx, 0U, &frame);
        if (status == ISOTP_OK) {
            status = isotp_tx_next(&tx, 1U, &frame);
        }
        if (status == ISOTP_COMPLETE) {
            break;
        }
        assert(status == ISOTP_TX_FRAME_READY);
        assert((frame.data[0] & 0x0FU) == expected_sequence);
        expected_sequence = (uint8_t)((expected_sequence + 1U) & 0x0FU);
        assert(isotp_rx_feed(&rx, &frame, 1U, &event) == ((rx.active) ? ISOTP_OK : ISOTP_COMPLETE));
        if (event.has_flow_control) {
            assert(isotp_tx_feed_flow_control(&tx, &event.flow_control, 1U) == ISOTP_OK);
        }
    }
    assert(event.length == sizeof(long_payload));
    assert(memcmp(event.payload, long_payload, sizeof(long_payload)) == 0);

    isotp_rx_init(&rx, &config, 0x7E8U, 0x7E0U);
    frame.can_id = 0x7E8U;
    frame.dlc = 8U;
    frame.data[0] = 0x10U;
    frame.data[1] = 8U;
    memset(&frame.data[2], 0, 6U);
    assert(isotp_rx_feed(&rx, &frame, 0U, &event) == ISOTP_NEED_FLOW_CONTROL);
    frame.data[0] = 0x22U;
    frame.dlc = 2U;
    assert(isotp_rx_feed(&rx, &frame, 1U, &event) == ISOTP_ERR_SEQUENCE);
    assert(isotp_rx_tick(&rx, 2000U) == ISOTP_OK);
}

static void test_uds(void) {
    UdsCallbacks callbacks = {
        .read_did = read_did,
        .security_seed = security_seed,
        .security_key = security_key,
        .ecu_reset = ecu_reset,
        .communication_control = communication_control,
        .routine_control = routine_control,
        .request_download = request_download,
        .transfer_data = transfer_data,
        .request_transfer_exit = transfer_exit,
    };
    UdsServer server;
    uds_server_init(&server, &callbacks, NULL, 0U);
    uint8_t response[64];
    uint16_t response_len;

    uint8_t request[] = {0x10U, 0x03U};
    assert(uds_server_handle(&server, request, sizeof(request), response, &response_len,
                             sizeof(response), 0U) == UDS_RESULT_OK);
    assert(response_len == 6U && response[0] == 0x50U && response[1] == 0x03U);

    uint8_t read_request[] = {0x22U, 0xF1U, 0x90U};
    assert(uds_server_handle(&server, read_request, sizeof(read_request), response, &response_len,
                             sizeof(response), 1U) == UDS_RESULT_OK);
    assert(response_len == 10U && response[0] == 0x62U && response[1] == 0xF1U &&
           response[2] == 0x90U);

    uint8_t bad_did[] = {0x22U, 0x12U, 0x34U};
    assert(uds_server_handle(&server, bad_did, sizeof(bad_did), response, &response_len,
                             sizeof(response), 2U) == UDS_RESULT_OK);
    assert(response_len == 3U && response[0] == 0x7FU && response[2] == 0x31U);

    uds_server_apply_reset(&server, UDS_RESET_PROGRAMMING, 10000U);
    uint8_t session_request[] = {0x10U, UDS_SESSION_EXTENDED};
    assert(uds_server_handle(&server, session_request, sizeof(session_request), response,
                             &response_len, sizeof(response), 10000U) == UDS_RESULT_OK);
    uint8_t seed_request[] = {0x27U, 0x01U};
    assert(uds_server_handle(&server, seed_request, sizeof(seed_request), response, &response_len,
                             sizeof(response), 10001U) == UDS_RESULT_OK);
    assert(response_len == 4U && response[0] == 0x67U && response[2] == 0x12U);
    uint8_t key_request[] = {0x27U, 0x02U, 0xCAU, 0xFEU};
    assert(uds_server_handle(&server, key_request, sizeof(key_request), response, &response_len,
                             sizeof(response), 10002U) == UDS_RESULT_OK);
    assert(uds_server_security_level(&server) == 1U);

    uint8_t reset_request[] = {0x11U, 0x01U};
    assert(uds_server_handle(&server, reset_request, sizeof(reset_request), response, &response_len,
                             sizeof(response), 5U) == UDS_RESULT_OK);
    assert(uds_server_reset_pending(&server));
    uds_server_clear_reset(&server);
    assert(!uds_server_reset_pending(&server));

    uint8_t communication_request[] = {0x28U, 0x00U, 0x01U};
    assert(uds_server_handle(&server, communication_request, sizeof(communication_request),
                             response, &response_len, sizeof(response), 6U) == UDS_RESULT_OK);
    assert(response[0] == 0x68U);

    uint8_t routine_request[] = {0x31U, 0x01U, 0x02U, 0x03U, 0xAAU};
    assert(uds_server_handle(&server, routine_request, sizeof(routine_request), response,
                             &response_len, sizeof(response), 7U) == UDS_RESULT_OK);
    assert(response_len == 5U && response[0] == 0x71U && response[4] == 0xAAU);

    uint8_t download_request[] = {0x34U, 0x44U, 0x08U, 0x08U, 0x00U,
                                  0x00U, 0x00U, 0x00U, 0x10U, 0x00U};
    assert(uds_server_handle(&server, download_request, sizeof(download_request), response,
                             &response_len, sizeof(response), 8U) == UDS_RESULT_OK);
    assert(response[0] == 0x74U && server.download_active);
    uint8_t transfer_request[] = {0x36U, 0x01U, 0xAAU, 0xBBU};
    assert(uds_server_handle(&server, transfer_request, sizeof(transfer_request), response,
                             &response_len, sizeof(response), 9U) == UDS_RESULT_OK);
    assert(response[0] == 0x76U);
    uint8_t exit_request[] = {0x37U};
    assert(uds_server_handle(&server, exit_request, sizeof(exit_request), response, &response_len,
                             sizeof(response), 10U) == UDS_RESULT_OK);
    assert(response_len == 2U && response[0] == 0x77U && response[1] == 0x55U);

    uint8_t unsupported[] = {0x99U};
    assert(uds_server_handle(&server, unsupported, sizeof(unsupported), response, &response_len,
                             sizeof(response), 11U) == UDS_RESULT_OK);
    assert(response[0] == 0x7FU && response[1] == 0x99U && response[2] == 0x11U);
}

int main(void) {
    test_isotp();
    test_uds();
    return 0;
}
