#include "stm32_fdcan_adapter.h"
#include "stm32f767_bxcan_adapter.h"

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    IsoTpCanFrame frames[8];
    uint8_t frame_count;
    bool fail_send;
    bool tx_error;
} FakeBus;

static bool fake_classic_send(void *context, uint32_t can_id, const uint8_t *data, uint8_t dlc) {
    FakeBus *bus = (FakeBus *)context;
    if ((bus == NULL) || bus->fail_send || (bus->frame_count >= 8U))
        return false;
    IsoTpCanFrame *frame = &bus->frames[bus->frame_count++];
    frame->can_id = can_id;
    frame->dlc = dlc;
    frame->is_fd = false;
    frame->bit_rate_switch = false;
    (void)memcpy(frame->data, data, dlc);
    return true;
}

static bool fake_fd_send(void *context, uint32_t can_id, const uint8_t *data, uint8_t dlc,
                         bool bit_rate_switch) {
    FakeBus *bus = (FakeBus *)context;
    if ((bus == NULL) || bus->fail_send || (bus->frame_count >= 8U))
        return false;
    IsoTpCanFrame *frame = &bus->frames[bus->frame_count++];
    frame->can_id = can_id;
    frame->dlc = dlc;
    frame->is_fd = true;
    frame->bit_rate_switch = bit_rate_switch;
    (void)memcpy(frame->data, data, dlc);
    return true;
}

static uint32_t fake_clock(void *context) {
    (void)context;
    return 0U;
}

static bool fake_tx_complete(void *context) {
    (void)context;
    return false;
}

static bool fake_tx_error(void *context) {
    Stm32F767BxCanBinding *binding = (Stm32F767BxCanBinding *)context;
    FakeBus *bus = (binding != NULL) ? (FakeBus *)binding->context : NULL;
    if ((bus == NULL) || !bus->tx_error)
        return false;
    bus->tx_error = false;
    return true;
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

static uint8_t reset_execute_count;

static UdsCallbackResult ecu_reset(void *context, uint8_t subfunction) {
    (void)context;
    return (subfunction == 0x01U) ? UDS_RESULT_OK : UDS_RESULT_OUT_OF_RANGE;
}

static void ecu_reset_execute(void *context, uint8_t subfunction) {
    (void)context;
    assert(subfunction == 0x01U);
    reset_execute_count++;
}

static void configure_bxcan(UdsIsoTpEndpointConfig *config, Stm32F767BxCanBinding *binding,
                            const UdsCallbacks *callbacks) {
    assert(stm32f767_bxcan_endpoint_configure(config, binding, callbacks, NULL));
}

static void test_classic_send(void) {
    FakeBus bus = {0};
    Stm32F767BxCanBinding binding = {
        .send_classic = fake_classic_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    configure_bxcan(&config, &binding, &callbacks);
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = {.can_id = 0x7E0U, .dlc = 3U, .data = {0x02U, 0x3EU, 0x00U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 1U && bus.frames[0].can_id == 0x7E8U && bus.frames[0].dlc == 3U &&
           !bus.frames[0].is_fd);
}

static void test_classic_send_failure_retries(void) {
    FakeBus bus = {.fail_send = true};
    Stm32F767BxCanBinding binding = {
        .send_classic = fake_classic_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    configure_bxcan(&config, &binding, &callbacks);
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = {.can_id = 0x7E0U, .dlc = 3U, .data = {0x02U, 0x3EU, 0x00U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_OK);
    assert(bus.frame_count == 0U);
    bus.fail_send = false;
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 1U);
}

static void test_classic_multiframe_sequence(void) {
    FakeBus bus = {0};
    Stm32F767BxCanBinding binding = {
        .send_classic = fake_classic_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    callbacks.read_did = read_did;
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    configure_bxcan(&config, &binding, &callbacks);
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = {.can_id = 0x7E0U, .dlc = 4U, .data = {0x03U, 0x22U, 0xF1U, 0x90U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 1U && (bus.frames[0].data[0] >> 4U) == 1U);
    IsoTpCanFrame flow_control = {.can_id = 0x7E0U, .dlc = 3U, .data = {0x30U, 0x00U, 0x00U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &flow_control, 0U) == ISOTP_OK);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 2U && (bus.frames[1].data[0] >> 4U) == 2U);
}

static void test_endpoint_tx_error_recovery(void) {
    FakeBus bus = {0};
    Stm32F767BxCanBinding binding = {
        .send_classic = fake_classic_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    callbacks.read_did = read_did;
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    configure_bxcan(&config, &binding, &callbacks);
    config.tx_complete = fake_tx_complete;
    config.tx_error = fake_tx_error;
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    assert(endpoint.config.tx_error == fake_tx_error);

    IsoTpCanFrame request = {.can_id = 0x7E0U, .dlc = 3U, .data = {0x02U, 0x3EU, 0x00U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(endpoint.tx_in_flight);
    bus.tx_error = true;
    assert(uds_isotp_endpoint_process(&endpoint, 1U) == ISOTP_ERR_STATE);
    assert(!endpoint.tx_in_flight);

    bus.frame_count = 0U;
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 2U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 2U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 1U);

    bus.tx_error = true;
    assert(uds_isotp_endpoint_process(&endpoint, 3U) == ISOTP_ERR_STATE);
    assert(!endpoint.tx_in_flight);
    bus.frame_count = 0U;
    IsoTpCanFrame multi_frame_request = {
        .can_id = 0x7E0U, .dlc = 4U, .data = {0x03U, 0x22U, 0xF1U, 0x90U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &multi_frame_request, 4U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 4U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 1U && (bus.frames[0].data[0] >> 4U) == 1U);
    bus.tx_error = true;
    assert(uds_isotp_endpoint_process(&endpoint, 5U) == ISOTP_ERR_STATE);
    assert(!endpoint.tx_in_flight);
    bus.frame_count = 0U;
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 6U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 6U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 1U);
}

static void test_repeated_classic_requests(void) {
    FakeBus bus = {0};
    Stm32F767BxCanBinding binding = {
        .send_classic = fake_classic_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    configure_bxcan(&config, &binding, &callbacks);
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));

    for (uint16_t count = 0U; count < 1000U; ++count) {
        bus.frame_count = 0U;
        IsoTpCanFrame request = {.can_id = 0x7E0U, .dlc = 3U, .data = {0x02U, 0x3EU, 0x00U}};
        assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
        assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
        assert(bus.frame_count == 1U && bus.frames[0].can_id == 0x7E8U &&
               bus.frames[0].data[0] == 0x02U && bus.frames[0].data[1] == 0x7EU &&
               bus.frames[0].data[2] == 0x00U);
    }
}

static void test_ecu_reset_response_and_exactly_once_execution(void) {
    FakeBus bus = {0};
    Stm32F767BxCanBinding binding = {
        .send_classic = fake_classic_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    callbacks.ecu_reset = ecu_reset;
    callbacks.ecu_reset_execute = ecu_reset_execute;
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    reset_execute_count = 0U;
    configure_bxcan(&config, &binding, &callbacks);
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = {.can_id = 0x7E0U, .dlc = 3U, .data = {0x02U, 0x11U, 0x01U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 1U && bus.frames[0].dlc == 3U && bus.frames[0].data[0] == 0x02U &&
           bus.frames[0].data[1] == 0x51U && bus.frames[0].data[2] == 0x01U);
    assert(reset_execute_count == 1U);
    uds_isotp_endpoint_tx_complete(&endpoint);
    assert(reset_execute_count == 1U);
}

static void test_fd(void) {
    FakeBus bus = {0};
    Stm32FdCanBinding binding = {.send_fd = fake_fd_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    assert(stm32_fdcan_endpoint_configure(&config, &binding, &callbacks, NULL));
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = {
        .can_id = 0x7E0U, .dlc = 8U, .is_fd = true, .data = {0x02U, 0x3EU, 0x00U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.frame_count == 1U && bus.frames[0].can_id == 0x7E8U && bus.frames[0].dlc == 8U &&
           bus.frames[0].is_fd && bus.frames[0].bit_rate_switch);
}

int main(void) {
    test_classic_send();
    test_classic_send_failure_retries();
    test_classic_multiframe_sequence();
    test_endpoint_tx_error_recovery();
    test_repeated_classic_requests();
    test_ecu_reset_response_and_exactly_once_execution();
    test_fd();
    return 0;
}
