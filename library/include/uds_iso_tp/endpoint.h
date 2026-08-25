#ifndef STM32_UDS_ISO_TP_ENDPOINT_H
#define STM32_UDS_ISO_TP_ENDPOINT_H

#include "uds_iso_tp/isotp.h"
#include "uds_iso_tp/uds.h"

#include <stdbool.h>
#include <stdint.h>

typedef bool (*UdsIsoTpSendFrameFn)(void *context, const IsoTpCanFrame *frame);
typedef uint32_t (*UdsIsoTpClockFn)(void *context);

typedef struct {
    UdsIsoTpSendFrameFn send_frame;
    UdsIsoTpClockFn clock_ms;
    void *context;
    IsoTpConfig isotp_config;
    uint32_t request_id;
    uint32_t response_id;
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
} UdsIsoTpEndpoint;

bool uds_isotp_endpoint_init(UdsIsoTpEndpoint *endpoint, const UdsIsoTpEndpointConfig *config,
                             uint32_t now_ms);
IsoTpStatus uds_isotp_endpoint_receive(UdsIsoTpEndpoint *endpoint, const IsoTpCanFrame *frame,
                                       uint32_t now_ms);
IsoTpStatus uds_isotp_endpoint_process(UdsIsoTpEndpoint *endpoint, uint32_t now_ms);
IsoTpStatus uds_isotp_endpoint_tick(UdsIsoTpEndpoint *endpoint, uint32_t now_ms);
UdsServer *uds_isotp_endpoint_server(UdsIsoTpEndpoint *endpoint);

#endif
