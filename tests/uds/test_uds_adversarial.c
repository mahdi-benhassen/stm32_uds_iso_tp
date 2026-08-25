/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "isotp.h"
#include "uds.h"
#include "uds_security_provider.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static IsoTpCanFrame frame(uint32_t id, uint8_t dlc, const uint8_t *data) {
    IsoTpCanFrame result = {0};
    result.can_id = id;
    result.dlc = dlc;
    if ((data != NULL) && (dlc <= ISOTP_MAX_FRAME_DATA)) {
        (void)memcpy(result.data, data, dlc);
    }
    return result;
}

static IsoTpConfig adversarial_config(void) {
    IsoTpConfig config;
    isotp_config_default(&config);
    config.block_size = 1U;
    config.rx_timeout_ms = 10U;
    config.tx_timeout_ms = 10U;
    config.max_wait_frames = 3U;
    return config;
}

static void test_isotp_rx_adversarial(void) {
    IsoTpConfig config = adversarial_config();
    IsoTpRx rx;
    IsoTpRxEvent event;
    const uint8_t malformed_sf[] = {0x00U};
    const uint8_t invalid_ff_length[] = {0x10U, 0x07U, 0U, 0U, 0U, 0U, 0U, 0U};
    const uint8_t valid_ff[] = {0x10U, 0x14U, 0U, 1U, 2U, 3U, 4U, 5U};
    const uint8_t cf_sequence_1[] = {0x21U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};
    const uint8_t cf_duplicate[] = {0x21U, 13U, 14U, 15U, 16U, 17U, 18U, 19U};
    const uint8_t invalid_pci[] = {0x40U, 0U};
    IsoTpCanFrame input;

    isotp_rx_init(&rx, &config, 0x7E0U, 0x7E8U);
    input = frame(0x7E0U, 1U, malformed_sf);
    assert(isotp_rx_feed(&rx, &input, 0U, &event) == ISOTP_ERR_FORMAT);
    input = frame(0x7E0U, 8U, invalid_ff_length);
    assert(isotp_rx_feed(&rx, &input, 0U, &event) == ISOTP_ERR_FORMAT);
    input = frame(0x7E0U, 2U, invalid_pci);
    assert(isotp_rx_feed(&rx, &input, 0U, &event) == ISOTP_ERR_FORMAT);
    input.dlc = 9U;
    assert(isotp_rx_feed(&rx, &input, 0U, &event) == ISOTP_ERR_ARGUMENT);

    input = frame(0x7E0U, 8U, valid_ff);
    assert(isotp_rx_feed(&rx, &input, 0U, &event) == ISOTP_NEED_FLOW_CONTROL);
    assert(event.has_flow_control && event.flow_control.can_id == 0x7E8U);
    input = frame(0x7E0U, 8U, cf_sequence_1);
    assert(isotp_rx_feed(&rx, &input, 1U, &event) == ISOTP_NEED_FLOW_CONTROL);
    assert(event.has_flow_control);
    input = frame(0x7E0U, 8U, cf_duplicate);
    assert(isotp_rx_feed(&rx, &input, 2U, &event) == ISOTP_ERR_SEQUENCE);

    input = frame(0x7E0U, 8U, valid_ff);
    assert(isotp_rx_feed(&rx, &input, 0U, &event) == ISOTP_NEED_FLOW_CONTROL);
    assert(isotp_rx_tick(&rx, 11U) == ISOTP_ERR_TIMEOUT); /* missing CF/tester timeout */
    input = frame(0x701U, 1U, (const uint8_t[]){0U});
    assert(isotp_rx_feed(&rx, &input, 0U, &event) == ISOTP_OK);
}

static void test_isotp_tx_adversarial(void) {
    IsoTpConfig config = adversarial_config();
    IsoTpTx tx;
    IsoTpCanFrame output;
    const uint8_t payload[ISOTP_MAX_PAYLOAD] = {0U};
    const uint8_t bad_stmin[] = {0x30U, 0U, 0x80U};
    const uint8_t overflow_fc[] = {0x32U, 0U, 0U};
    const uint8_t wait_fc[] = {0x31U, 0U, 0U};
    const uint8_t cts_bs_zero[] = {0x30U, 0U, 0U};
    IsoTpCanFrame flow_control;

    isotp_tx_init(&tx, &config, 0x7E0U, 0x7E8U);
    assert(isotp_tx_start(&tx, payload, 0U, 0U, &output) == ISOTP_ERR_ARGUMENT); /* zero payload */
    assert(isotp_tx_start(&tx, payload, sizeof(payload), 0U, &output) == ISOTP_TX_FRAME_READY);
    flow_control = frame(0x7E0U, 3U, bad_stmin);
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 0U) == ISOTP_ERR_FLOW_CONTROL);

    assert(isotp_tx_start(&tx, payload, sizeof(payload), 0U, &output) == ISOTP_TX_FRAME_READY);
    flow_control = frame(0x7E0U, 3U, overflow_fc);
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 0U) ==
           ISOTP_ERR_FLOW_CONTROL); /* FC overflow */

    assert(isotp_tx_start(&tx, payload, sizeof(payload), 0U, &output) == ISOTP_TX_FRAME_READY);
    flow_control.can_id = 0x123U;
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 0U) ==
           ISOTP_OK); /* unexpected FC ID ignored */
    flow_control = frame(0x7E0U, 3U, wait_fc);
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 0U) == ISOTP_OK);
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 1U) == ISOTP_OK);
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 2U) == ISOTP_OK);
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 3U) ==
           ISOTP_ERR_FLOW_CONTROL); /* WAIT flood */

    assert(isotp_tx_start(&tx, payload, sizeof(payload), 0U, &output) == ISOTP_TX_FRAME_READY);
    flow_control = frame(0x7E0U, 3U, cts_bs_zero);
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 0U) == ISOTP_OK); /* BS=0 */
    assert(isotp_tx_next(&tx, 0U, &output) == ISOTP_TX_FRAME_READY);
    assert(isotp_tx_tick(&tx, 11U) == ISOTP_ERR_TIMEOUT);

    assert(isotp_tx_start(&tx, payload, (uint16_t)(sizeof(payload) + 1U), 0U, &output) ==
           ISOTP_ERR_OVERFLOW); /* maximum-plus-one overflow */
    assert(isotp_tx_start(&tx, payload, sizeof(payload), 0U, &output) == ISOTP_TX_FRAME_READY);
    flow_control = frame(0x7E0U, 3U, cts_bs_zero);
    assert(isotp_tx_feed_flow_control(&tx, &flow_control, 0U) == ISOTP_OK);
}

typedef struct {
    UdsSecurityProvider provider;
    uint32_t now_ms;
} SecurityContext;

static UdsCallbackResult invalid_did_read(void *context, uint16_t did, uint8_t *data,
                                          uint16_t *length, uint16_t capacity) {
    (void)context;
    (void)did;
    (void)data;
    (void)length;
    (void)capacity;
    return UDS_RESULT_OUT_OF_RANGE;
}

static UdsCallbackResult security_seed(void *context, uint8_t level, uint8_t *seed,
                                       uint16_t *length, uint16_t capacity) {
    SecurityContext *state = (SecurityContext *)context;
    UdsSecurityResult result = uds_security_provider_generate_seed(&state->provider, level, seed,
                                                                   length, capacity, state->now_ms);
    return (result == UDS_SECURITY_OK) ? UDS_RESULT_OK : UDS_RESULT_DELAY_ACTIVE;
}

static UdsCallbackResult security_key(void *context, uint8_t level, const uint8_t *key,
                                      uint16_t length) {
    SecurityContext *state = (SecurityContext *)context;
    UdsSecurityResult result =
        uds_security_provider_verify_key(&state->provider, level, key, length, state->now_ms);
    switch (result) {
    case UDS_SECURITY_OK:
        return UDS_RESULT_OK;
    case UDS_SECURITY_INVALID_KEY:
        return UDS_RESULT_INVALID_KEY;
    case UDS_SECURITY_ATTEMPTS_EXCEEDED:
        return UDS_RESULT_ATTEMPTS_EXCEEDED;
    case UDS_SECURITY_DELAY_ACTIVE:
        return UDS_RESULT_DELAY_ACTIVE;
    default:
        return UDS_RESULT_DENIED;
    }
}

static void test_uds_adversarial(void) {
    UdsServer server;
    UdsCallbacks callbacks = {0};
    SecurityContext security = {0};
    uint8_t response[32] = {0};
    uint16_t response_len = 0U;
    const uint8_t invalid_sid[] = {0x00U};
    const uint8_t unsupported_sid[] = {0x99U};
    const uint8_t invalid_did[] = {0x22U, 0xFFU, 0xFFU};
    const uint8_t invalid_session[] = {0x10U, 0x7FU};
    const uint8_t seed_request[] = {0x27U, 0x01U};
    const uint8_t bad_key[] = {0x27U, 0x02U, 0U, 0U, 0U, 0U};

    uds_security_provider_init(&security.provider, 0x12345678UL, 3U, 100U);
    callbacks.read_did = invalid_did_read;
    callbacks.security_seed = security_seed;
    callbacks.security_key = security_key;
    uds_server_init(&server, &callbacks, &security, 0U);

    assert(uds_server_handle(&server, invalid_sid, sizeof(invalid_sid), response, &response_len,
                             sizeof(response), 0U) == UDS_RESULT_OK);
    assert(response_len == 3U && response[2] == UDS_NRC_SERVICE_NOT_SUPPORTED);
    response_len = 0U;
    assert(uds_server_handle(&server, unsupported_sid, sizeof(unsupported_sid), response,
                             &response_len, sizeof(response), 0U) == UDS_RESULT_OK);
    assert(response[2] == UDS_NRC_SERVICE_NOT_SUPPORTED);
    response_len = 0U;
    assert(uds_server_handle(&server, invalid_did, sizeof(invalid_did), response, &response_len,
                             sizeof(response), 0U) == UDS_RESULT_OK);
    assert(response[2] == UDS_NRC_REQUEST_OUT_OF_RANGE);
    response_len = 0U;
    assert(uds_server_handle(&server, invalid_session, sizeof(invalid_session), response,
                             &response_len, sizeof(response), 0U) == UDS_RESULT_OK);
    assert(response[2] == UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);

    response_len = 0U;
    assert(uds_server_handle(&server, seed_request, sizeof(seed_request), response, &response_len,
                             sizeof(response), 0U) == UDS_RESULT_OK);
    assert(response[0] == 0x67U);
    response_len = 0U;
    assert(uds_server_handle(&server, bad_key, sizeof(bad_key), response, &response_len,
                             sizeof(response), 1U) == UDS_RESULT_OK);
    assert(response[0] == 0x7FU && response[2] == UDS_NRC_INVALID_KEY);
    response_len = 0U;
    assert(uds_server_handle(&server, seed_request, sizeof(seed_request), response, &response_len,
                             sizeof(response), 1U) == UDS_RESULT_OK);
    response_len = 0U;
    assert(uds_server_handle(&server, bad_key, sizeof(bad_key), response, &response_len,
                             sizeof(response), 2U) == UDS_RESULT_OK);
    assert(response[2] == UDS_NRC_INVALID_KEY);
    response_len = 0U;
    assert(uds_server_handle(&server, seed_request, sizeof(seed_request), response, &response_len,
                             sizeof(response), 2U) == UDS_RESULT_OK);
    response_len = 0U;
    assert(uds_server_handle(&server, bad_key, sizeof(bad_key), response, &response_len,
                             sizeof(response), 3U) == UDS_RESULT_OK);
    assert(response[2] == UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS);
    response_len = 0U;
    assert(uds_server_handle(&server, bad_key, sizeof(bad_key), response, &response_len,
                             sizeof(response), 4U) == UDS_RESULT_OK);
    assert(response[2] == UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED);
}

int main(void) {
    test_isotp_rx_adversarial();
    test_isotp_tx_adversarial();
    test_uds_adversarial();
    return 0;
}
