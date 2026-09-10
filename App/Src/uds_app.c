#include "uds_app.h"

#include "uds_platform.h"
#include "uds_app_config.h"
#include "uds_iso_tp/endpoint.h"
#include "uds_iso_tp/uds.h"

#include <stddef.h>
#include <string.h>

#define UDS_APP_REQUEST_ID 0x7E0U
#define UDS_APP_FUNCTIONAL_ID 0x7DFU
#define UDS_APP_RESPONSE_ID 0x7E8U

#define UDS_APP_RX_FIFO_CAPACITY 8U

static UdsCanTransport *s_transport;
static UdsIsoTpEndpoint s_endpoint;
static IsoTpCanFrame s_rx_fifo[UDS_APP_RX_FIFO_CAPACITY];
static volatile uint8_t s_rx_head;
static volatile uint8_t s_rx_tail;
static bool s_initialized;
static uint8_t s_app_did_f190_data[32] = "stm32f767-uds";
static uint16_t s_app_did_f190_len = 13U;

static UdsCallbackResult uds_app_read_did(void *context, uint16_t did, uint8_t *data,
                                          uint16_t *length, uint16_t capacity) {
    (void)context;
    if ((did != 0xF190U) || (capacity < s_app_did_f190_len)) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    (void)memcpy(data, s_app_did_f190_data, s_app_did_f190_len);
    *length = s_app_did_f190_len;
    return UDS_RESULT_OK;
}

static UdsCallbackResult uds_app_write_did(void *context, uint16_t did, const uint8_t *data,
                                           uint16_t length) {
    (void)context;
    if ((did != 0xF190U) || (length == 0U) || (length > sizeof(s_app_did_f190_data))) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    (void)memcpy(s_app_did_f190_data, data, length);
    s_app_did_f190_len = length;
    return UDS_RESULT_OK;
}

static UdsCallbackResult uds_app_ecu_reset_prepare(void *context, uint8_t subfunction) {
    (void)context;
    return (subfunction == 0x01U) ? UDS_RESULT_OK : UDS_RESULT_OUT_OF_RANGE;
}

static void uds_app_ecu_reset_execute(void *context, uint8_t subfunction) {
    (void)context;
    uds_platform_system_reset(subfunction);
}

void uds_app_init(UdsCanTransport *transport, uint32_t now_ms) {
    if (transport == NULL) {
        return;
    }

    UdsIsoTpEndpointConfig config = {0};
    isotp_config_classic_can(&config.isotp_config);
#if UDS_APP_CLASSIC_PADDING_ENABLED
    isotp_config_set_padding(&config.isotp_config, true, UDS_APP_CLASSIC_PADDING_VALUE);
#endif
    config.send_frame = uds_can_transport_send;
    config.tx_complete = uds_can_transport_tx_complete;
    config.clock_ms = uds_can_transport_clock;
    config.context = transport;
    config.request_id = UDS_APP_REQUEST_ID;
    config.response_id = UDS_APP_RESPONSE_ID;
    config.functional_request_id = UDS_APP_FUNCTIONAL_ID;
    config.uds_callbacks.read_did = uds_app_read_did;
    config.uds_callbacks.write_did = uds_app_write_did;
    config.uds_callbacks.ecu_reset = uds_app_ecu_reset_prepare;
    config.uds_callbacks.ecu_reset_execute = uds_app_ecu_reset_execute;
    config.uds_context = transport;

    s_transport = transport;
    s_rx_head = 0U;
    s_rx_tail = 0U;
    s_initialized = uds_isotp_endpoint_init(&s_endpoint, &config, now_ms);
}

void uds_app_rx_from_isr(uint32_t can_id, const uint8_t *data, uint8_t dlc) {
    if (!s_initialized || (data == NULL) || (dlc == 0U) || (dlc > 8U) ||
        ((can_id != UDS_APP_REQUEST_ID) && (can_id != UDS_APP_FUNCTIONAL_ID))) {
        return;
    }
    uint8_t next_head = (uint8_t)((s_rx_head + 1U) % UDS_APP_RX_FIFO_CAPACITY);
    if (next_head == s_rx_tail) {
        return;
    }
    s_rx_fifo[s_rx_head].can_id = can_id;
    s_rx_fifo[s_rx_head].dlc = dlc;
    s_rx_fifo[s_rx_head].is_fd = false;
    s_rx_fifo[s_rx_head].bit_rate_switch = false;
    (void)memcpy(s_rx_fifo[s_rx_head].data, data, dlc);
    s_rx_head = next_head;
}

void uds_app_process(uint32_t now_ms) {
    if (!s_initialized || (s_transport == NULL)) {
        return;
    }

    IsoTpCanFrame frame = {0};
    bool has_frame = false;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (s_rx_head != s_rx_tail) {
        frame = s_rx_fifo[s_rx_tail];
        s_rx_tail = (uint8_t)((s_rx_tail + 1U) % UDS_APP_RX_FIFO_CAPACITY);
        has_frame = true;
    }
    __set_PRIMASK(primask);

    if (uds_can_transport_tx_complete(s_transport)) {
        uds_isotp_endpoint_tx_complete(&s_endpoint);
    }
    if (has_frame) {
        (void)uds_isotp_endpoint_receive(&s_endpoint, &frame, now_ms);
    }
    (void)uds_isotp_endpoint_process(&s_endpoint, now_ms);
    (void)uds_isotp_endpoint_tick(&s_endpoint, now_ms);
}
