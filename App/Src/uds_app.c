#include "uds_app.h"

#include "uds_platform.h"
#include "uds_iso_tp/endpoint.h"
#include "uds_iso_tp/uds.h"

#include <stddef.h>
#include <string.h>

#define UDS_APP_REQUEST_ID 0x7E0U
#define UDS_APP_RESPONSE_ID 0x7E8U

static UdsCanTransport *s_transport;
static UdsIsoTpEndpoint s_endpoint;
static IsoTpCanFrame s_rx_frame;
static volatile bool s_rx_pending;
static bool s_initialized;

static UdsCallbackResult uds_app_read_did(void *context, uint16_t did, uint8_t *data,
                                          uint16_t *length, uint16_t capacity) {
    (void)context;
    static const uint8_t identity[] = "stm32f767-uds";
    if ((did != 0xF190U) || (capacity < (sizeof(identity) - 1U)))
        return UDS_RESULT_OUT_OF_RANGE;
    (void)memcpy(data, identity, sizeof(identity) - 1U);
    *length = (uint16_t)(sizeof(identity) - 1U);
    return UDS_RESULT_OK;
}

void uds_app_init(UdsCanTransport *transport, uint32_t now_ms) {
    if (transport == NULL)
        return;

    UdsIsoTpEndpointConfig config = {0};
    isotp_config_classic_can(&config.isotp_config);
    config.send_frame = uds_can_transport_send;
    config.clock_ms = uds_can_transport_clock;
    config.context = transport;
    config.request_id = UDS_APP_REQUEST_ID;
    config.response_id = UDS_APP_RESPONSE_ID;
    config.uds_callbacks.read_did = uds_app_read_did;
    config.uds_context = transport;

    s_transport = transport;
    s_rx_pending = false;
    s_initialized = uds_isotp_endpoint_init(&s_endpoint, &config, now_ms);
}

void uds_app_rx_from_isr(uint32_t can_id, const uint8_t *data, uint8_t dlc) {
    if (!s_initialized || (data == NULL) || (dlc == 0U) || (dlc > 8U) ||
        (can_id != UDS_APP_REQUEST_ID))
        return;
    if (s_rx_pending)
        return;
    s_rx_frame.can_id = can_id;
    s_rx_frame.dlc = dlc;
    s_rx_frame.is_fd = false;
    s_rx_frame.bit_rate_switch = false;
    (void)memcpy(s_rx_frame.data, data, dlc);
    s_rx_pending = true;
}

void uds_app_process(uint32_t now_ms) {
    if (!s_initialized || (s_transport == NULL))
        return;

    IsoTpCanFrame frame = {0};
    bool has_frame = false;
    __disable_irq();
    if (s_rx_pending) {
        frame = s_rx_frame;
        s_rx_pending = false;
        has_frame = true;
    }
    __enable_irq();

    if (has_frame)
        (void)uds_isotp_endpoint_receive(&s_endpoint, &frame, now_ms);
    (void)uds_isotp_endpoint_process(&s_endpoint, now_ms);
    (void)uds_isotp_endpoint_tick(&s_endpoint, now_ms);
}
