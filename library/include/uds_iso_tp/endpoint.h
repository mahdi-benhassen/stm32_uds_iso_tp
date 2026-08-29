#ifndef STM32_UDS_ISO_TP_ENDPOINT_H
#define STM32_UDS_ISO_TP_ENDPOINT_H

#include "uds_iso_tp/isotp.h"
#include "uds_iso_tp/uds.h"

#include <stdbool.h>
#include <stdint.h>

typedef bool (*UdsIsoTpSendFrameFn)(void *context, const IsoTpCanFrame *frame);
typedef bool (*UdsIsoTpTxCompleteFn)(void *context);
typedef bool (*UdsIsoTpTxErrorFn)(void *context);
typedef uint32_t (*UdsIsoTpClockFn)(void *context);
typedef enum {
    UDS_RESET_EVENT_REQUESTED = 0,
    UDS_RESET_EVENT_RESPONSE_READY,
    UDS_RESET_EVENT_TX_SUBMITTED,
    UDS_RESET_EVENT_TX_COMPLETE,
    UDS_RESET_EVENT_EXECUTED
} UdsResetEvent;
typedef void (*UdsIsoTpResetEventFn)(void *context, UdsResetEvent event);

typedef struct {
    UdsIsoTpSendFrameFn send_frame;
    /* Required when uds_callbacks.ecu_reset is configured; queue acceptance is not completion. */
    UdsIsoTpTxCompleteFn tx_complete;
    UdsIsoTpTxErrorFn tx_error;
    UdsIsoTpClockFn clock_ms;
    UdsIsoTpResetEventFn reset_event;
    void *context;
    void *reset_event_context;
    IsoTpConfig isotp_config;
    uint32_t request_id;
    uint32_t response_id;
    uint32_t functional_request_id;
    UdsCallbacks uds_callbacks;
    void *uds_context;
} UdsIsoTpEndpointConfig;

typedef struct {
    UdsIsoTpEndpointConfig config;
    IsoTpRx rx;
    IsoTpTx tx;
    UdsServer uds;
    uint8_t response[UDS_MAX_RESPONSE_LENGTH];
    IsoTpCanFrame pending_frame;
    bool tx_pending;
    IsoTpCanFrame pending_control_frame;
    bool control_pending;
    bool pending_reset_completion;
    bool pending_frame_final;
    bool tx_in_flight;
    bool in_flight_final;
    bool in_flight_reset_completion;
    bool tx_reset_completion;
    uint8_t queued_response[UDS_MAX_RESPONSE_LENGTH];
    uint16_t queued_response_length;
    bool queued_response_pending;
    bool queued_reset_completion;
} UdsIsoTpEndpoint;

bool uds_isotp_endpoint_init(UdsIsoTpEndpoint *endpoint, const UdsIsoTpEndpointConfig *config,
                             uint32_t now_ms);
IsoTpStatus uds_isotp_endpoint_receive(UdsIsoTpEndpoint *endpoint, const IsoTpCanFrame *frame,
                                       uint32_t now_ms);
IsoTpStatus uds_isotp_endpoint_process(UdsIsoTpEndpoint *endpoint, uint32_t now_ms);
IsoTpStatus uds_isotp_endpoint_tick(UdsIsoTpEndpoint *endpoint, uint32_t now_ms);
void uds_isotp_endpoint_tx_complete(UdsIsoTpEndpoint *endpoint);
UdsServer *uds_isotp_endpoint_server(UdsIsoTpEndpoint *endpoint);

#endif
