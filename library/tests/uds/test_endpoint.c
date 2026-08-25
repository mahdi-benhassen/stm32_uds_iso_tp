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
    while (endpoint.tx.active || endpoint.tx_pending) {
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
    assert(uds_isotp_endpoint_receive(&endpoint, &overflow, 1U) == ISOTP_ERR_FLOW_CONTROL);
    assert(!endpoint.tx.active);

    assert(uds_isotp_endpoint_receive(&endpoint, &request, 2U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 2U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_tick(&endpoint, 2001U) == ISOTP_ERR_TIMEOUT);
    assert(!endpoint.tx.active);
}

int main(void) {
    run_multiframe_profile(false, 20U);
    run_multiframe_profile(true, 64U);
    test_flow_control_error_and_timeout();
    return 0;
}
