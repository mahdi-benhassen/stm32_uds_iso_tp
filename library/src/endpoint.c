#include "uds_iso_tp/endpoint.h"

#include <stddef.h>
#include <string.h>

static bool frame_is_functional(const UdsIsoTpEndpoint *endpoint, const IsoTpCanFrame *frame) {
    return (endpoint->config.functional_request_id != 0U) &&
           (frame->can_id == endpoint->config.functional_request_id);
}

static IsoTpStatus queue_control_frame(UdsIsoTpEndpoint *endpoint, const IsoTpCanFrame *frame) {
    if (endpoint->control_pending)
        return ISOTP_ERR_STATE;
    endpoint->pending_control_frame = *frame;
    endpoint->control_pending = true;
    return ISOTP_NEED_FLOW_CONTROL;
}

static bool response_reset_pending(const UdsServer *server) {
    return uds_server_reset_pending(server) && (server->pending_reset_subfunction != 0U);
}

static IsoTpStatus start_response(UdsIsoTpEndpoint *endpoint, const uint8_t *response,
                                  uint16_t response_length, bool reset_completion,
                                  uint32_t now_ms) {
    if ((endpoint->tx_pending || endpoint->tx_in_flight) ||
        (isotp_tx_state(&endpoint->tx) != ISOTP_TX_STATE_IDLE)) {
        if (endpoint->queued_response_pending)
            return ISOTP_ERR_STATE;
        (void)memcpy(endpoint->queued_response, response, response_length);
        endpoint->queued_response_length = response_length;
        endpoint->queued_response_pending = true;
        endpoint->queued_reset_completion = reset_completion;
        return ISOTP_TX_FRAME_READY;
    }
    IsoTpStatus status =
        isotp_tx_start(&endpoint->tx, response, response_length, now_ms, &endpoint->pending_frame);
    if (status != ISOTP_TX_FRAME_READY)
        return status;
    endpoint->tx_pending = true;
    endpoint->pending_frame_final = (isotp_tx_state(&endpoint->tx) == ISOTP_TX_STATE_IDLE);
    endpoint->pending_reset_completion = reset_completion;
    endpoint->tx_reset_completion = reset_completion;
    return status;
}

static IsoTpStatus submit_control(UdsIsoTpEndpoint *endpoint) {
    if (!endpoint->control_pending)
        return ISOTP_OK;
    if (!endpoint->config.send_frame(endpoint->config.context, &endpoint->pending_control_frame))
        return ISOTP_OK;
    endpoint->control_pending = false;
    return ISOTP_TX_FRAME_READY;
}

static void complete_in_flight(UdsIsoTpEndpoint *endpoint) {
    if ((endpoint == NULL) || !endpoint->tx_in_flight)
        return;
    endpoint->tx_in_flight = false;
    if (endpoint->in_flight_final && endpoint->in_flight_reset_completion) {
        (void)uds_server_complete_reset(&endpoint->uds);
    }
    endpoint->in_flight_final = false;
    endpoint->in_flight_reset_completion = false;
}

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
    endpoint->control_pending = false;
    endpoint->pending_reset_completion = false;
    endpoint->pending_frame_final = false;
    endpoint->tx_in_flight = false;
    endpoint->in_flight_final = false;
    endpoint->in_flight_reset_completion = false;
    endpoint->tx_reset_completion = false;
    endpoint->queued_response_length = 0U;
    endpoint->queued_response_pending = false;
    endpoint->queued_reset_completion = false;
    return true;
}

IsoTpStatus uds_isotp_endpoint_receive(UdsIsoTpEndpoint *endpoint, const IsoTpCanFrame *frame,
                                       uint32_t now_ms) {
    if ((endpoint == NULL) || (frame == NULL) || (frame->dlc == 0U))
        return ISOTP_ERR_ARGUMENT;

    if ((frame->can_id == endpoint->config.request_id) && ((frame->data[0] >> 4U) == 3U)) {
        if ((isotp_tx_state(&endpoint->tx) == ISOTP_TX_STATE_IDLE) &&
            endpoint->config.isotp_config.full_duplex && endpoint->rx.active)
            return ISOTP_OK;
        return isotp_tx_feed_flow_control(&endpoint->tx, frame, now_ms);
    }

    IsoTpCanFrame network_frame = *frame;
    UdsAddressMode address_mode = UDS_ADDRESS_PHYSICAL;
    if (frame_is_functional(endpoint, frame)) {
        network_frame.can_id = endpoint->config.request_id;
        address_mode = UDS_ADDRESS_FUNCTIONAL;
    }

    IsoTpRxEvent event;
    IsoTpStatus status = isotp_rx_feed(&endpoint->rx, &network_frame, now_ms, &event);
    if (event.has_flow_control) {
        IsoTpStatus queue_status = queue_control_frame(endpoint, &event.flow_control);
        if (queue_status != ISOTP_NEED_FLOW_CONTROL)
            return queue_status;
    }
    if (status != ISOTP_COMPLETE)
        return status;
    if (event.length > UINT16_MAX)
        return ISOTP_ERR_OVERFLOW;
    uint16_t response_length = 0U;
    UdsCallbackResult result = uds_server_handle_addressed(
        &endpoint->uds, event.payload, (uint16_t)event.length, endpoint->response, &response_length,
        (uint16_t)sizeof(endpoint->response), address_mode, now_ms);
    bool reset_completion = response_reset_pending(&endpoint->uds);
    if (result == UDS_RESULT_NO_RESPONSE) {
        if (reset_completion)
            (void)uds_server_complete_reset(&endpoint->uds);
        return status;
    }
    if ((result != UDS_RESULT_OK) || (response_length == 0U))
        return ISOTP_ERR_STATE;
    return start_response(endpoint, endpoint->response, response_length, reset_completion, now_ms);
}

IsoTpStatus uds_isotp_endpoint_process(UdsIsoTpEndpoint *endpoint, uint32_t now_ms) {
    if (endpoint == NULL)
        return ISOTP_ERR_ARGUMENT;
    if (endpoint->tx_in_flight)
        return ISOTP_OK;

    IsoTpStatus status = submit_control(endpoint);
    if (endpoint->control_pending || (status == ISOTP_TX_FRAME_READY))
        return status;

    if (endpoint->tx_pending) {
        if (!endpoint->config.send_frame(endpoint->config.context, &endpoint->pending_frame))
            return ISOTP_OK;
        endpoint->tx_pending = false;
        endpoint->tx_in_flight = true;
        endpoint->in_flight_final = endpoint->pending_frame_final;
        endpoint->in_flight_reset_completion = endpoint->pending_reset_completion;
        endpoint->pending_frame_final = false;
        endpoint->pending_reset_completion = false;
        if ((endpoint->config.tx_complete == NULL) ||
            endpoint->config.tx_complete(endpoint->config.context))
            complete_in_flight(endpoint);
        return ISOTP_TX_FRAME_READY;
    }

    if (isotp_tx_state(&endpoint->tx) != ISOTP_TX_STATE_IDLE) {
        status = isotp_tx_next(&endpoint->tx, now_ms, &endpoint->pending_frame);
        if (status == ISOTP_TX_FRAME_READY) {
            endpoint->tx_pending = true;
            endpoint->pending_frame_final = (endpoint->tx.offset >= endpoint->tx.payload_len);
            endpoint->pending_reset_completion = endpoint->tx_reset_completion;
            return status;
        }
        if (status != ISOTP_COMPLETE)
            return status;
    }

    if (endpoint->queued_response_pending) {
        uint16_t length = endpoint->queued_response_length;
        bool reset_completion = endpoint->queued_reset_completion;
        endpoint->queued_response_pending = false;
        endpoint->queued_response_length = 0U;
        endpoint->queued_reset_completion = false;
        return start_response(endpoint, endpoint->queued_response, length, reset_completion,
                              now_ms);
    }
    return ISOTP_OK;
}

IsoTpStatus uds_isotp_endpoint_tick(UdsIsoTpEndpoint *endpoint, uint32_t now_ms) {
    if (endpoint == NULL)
        return ISOTP_ERR_ARGUMENT;
    IsoTpStatus rx_status = isotp_rx_tick(&endpoint->rx, now_ms);
    IsoTpStatus tx_status = isotp_tx_tick(&endpoint->tx, now_ms);
    (void)uds_server_tick(&endpoint->uds, now_ms);
    return (rx_status != ISOTP_OK) ? rx_status : tx_status;
}

void uds_isotp_endpoint_tx_complete(UdsIsoTpEndpoint *endpoint) {
    complete_in_flight(endpoint);
}

UdsServer *uds_isotp_endpoint_server(UdsIsoTpEndpoint *endpoint) {
    return (endpoint != NULL) ? &endpoint->uds : NULL;
}
