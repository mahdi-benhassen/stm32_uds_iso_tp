#include "uds_app_fdcan.h"

#include "uds_app_config.h"
#include "uds_platform_fdcan.h"

#include <stddef.h>
#include <string.h>

static UdsC092FdcanTransport *s_transport;
static UdsIsoTpEndpoint s_endpoint;
static IsoTpCanFrame s_rx_frame;
static volatile bool s_rx_pending;
static bool s_initialized;

void uds_c092_app_init(UdsC092FdcanTransport *transport, uint32_t now_ms,
                       const UdsCallbacks *application_callbacks, void *uds_context,
                       UdsIsoTpResetEventFn reset_event, void *reset_event_context) {
    if (transport == NULL)
        return;

    UdsIsoTpEndpointConfig config = {0};
    isotp_config_classic_can(&config.isotp_config);
#if UDS_C092_CLASSIC_PADDING_ENABLED
    isotp_config_set_padding(&config.isotp_config, true, UDS_C092_CLASSIC_PADDING_VALUE);
#endif
    config.send_frame = uds_c092_fdcan_send;
    config.tx_complete = uds_c092_fdcan_tx_complete;
    config.clock_ms = uds_c092_fdcan_clock;
    config.reset_event = reset_event;
    config.context = transport;
    config.reset_event_context = reset_event_context;
    config.request_id = UDS_C092_REQUEST_ID;
    config.response_id = UDS_C092_RESPONSE_ID;
    config.functional_request_id = UDS_C092_FUNCTIONAL_REQUEST_ID;
    if (application_callbacks != NULL)
        config.uds_callbacks = *application_callbacks;
    config.uds_callbacks.ecu_reset = uds_c092_platform_reset_prepare;
    config.uds_callbacks.ecu_reset_execute = uds_c092_platform_reset_execute;
    config.uds_context = uds_context;

    s_transport = transport;
    s_rx_pending = false;
    s_initialized = uds_isotp_endpoint_init(&s_endpoint, &config, now_ms);
}

void uds_c092_app_init_default(UdsC092FdcanTransport *transport, uint32_t now_ms) {
    uds_c092_app_init(transport, now_ms, NULL, NULL, NULL, NULL);
}

void uds_c092_app_rx_from_isr(uint32_t can_id, const uint8_t *data, uint8_t dlc, bool is_fd,
                              bool bit_rate_switch) {
    if (!s_initialized || (data == NULL) || (dlc == 0U) ||
        (dlc > (is_fd ? ISOTP_MAX_FRAME_DATA : 8U)) ||
        ((can_id != UDS_C092_REQUEST_ID) && (can_id != UDS_C092_FUNCTIONAL_REQUEST_ID)))
        return;
    if (s_rx_pending)
        return;
    s_rx_frame.can_id = can_id;
    s_rx_frame.dlc = dlc;
    s_rx_frame.is_fd = is_fd;
    s_rx_frame.bit_rate_switch = bit_rate_switch;
    (void)memcpy(s_rx_frame.data, data, dlc);
    s_rx_pending = true;
}

void uds_c092_app_process(uint32_t now_ms) {
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

    uds_c092_fdcan_poll_tx_events(s_transport);
    if (uds_c092_fdcan_tx_complete(s_transport))
        uds_isotp_endpoint_tx_complete(&s_endpoint);
    if (has_frame)
        (void)uds_isotp_endpoint_receive(&s_endpoint, &frame, now_ms);
    (void)uds_isotp_endpoint_process(&s_endpoint, now_ms);
    (void)uds_isotp_endpoint_tick(&s_endpoint, now_ms);
}
