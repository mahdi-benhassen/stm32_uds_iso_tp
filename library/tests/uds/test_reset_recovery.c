#include "uds_iso_tp/endpoint.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    IsoTpCanFrame last_frame;
    uint32_t frame_count;
    uint16_t reset_count;
} ResetRecoveryBus;

static bool send_frame(void *context, const IsoTpCanFrame *frame) {
    ResetRecoveryBus *bus = (ResetRecoveryBus *)context;
    if ((bus == NULL) || (frame == NULL))
        return false;
    bus->last_frame = *frame;
    bus->frame_count++;
    return true;
}

static bool tx_complete(void *context) {
    (void)context;
    return true;
}

static uint32_t clock_ms(void *context) {
    (void)context;
    return 0U;
}

static UdsCallbackResult reset_prepare(void *context, uint8_t subfunction) {
    (void)context;
    return (subfunction == UDS_RESET_TYPE_HARD) ? UDS_RESULT_OK : UDS_RESULT_OUT_OF_RANGE;
}

static void reset_execute(void *context, uint8_t subfunction) {
    ResetRecoveryBus *bus = (ResetRecoveryBus *)context;
    assert((bus != NULL) && (subfunction == UDS_RESET_TYPE_HARD));
    bus->reset_count++;
}

static UdsCallbackResult read_did(void *context, uint16_t did, uint8_t *data, uint16_t *length,
                                  uint16_t capacity) {
    (void)context;
    if ((did != 0xF190U) || (capacity < 12U))
        return UDS_RESULT_OUT_OF_RANGE;
    for (uint8_t index = 0U; index < 12U; ++index)
        data[index] = (uint8_t)(0xA0U + index);
    *length = 12U;
    return UDS_RESULT_OK;
}

static UdsIsoTpEndpointConfig make_config(ResetRecoveryBus *bus) {
    UdsIsoTpEndpointConfig config = {0};
    isotp_config_classic_can(&config.isotp_config);
    config.send_frame = send_frame;
    config.tx_complete = tx_complete;
    config.clock_ms = clock_ms;
    config.context = bus;
    config.request_id = 0x7E0U;
    config.response_id = 0x7E8U;
    config.uds_callbacks.ecu_reset = reset_prepare;
    config.uds_callbacks.ecu_reset_execute = reset_execute;
    config.uds_callbacks.read_did = read_did;
    config.uds_context = bus;
    return config;
}

static IsoTpCanFrame request_frame(const uint8_t *data, uint8_t dlc) {
    IsoTpCanFrame frame = {0};
    frame.can_id = 0x7E0U;
    frame.dlc = dlc;
    (void)memcpy(frame.data, data, dlc);
    return frame;
}

static void reinitialize_after_reset(UdsIsoTpEndpoint *endpoint, ResetRecoveryBus *bus,
                                     uint32_t now_ms) {
    UdsIsoTpEndpointConfig config = make_config(bus);
    assert(uds_isotp_endpoint_init(endpoint, &config, now_ms));
}

static void expect_single_response(UdsIsoTpEndpoint *endpoint, ResetRecoveryBus *bus,
                                   const uint8_t *request, uint8_t request_length,
                                   uint8_t response_sid, uint32_t now_ms) {
    uint32_t before = bus->frame_count;
    IsoTpCanFrame frame = request_frame(request, request_length);
    assert(uds_isotp_endpoint_receive(endpoint, &frame, now_ms) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(endpoint, now_ms) == ISOTP_TX_FRAME_READY);
    assert(bus->frame_count == (before + 1U));
    assert((bus->last_frame.data[0] & 0x0FU) == bus->last_frame.data[0]);
    assert(bus->last_frame.data[1] == response_sid);
}

static void test_reset_and_post_reset_services(void) {
    ResetRecoveryBus bus = {0};
    UdsIsoTpEndpoint endpoint;
    UdsIsoTpEndpointConfig config = make_config(&bus);
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));

    const uint8_t reset_request[] = {0x02U, 0x11U, 0x01U};
    IsoTpCanFrame reset_frame = request_frame(reset_request, 3U);
    assert(uds_isotp_endpoint_receive(&endpoint, &reset_frame, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.last_frame.data[1] == 0x51U && bus.last_frame.data[2] == 0x01U);
    assert(bus.reset_count == 0U);
    assert(uds_isotp_endpoint_tick(&endpoint, 0U) == ISOTP_OK);
    assert(bus.reset_count == 1U);

    reinitialize_after_reset(&endpoint, &bus, 1U);
    const uint8_t session_request[] = {0x02U, 0x10U, 0x01U};
    expect_single_response(&endpoint, &bus, session_request, 3U, 0x50U, 1U);

    const uint8_t did_request[] = {0x03U, 0x22U, 0xF1U, 0x90U};
    uint32_t before = bus.frame_count;
    IsoTpCanFrame did_frame = request_frame(did_request, 4U);
    assert(uds_isotp_endpoint_receive(&endpoint, &did_frame, 2U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 2U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == (before + 1U));
    assert((bus.last_frame.data[0] >> 4U) == 1U);
    IsoTpCanFrame flow_control = request_frame((const uint8_t[]){0x30U, 0x00U, 0x00U}, 3U);
    assert(uds_isotp_endpoint_receive(&endpoint, &flow_control, 3U) == ISOTP_OK);
    while ((isotp_tx_state(&endpoint.tx) != ISOTP_TX_STATE_IDLE) || endpoint.tx_pending)
        (void)uds_isotp_endpoint_process(&endpoint, 3U);
    assert(bus.frame_count >= (before + 3U));

    const uint8_t tester_present[] = {0x02U, 0x3EU, 0x00U};
    expect_single_response(&endpoint, &bus, tester_present, 3U, 0x7EU, 4U);

    const uint8_t invalid_request[] = {0x02U, 0x99U, 0x00U};
    expect_single_response(&endpoint, &bus, invalid_request, 3U, 0x7FU, 5U);
    expect_single_response(&endpoint, &bus, session_request, 3U, 0x50U, 6U);
}

static void test_one_hundred_reset_cycles_and_one_thousand_requests(void) {
    ResetRecoveryBus bus = {0};
    UdsIsoTpEndpoint endpoint;
    UdsIsoTpEndpointConfig config = make_config(&bus);
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));

    const uint8_t reset_request[] = {0x02U, 0x11U, 0x01U};
    const uint8_t tester_present[] = {0x02U, 0x3EU, 0x00U};
    for (uint16_t cycle = 0U; cycle < 100U; ++cycle) {
        IsoTpCanFrame reset_frame = request_frame(reset_request, 3U);
        assert(uds_isotp_endpoint_receive(&endpoint, &reset_frame, cycle) == ISOTP_TX_FRAME_READY);
        assert(uds_isotp_endpoint_process(&endpoint, cycle) == ISOTP_TX_FRAME_READY);
        assert(uds_isotp_endpoint_tick(&endpoint, cycle) == ISOTP_OK);
        reinitialize_after_reset(&endpoint, &bus, (uint32_t)(cycle + 1U));
        for (uint8_t request = 0U; request < 10U; ++request)
            expect_single_response(&endpoint, &bus, tester_present, 3U, 0x7EU,
                                   (uint32_t)(cycle + request + 1U));
    }
    assert(bus.reset_count == 100U);
}

int main(void) {
    test_reset_and_post_reset_services();
    test_one_hundred_reset_cycles_and_one_thousand_requests();
    return 0;
}
