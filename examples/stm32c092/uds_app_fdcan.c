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
static UdsC092DiagnosticTrace *s_diagnostics;

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
    config.tx_error = uds_c092_fdcan_tx_error;
    config.clock_ms = uds_c092_fdcan_clock;
    config.reset_event = reset_event;
    config.context = transport;
    config.reset_event_context = reset_event_context;
    config.request_id = UDS_C092_REQUEST_ID;
    config.response_id = UDS_C092_RESPONSE_ID;
    config.functional_request_id = UDS_C092_FUNCTIONAL_REQUEST_ID;
    config.reset_guard_ms = UDS_C092_RESET_GUARD_MS;
    if (application_callbacks != NULL)
        config.uds_callbacks = *application_callbacks;
    config.uds_callbacks.ecu_reset = uds_c092_platform_reset_prepare;
    config.uds_callbacks.ecu_reset_execute = uds_c092_platform_reset_execute;
    config.uds_context = uds_context;

    s_transport = transport;
    s_rx_pending = false;
    s_initialized = uds_isotp_endpoint_init(&s_endpoint, &config, now_ms);
    if (s_diagnostics != NULL) {
        uds_c092_fdcan_attach_diagnostics(transport, s_diagnostics);
        if (s_initialized) {
            uds_c092_diagnostic_mark(s_diagnostics, UDS_C092_BOOT_ISOTP_INIT_DONE, now_ms);
            uds_c092_diagnostic_mark(s_diagnostics, UDS_C092_BOOT_UDS_INIT_DONE, now_ms);
        } else {
            uds_c092_diagnostic_fault(s_diagnostics, now_ms);
        }
    }
}

void uds_c092_app_attach_diagnostics(UdsC092DiagnosticTrace *trace) {
    s_diagnostics = trace;
    if (s_transport != NULL)
        uds_c092_fdcan_attach_diagnostics(s_transport, trace);
}

bool uds_c092_app_is_diagnostic_ready(void) {
    return (s_diagnostics == NULL) || uds_c092_diagnostic_is_ready(s_diagnostics);
}

void uds_c092_app_init_default(UdsC092FdcanTransport *transport, uint32_t now_ms) {
    uds_c092_app_init(transport, now_ms, NULL, NULL, NULL, NULL);
}

bool uds_c092_app_accept_rx(uint32_t can_id, bool is_fd, bool bit_rate_switch, bool is_extended_id,
                            bool is_remote_frame) {
    return uds_c092_filter_accept(can_id, UDS_C092_REQUEST_ID, UDS_C092_FUNCTIONAL_REQUEST_ID,
                                  is_fd, bit_rate_switch, is_extended_id, is_remote_frame);
}

void uds_c092_app_rx_from_isr_ex(uint32_t can_id, const uint8_t *data, uint8_t dlc, bool is_fd,
                                 bool bit_rate_switch, bool is_extended_id, bool is_remote_frame) {
    uint32_t now_ms = uds_c092_fdcan_clock(s_transport);
    if (!s_initialized) {
        uds_c092_diagnostic_count_rx_rejected_not_initialized(s_diagnostics, now_ms);
        return;
    }
    if ((data == NULL) || (dlc == 0U) || (dlc > (is_fd ? ISOTP_MAX_FRAME_DATA : 8U)) ||
        !uds_c092_app_accept_rx(can_id, is_fd, bit_rate_switch, is_extended_id, is_remote_frame)) {
        uds_c092_diagnostic_count_rx_reject(s_diagnostics);
        return;
    }
    if (s_rx_pending) {
        uds_c092_diagnostic_count_rx_mailbox_full_at(s_diagnostics, now_ms);
        return;
    }
    uds_c092_diagnostic_count_rx(s_diagnostics, now_ms);
    uds_c092_diagnostic_count_rx_accepted(s_diagnostics, now_ms);
    s_rx_frame.can_id = can_id;
    s_rx_frame.dlc = dlc;
    s_rx_frame.is_fd = is_fd;
    s_rx_frame.bit_rate_switch = bit_rate_switch;
    (void)memcpy(s_rx_frame.data, data, dlc);
    s_rx_pending = true;
}

void uds_c092_app_rx_from_isr(uint32_t can_id, const uint8_t *data, uint8_t dlc, bool is_fd,
                              bool bit_rate_switch) {
    uds_c092_app_rx_from_isr_ex(can_id, data, dlc, is_fd, bit_rate_switch, false, false);
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

    __disable_irq();
    uds_c092_fdcan_poll_tx_events(s_transport);
    bool tx_complete = uds_c092_fdcan_tx_complete(s_transport);
    __enable_irq();
    if (tx_complete) {
        uds_c092_diagnostic_count_tx_complete(s_diagnostics, now_ms);
        uds_isotp_endpoint_tx_complete(&s_endpoint);
    }
    if (has_frame) {
        IsoTpStatus receive_status = uds_isotp_endpoint_receive(&s_endpoint, &frame, now_ms);
        if (receive_status != ISOTP_ERR_ARGUMENT)
            uds_c092_diagnostic_count_isotp_rx_at(s_diagnostics, now_ms);
        if ((receive_status == ISOTP_COMPLETE) || (receive_status == ISOTP_TX_FRAME_READY)) {
            uds_c092_diagnostic_count_uds_request(s_diagnostics, now_ms);
            if ((receive_status == ISOTP_TX_FRAME_READY) &&
                (s_endpoint.tx_pending || s_endpoint.queued_response_pending))
                uds_c092_diagnostic_count_uds_response_generated(s_diagnostics, now_ms);
        }
    }
    (void)uds_isotp_endpoint_process(&s_endpoint, now_ms);
    (void)uds_isotp_endpoint_tick(&s_endpoint, now_ms);
}
