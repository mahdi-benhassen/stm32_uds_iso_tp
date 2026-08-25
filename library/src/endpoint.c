#include "uds_iso_tp/endpoint.h"

#include <stddef.h>

bool uds_isotp_endpoint_init(UdsIsoTpEndpoint *endpoint, const UdsIsoTpEndpointConfig *config,
                             uint32_t now_ms) {
    if ((endpoint == NULL) || (config == NULL) || (config->send_frame == NULL) ||
        (config->clock_ms == NULL) || (config->request_id == config->response_id)) {
        return false;
    }
    endpoint->config = *config;
    isotp_rx_init(&endpoint->rx, &config->isotp_config, config->request_id, config->response_id);
    isotp_tx_init(&endpoint->tx, &config->isotp_config, config->request_id, config->response_id);
    uds_server_init(&endpoint->uds, &config->uds_callbacks, config->uds_context, now_ms);
    endpoint->tx_pending = false;
    return true;
}

static IsoTpStatus submit_pending(UdsIsoTpEndpoint *endpoint) {
    if (!endpoint->tx_pending)
        return ISOTP_OK;
    if (!endpoint->config.send_frame(endpoint->config.context, &endpoint->pending_frame))
        return ISOTP_OK;
    endpoint->tx_pending = false;
    return ISOTP_TX_FRAME_READY;
}

IsoTpStatus uds_isotp_endpoint_receive(UdsIsoTpEndpoint *endpoint, const IsoTpCanFrame *frame,
                                       uint32_t now_ms) {
    if ((endpoint == NULL) || (frame == NULL) || (frame->dlc == 0U))
        return ISOTP_ERR_ARGUMENT;
    if ((frame->can_id == endpoint->config.request_id) && ((frame->data[0] >> 4U) == 3U)) {
        return isotp_tx_feed_flow_control(&endpoint->tx, frame, now_ms);
    }
    IsoTpRxEvent event;
    IsoTpStatus status = isotp_rx_feed(&endpoint->rx, frame, now_ms, &event);
    if (event.has_flow_control) {
        endpoint->pending_frame = event.flow_control;
        endpoint->tx_pending = true;
    }
    if (status != ISOTP_COMPLETE)
        return status;
    if (event.length > UINT16_MAX)
        return ISOTP_ERR_OVERFLOW;
    uint16_t response_length = 0U;
    UdsCallbackResult result =
        uds_server_handle(&endpoint->uds, event.payload, (uint16_t)event.length, endpoint->response,
                          &response_length, (uint16_t)sizeof(endpoint->response), now_ms);
    if ((result == UDS_RESULT_NO_RESPONSE) || (response_length == 0U))
        return status;
    if (result != UDS_RESULT_OK)
        return ISOTP_ERR_STATE;
    status = isotp_tx_start(&endpoint->tx, endpoint->response, response_length, now_ms,
                            &endpoint->pending_frame);
    if (status == ISOTP_TX_FRAME_READY)
        endpoint->tx_pending = true;
    return status;
}

IsoTpStatus uds_isotp_endpoint_process(UdsIsoTpEndpoint *endpoint, uint32_t now_ms) {
    if (endpoint == NULL)
        return ISOTP_ERR_ARGUMENT;
    IsoTpStatus status = submit_pending(endpoint);
    if (endpoint->tx_pending || (status == ISOTP_TX_FRAME_READY))
        return status;
    if (!endpoint->tx.active)
        return ISOTP_OK;
    status = isotp_tx_next(&endpoint->tx, now_ms, &endpoint->pending_frame);
    if (status == ISOTP_TX_FRAME_READY)
        endpoint->tx_pending = true;
    return status;
}

IsoTpStatus uds_isotp_endpoint_tick(UdsIsoTpEndpoint *endpoint, uint32_t now_ms) {
    if (endpoint == NULL)
        return ISOTP_ERR_ARGUMENT;
    IsoTpStatus rx_status = isotp_rx_tick(&endpoint->rx, now_ms);
    IsoTpStatus tx_status = isotp_tx_tick(&endpoint->tx, now_ms);
    (void)uds_server_tick(&endpoint->uds, now_ms);
    return (rx_status != ISOTP_OK) ? rx_status : tx_status;
}

UdsServer *uds_isotp_endpoint_server(UdsIsoTpEndpoint *endpoint) {
    return (endpoint != NULL) ? &endpoint->uds : NULL;
}
