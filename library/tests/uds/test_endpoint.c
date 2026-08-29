#include "uds_iso_tp/endpoint.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    IsoTpCanFrame frames[32];
    size_t count;
    unsigned int failures_remaining;
    uint8_t response_size;
    unsigned int reset_calls;
    UdsResetEvent reset_events[8];
    size_t reset_event_count;
} Sink;

static bool send_frame(void *context, const IsoTpCanFrame *frame) {
    Sink *sink = (Sink *)context;
    if (sink->failures_remaining > 0U) {
        --sink->failures_remaining;
        return false;
    }
    assert(sink->count < (sizeof(sink->frames) / sizeof(sink->frames[0])));
    sink->frames[sink->count] = *frame;
    ++sink->count;
    return true;
}

static uint32_t clock_ms(void *context) {
    (void)context;
    return 0U;
}

static UdsCallbackResult read_did(void *context, uint16_t did, uint8_t *data, uint16_t *length,
                                  uint16_t capacity) {
    Sink *sink = (Sink *)context;
    if ((did != 0xF190U) || (capacity < sink->response_size)) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    for (uint16_t index = 0U; index < sink->response_size; ++index) {
        data[index] = (uint8_t)(index ^ 0x5AU);
    }
    *length = sink->response_size;
    return UDS_RESULT_OK;
}

static IsoTpCanFrame tester_present(bool can_fd) {
    IsoTpCanFrame frame = {0};
    frame.can_id = 0x7E0U;
    frame.dlc = can_fd ? 8U : 3U;
    frame.is_fd = can_fd;
    frame.data[0] = 0x02U;
    frame.data[1] = 0x3EU;
    frame.data[2] = 0x00U;
    return frame;
}

static UdsCallbackResult ecu_reset_prepare(void *context, uint8_t subfunction) {
    (void)context;
    return (subfunction == 1U) ? UDS_RESULT_OK : UDS_RESULT_OUT_OF_RANGE;
}

static void ecu_reset_execute(void *context, uint8_t subfunction) {
    Sink *sink = (Sink *)context;
    assert(subfunction == 1U);
    ++sink->reset_calls;
}

static bool tx_complete_deferred(void *context) {
    (void)context;
    return false;
}

static void reset_event(void *context, UdsResetEvent event) {
    Sink *sink = (Sink *)context;
    assert(sink->reset_event_count < (sizeof(sink->reset_events) / sizeof(sink->reset_events[0])));
    sink->reset_events[sink->reset_event_count++] = event;
}

static IsoTpCanFrame flow_control(bool can_fd, uint8_t status) {
    IsoTpCanFrame frame = {0};
    frame.can_id = 0x7E0U;
    frame.dlc = can_fd ? 8U : 3U;
    frame.is_fd = can_fd;
    frame.data[0] = (uint8_t)(0x30U | status);
    frame.data[1] = 0U;
    frame.data[2] = 0U;
    return frame;
}

static void run_multiframe_profile(bool can_fd, uint8_t response_size) {
    IsoTpConfig transport;
    if (can_fd) {
        isotp_config_can_fd(&transport, 64U, 64U);
        transport.bit_rate_switch = true;
    } else {
        isotp_config_classic_can(&transport);
    }

    Sink sink = {0};
    sink.response_size = response_size;
    UdsCallbacks callbacks = {.read_did = read_did};
    UdsIsoTpEndpointConfig config = {
        .send_frame = send_frame,
        .clock_ms = clock_ms,
        .context = &sink,
        .isotp_config = transport,
        .request_id = 0x7E0U,
        .response_id = 0x7E8U,
        .uds_callbacks = callbacks,
        .uds_context = &sink,
    };
    UdsIsoTpEndpoint endpoint;
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));

    IsoTpCanFrame request = tester_present(can_fd);
    request.dlc = can_fd ? 8U : 4U;
    request.data[0] = 0x03U;
    request.data[1] = 0x22U;
    request.data[2] = 0xF1U;
    request.data[3] = 0x90U;
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);

    sink.failures_remaining = 1U;
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_OK);
    assert(sink.count == 0U);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(sink.count == 1U);
    assert(sink.frames[0].can_id == 0x7E8U);
    assert(sink.frames[0].is_fd == can_fd);
    assert(sink.frames[0].bit_rate_switch == can_fd);
    assert((sink.frames[0].data[0] >> 4U) == 1U);
    assert(sink.frames[0].data[1] == (uint8_t)(response_size + 3U));
    assert(sink.frames[0].dlc == (can_fd ? 64U : 8U));

    IsoTpCanFrame fc = flow_control(can_fd, ISOTP_FC_CTS);
    assert(uds_isotp_endpoint_receive(&endpoint, &fc, 1U) == ISOTP_OK);
    uint8_t sequence = 1U;
    while ((isotp_tx_state(&endpoint.tx) != ISOTP_TX_STATE_IDLE) || endpoint.tx_pending) {
        size_t before = sink.count;
        (void)uds_isotp_endpoint_process(&endpoint, 1U);
        if (sink.count == before) {
            continue;
        }
        const IsoTpCanFrame *emitted = &sink.frames[sink.count - 1U];
        assert((emitted->data[0] >> 4U) == 2U);
        assert((emitted->data[0] & 0x0FU) == sequence);
        assert(emitted->is_fd == can_fd);
        sequence = (uint8_t)((sequence + 1U) & 0x0FU);
    }
    assert(uds_isotp_endpoint_process(&endpoint, 1U) == ISOTP_OK);
}

static void test_deferred_reset_and_full_duplex(void) {
    IsoTpConfig transport;
    isotp_config_classic_can(&transport);
    isotp_config_set_full_duplex(&transport, true);

    Sink reset_sink = {0};
    UdsCallbacks reset_callbacks = {
        .ecu_reset = ecu_reset_prepare,
        .ecu_reset_execute = ecu_reset_execute,
    };
    UdsIsoTpEndpointConfig reset_config = {
        .send_frame = send_frame,
        .tx_complete = tx_complete_deferred,
        .clock_ms = clock_ms,
        .reset_event = reset_event,
        .context = &reset_sink,
        .reset_event_context = &reset_sink,
        .isotp_config = transport,
        .request_id = 0x7E0U,
        .response_id = 0x7E8U,
        .uds_callbacks = reset_callbacks,
        .uds_context = &reset_sink,
    };
    UdsIsoTpEndpoint reset_endpoint;
    UdsIsoTpEndpointConfig unsafe_reset_config = reset_config;
    unsafe_reset_config.tx_complete = NULL;
    assert(!uds_isotp_endpoint_init(&reset_endpoint, &unsafe_reset_config, 0U));
    assert(uds_isotp_endpoint_init(&reset_endpoint, &reset_config, 0U));
    IsoTpCanFrame reset_request = tester_present(false);
    reset_request.dlc = 3U;
    reset_request.data[0] = 0x02U;
    reset_request.data[1] = 0x11U;
    reset_request.data[2] = 0x01U;
    assert(uds_isotp_endpoint_receive(&reset_endpoint, &reset_request, 0U) == ISOTP_TX_FRAME_READY);
    assert(reset_sink.reset_event_count == 2U &&
           reset_sink.reset_events[0] == UDS_RESET_EVENT_REQUESTED &&
           reset_sink.reset_events[1] == UDS_RESET_EVENT_RESPONSE_READY);
    assert(uds_isotp_endpoint_process(&reset_endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(reset_sink.reset_event_count == 3U &&
           reset_sink.reset_events[2] == UDS_RESET_EVENT_TX_SUBMITTED);
    assert(reset_sink.count == 1U && reset_sink.frames[0].data[0] == 0x02U &&
           reset_sink.frames[0].data[1] == 0x51U && reset_sink.reset_calls == 0U);
    assert(uds_server_reset_pending(&reset_endpoint.uds));
    uds_isotp_endpoint_tx_complete(&reset_endpoint);
    assert(reset_sink.reset_calls == 0U && reset_sink.reset_event_count == 4U &&
           reset_sink.reset_events[3] == UDS_RESET_EVENT_TX_COMPLETE);
    assert(uds_server_reset_pending(&reset_endpoint.uds));
    assert(uds_isotp_endpoint_receive(&reset_endpoint, &reset_request, 0U) == ISOTP_OK);
    assert(reset_sink.reset_calls == 0U && reset_sink.count == 1U);
    assert(uds_isotp_endpoint_tick(&reset_endpoint, 0U) == ISOTP_OK);
    assert(reset_sink.reset_calls == 1U && reset_sink.reset_event_count == 5U &&
           reset_sink.reset_events[4] == UDS_RESET_EVENT_EXECUTED);
    assert(!uds_server_reset_pending(&reset_endpoint.uds));
    uds_isotp_endpoint_tx_complete(&reset_endpoint);
    assert(reset_sink.reset_calls == 1U && reset_sink.reset_event_count == 5U);

    reset_request.data[2] = 0x02U;
    assert(uds_isotp_endpoint_receive(&reset_endpoint, &reset_request, 1U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&reset_endpoint, 1U) == ISOTP_TX_FRAME_READY);
    assert(reset_sink.count == 2U && reset_sink.frames[1].data[0] == 0x03U &&
           reset_sink.frames[1].data[1] == 0x7FU && reset_sink.frames[1].data[2] == 0x11U &&
           reset_sink.frames[1].data[3] == UDS_NRC_REQUEST_OUT_OF_RANGE);
    assert(reset_sink.reset_calls == 1U);

    reset_request.data[1] = 0x11U;
    reset_request.data[2] = 0x81U;
    assert(uds_isotp_endpoint_receive(&reset_endpoint, &reset_request, 2U) == ISOTP_COMPLETE);
    assert(!reset_endpoint.tx_pending && reset_sink.reset_calls == 1U &&
           reset_sink.reset_event_count >= 6U && uds_server_reset_pending(&reset_endpoint.uds));
    assert(uds_isotp_endpoint_tick(&reset_endpoint, 2U) == ISOTP_OK);
    assert(reset_sink.reset_calls == 2U && reset_sink.reset_event_count >= 7U &&
           reset_sink.reset_events[reset_sink.reset_event_count - 1U] == UDS_RESET_EVENT_EXECUTED);

    Sink duplex_sink = {0};
    duplex_sink.response_size = 16U;
    UdsCallbacks duplex_callbacks = {.read_did = read_did};
    UdsIsoTpEndpointConfig duplex_config = {
        .send_frame = send_frame,
        .clock_ms = clock_ms,
        .context = &duplex_sink,
        .isotp_config = transport,
        .request_id = 0x7E0U,
        .response_id = 0x7E8U,
        .uds_callbacks = duplex_callbacks,
        .uds_context = &duplex_sink,
    };
    UdsIsoTpEndpoint duplex_endpoint;
    assert(uds_isotp_endpoint_init(&duplex_endpoint, &duplex_config, 0U));
    IsoTpCanFrame outgoing_request = tester_present(false);
    outgoing_request.dlc = 4U;
    outgoing_request.data[0] = 0x03U;
    outgoing_request.data[1] = 0x22U;
    outgoing_request.data[2] = 0xF1U;
    outgoing_request.data[3] = 0x90U;
    assert(uds_isotp_endpoint_receive(&duplex_endpoint, &outgoing_request, 0U) ==
           ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&duplex_endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(duplex_sink.count == 1U && (duplex_sink.frames[0].data[0] >> 4U) == 1U);

    IsoTpCanFrame inbound_first = {0};
    inbound_first.can_id = 0x7E0U;
    inbound_first.dlc = 8U;
    inbound_first.data[0] = 0x10U;
    inbound_first.data[1] = 10U;
    assert(uds_isotp_endpoint_receive(&duplex_endpoint, &inbound_first, 1U) ==
           ISOTP_NEED_FLOW_CONTROL);
    assert(duplex_endpoint.control_pending);
    assert(uds_isotp_endpoint_process(&duplex_endpoint, 1U) == ISOTP_TX_FRAME_READY);
    assert(duplex_sink.count == 2U && duplex_sink.frames[1].data[0] == 0x30U);

    IsoTpCanFrame outbound_fc = flow_control(false, ISOTP_FC_CTS);
    assert(uds_isotp_endpoint_receive(&duplex_endpoint, &outbound_fc, 2U) == ISOTP_OK);
    IsoTpCanFrame inbound_cf = {0};
    inbound_cf.can_id = 0x7E0U;
    inbound_cf.dlc = 5U;
    inbound_cf.data[0] = 0x21U;
    assert(uds_isotp_endpoint_receive(&duplex_endpoint, &inbound_cf, 2U) == ISOTP_TX_FRAME_READY);
    assert(duplex_endpoint.queued_response_pending);

    bool saw_outbound_cf = false;
    bool saw_queued_response = false;
    for (unsigned int iteration = 0U; iteration < 32U; ++iteration) {
        (void)uds_isotp_endpoint_process(&duplex_endpoint, 2U);
        if (duplex_sink.count == 0U)
            continue;
        const IsoTpCanFrame *emitted = &duplex_sink.frames[duplex_sink.count - 1U];
        if ((emitted->can_id == 0x7E8U) && ((emitted->data[0] >> 4U) == 2U))
            saw_outbound_cf = true;
        if ((emitted->can_id == 0x7E8U) && (emitted->data[0] == 0x03U) &&
            (emitted->data[1] == 0x7FU))
            saw_queued_response = true;
        if (saw_queued_response && (isotp_tx_state(&duplex_endpoint.tx) == ISOTP_TX_STATE_IDLE) &&
            !duplex_endpoint.tx_pending)
            break;
    }
    assert(saw_outbound_cf && saw_queued_response);
}

static void test_functional_endpoint_addressing(void) {
    IsoTpConfig transport;
    isotp_config_classic_can(&transport);
    Sink sink = {0};
    sink.response_size = 1U;
    UdsCallbacks callbacks = {.read_did = read_did};
    UdsIsoTpEndpointConfig config = {
        .send_frame = send_frame,
        .clock_ms = clock_ms,
        .context = &sink,
        .isotp_config = transport,
        .request_id = 0x7E0U,
        .response_id = 0x7E8U,
        .functional_request_id = 0x7DFU,
        .uds_callbacks = callbacks,
        .uds_context = &sink,
    };
    UdsIsoTpEndpoint endpoint;
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = tester_present(false);
    request.can_id = 0x7DFU;
    request.dlc = 4U;
    request.data[0] = 0x03U;
    request.data[1] = 0x22U;
    request.data[2] = 0xF1U;
    request.data[3] = 0x90U;
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(sink.count == 1U && sink.frames[0].can_id == 0x7E8U && sink.frames[0].dlc == 5U &&
           sink.frames[0].data[0] == 0x04U && sink.frames[0].data[1] == 0x62U);
}

static void test_flow_control_error_and_timeout(void) {
    IsoTpConfig transport;
    isotp_config_classic_can(&transport);
    Sink sink = {0};
    sink.response_size = 20U;
    UdsCallbacks callbacks = {.read_did = read_did};
    UdsIsoTpEndpointConfig config = {
        .send_frame = send_frame,
        .clock_ms = clock_ms,
        .context = &sink,
        .isotp_config = transport,
        .request_id = 0x7E0U,
        .response_id = 0x7E8U,
        .uds_callbacks = callbacks,
        .uds_context = &sink,
    };
    UdsIsoTpEndpoint endpoint;
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = tester_present(false);
    request.dlc = 4U;
    request.data[0] = 0x03U;
    request.data[1] = 0x22U;
    request.data[2] = 0xF1U;
    request.data[3] = 0x90U;
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    IsoTpCanFrame overflow = flow_control(false, ISOTP_FC_OVERFLOW);
    assert(uds_isotp_endpoint_receive(&endpoint, &overflow, 1U) == ISOTP_ERR_FLOW_OVERFLOW);
    assert(isotp_tx_state(&endpoint.tx) == ISOTP_TX_STATE_IDLE);

    assert(uds_isotp_endpoint_receive(&endpoint, &request, 2U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 2U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_tick(&endpoint, 2001U) == ISOTP_ERR_TIMEOUT);
    assert(isotp_tx_state(&endpoint.tx) == ISOTP_TX_STATE_IDLE);
}

int main(void) {
    run_multiframe_profile(false, 20U);
    run_multiframe_profile(true, 64U);
    test_deferred_reset_and_full_duplex();
    test_functional_endpoint_addressing();
    test_flow_control_error_and_timeout();
    return 0;
}
