#include "uds_iso_tp/endpoint.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    IsoTpCanFrame frames[256];
    uint16_t frame_count;
    uint16_t reset_count;
    bool tx_complete_release;
} ResetLifecycleBus;

static bool send_frame(void *context, const IsoTpCanFrame *frame) {
    ResetLifecycleBus *bus = (ResetLifecycleBus *)context;
    if ((bus == NULL) || (frame == NULL) || (bus->frame_count >= 256U))
        return false;
    bus->frames[bus->frame_count++] = *frame;
    return true;
}

static bool tx_complete(void *context) {
    ResetLifecycleBus *bus = (ResetLifecycleBus *)context;
    return (bus != NULL) && bus->tx_complete_release;
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
    ResetLifecycleBus *bus = (ResetLifecycleBus *)context;
    assert((bus != NULL) && (subfunction == UDS_RESET_TYPE_HARD));
    assert(bus->tx_complete_release);
    bus->reset_count++;
}

static UdsIsoTpEndpointConfig make_config(ResetLifecycleBus *bus) {
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

static void assert_endpoint_clean(const UdsIsoTpEndpoint *endpoint) {
    assert(endpoint != NULL);
    assert(!endpoint->rx.active);
    assert(isotp_tx_state(&endpoint->tx) == ISOTP_TX_STATE_IDLE);
    assert(!endpoint->tx_pending);
    assert(!endpoint->tx_in_flight);
    assert(!endpoint->control_pending);
    assert(!endpoint->queued_response_pending);
    assert(!endpoint->pending_reset_completion);
    assert(!endpoint->in_flight_reset_completion);
    assert(!endpoint->tx_reset_completion);
    assert(!endpoint->queued_reset_completion);
    assert(!uds_server_reset_pending(&endpoint->uds));
    assert(uds_server_session(&endpoint->uds) == UDS_SESSION_DEFAULT);
    assert(uds_server_security_level(&endpoint->uds) == 0U);
}

static void test_ecu_reset_transaction_lifecycle(void) {
    ResetLifecycleBus bus = {0};
    UdsIsoTpEndpoint endpoint = {0};
    UdsIsoTpEndpointConfig config = make_config(&bus);
    const uint8_t reset_request[] = {0x02U, 0x11U, 0x01U};
    const uint8_t session_request[] = {0x02U, 0x10U, 0x01U};

    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    assert_endpoint_clean(&endpoint);

    for (uint16_t cycle = 0U; cycle < 100U; ++cycle) {
        bus.tx_complete_release = false;
        IsoTpCanFrame reset_frame = request_frame(reset_request, 3U);
        assert(uds_isotp_endpoint_receive(&endpoint, &reset_frame, cycle) == ISOTP_TX_FRAME_READY);
        assert(uds_isotp_endpoint_process(&endpoint, cycle) == ISOTP_TX_FRAME_READY);

        assert(bus.frame_count == (uint16_t)(cycle * 2U + 1U));
        assert(bus.frames[bus.frame_count - 1U].data[1] == 0x51U);
        assert(bus.frames[bus.frame_count - 1U].data[2] == 0x01U);
        assert(bus.reset_count == cycle);
        assert(endpoint.tx_in_flight);
        assert(uds_server_reset_pending(&endpoint.uds));

        /* Queue acceptance and TX completion are deliberately separate. */
        bus.tx_complete_release = true;
        uds_isotp_endpoint_tx_complete(&endpoint);
        assert(bus.reset_count == (uint16_t)(cycle + 1U));
        assert_endpoint_clean(&endpoint);

        /* Model the MCU reboot: all endpoint and UDS state is initialized again. */
        assert(uds_isotp_endpoint_init(&endpoint, &config, (uint32_t)(cycle + 1U)));
        assert_endpoint_clean(&endpoint);

        /* No delay is inserted before the first post-reset request. */
        IsoTpCanFrame session_frame = request_frame(session_request, 3U);
        assert(uds_isotp_endpoint_receive(&endpoint, &session_frame, (uint32_t)(cycle + 1U)) ==
               ISOTP_TX_FRAME_READY);
        assert(uds_isotp_endpoint_process(&endpoint, (uint32_t)(cycle + 1U)) ==
               ISOTP_TX_FRAME_READY);
        assert(bus.frame_count == (uint16_t)(cycle * 2U + 2U));
        assert(bus.frames[bus.frame_count - 1U].data[1] == 0x50U);
        assert(bus.frames[bus.frame_count - 1U].data[2] == 0x01U);
        assert(!endpoint.tx_in_flight);
        assert(!uds_server_reset_pending(&endpoint.uds));
    }

    assert(bus.reset_count == 100U);
    assert(bus.frame_count == 200U);
}

int main(void) {
    test_ecu_reset_transaction_lifecycle();
    return 0;
}
