#include "can_transport_fdcan.h"

#include <stddef.h>

#define UDS_C092_FDCAN_TX_MARKER 0xD2U

uint8_t uds_c092_fdcan_data_length_bytes(uint32_t data_length_code) {
    switch (data_length_code) {
    case FDCAN_DLC_BYTES_0:
        return 0U;
    case FDCAN_DLC_BYTES_1:
        return 1U;
    case FDCAN_DLC_BYTES_2:
        return 2U;
    case FDCAN_DLC_BYTES_3:
        return 3U;
    case FDCAN_DLC_BYTES_4:
        return 4U;
    case FDCAN_DLC_BYTES_5:
        return 5U;
    case FDCAN_DLC_BYTES_6:
        return 6U;
    case FDCAN_DLC_BYTES_7:
        return 7U;
    case FDCAN_DLC_BYTES_8:
        return 8U;
    default:
        return 0U;
    }
}

static uint32_t fdcan_classic_dlc(uint8_t length) {
    switch (length) {
    case 0U:
        return FDCAN_DLC_BYTES_0;
    case 1U:
        return FDCAN_DLC_BYTES_1;
    case 2U:
        return FDCAN_DLC_BYTES_2;
    case 3U:
        return FDCAN_DLC_BYTES_3;
    case 4U:
        return FDCAN_DLC_BYTES_4;
    case 5U:
        return FDCAN_DLC_BYTES_5;
    case 6U:
        return FDCAN_DLC_BYTES_6;
    case 7U:
        return FDCAN_DLC_BYTES_7;
    default:
        return FDCAN_DLC_BYTES_8;
    }
}

void uds_c092_fdcan_transport_init(UdsC092FdcanTransport *transport, FDCAN_HandleTypeDef *hfdcan,
                                   uint32_t request_id, uint32_t response_id) {
    if (transport == NULL)
        return;
    transport->hfdcan = hfdcan;
    transport->request_id = request_id;
    transport->response_id = response_id;
    transport->tx_pending = false;
    transport->tx_complete = false;
    transport->tx_error = false;
    transport->tx_marker = UDS_C092_FDCAN_TX_MARKER;
    transport->diagnostics = NULL;
}

void uds_c092_fdcan_attach_diagnostics(UdsC092FdcanTransport *transport,
                                       UdsC092DiagnosticTrace *diagnostics) {
    if (transport != NULL)
        transport->diagnostics = diagnostics;
}

bool uds_c092_fdcan_send(void *context, const IsoTpCanFrame *frame) {
    UdsC092FdcanTransport *transport = (UdsC092FdcanTransport *)context;
    if ((transport == NULL) || (transport->hfdcan == NULL) || (frame == NULL) || frame->is_fd ||
        (frame->dlc > 8U) || transport->tx_pending)
        return false;

    FDCAN_TxHeaderTypeDef header = {0};
    header.Identifier = frame->can_id;
    header.IdType = FDCAN_STANDARD_ID;
    header.TxFrameType = FDCAN_DATA_FRAME;
    header.DataLength = fdcan_classic_dlc(frame->dlc);
    header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    header.BitRateSwitch = FDCAN_BRS_OFF;
    header.FDFormat = FDCAN_CLASSIC_CAN;
    header.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    transport->tx_marker = (transport->tx_marker + 1U) & 0xFFU;
    if (transport->tx_marker == 0U)
        transport->tx_marker = 1U;
    header.MessageMarker = transport->tx_marker;

    transport->tx_pending = true;
    transport->tx_complete = false;
    transport->tx_error = false;
    if (HAL_FDCAN_AddMessageToTxFifoQ(transport->hfdcan, &header, frame->data) != HAL_OK) {
        transport->tx_pending = false;
        transport->tx_error = true;
        return false;
    }
    uds_c092_diagnostic_count_fdcan_tx(transport->diagnostics, HAL_GetTick());
    return true;
}

bool uds_c092_fdcan_tx_complete(void *context) {
    UdsC092FdcanTransport *transport = (UdsC092FdcanTransport *)context;
    if ((transport == NULL) || (transport->hfdcan == NULL))
        return false;
    if (transport->tx_error)
        return false;
    if (transport->tx_pending)
        return false;
    if (transport->tx_complete) {
        transport->tx_complete = false;
        return true;
    }
    return false;
}

bool uds_c092_fdcan_tx_error(void *context) {
    UdsC092FdcanTransport *transport = (UdsC092FdcanTransport *)context;
    if ((transport == NULL) || (transport->hfdcan == NULL) || !transport->tx_error)
        return false;
    transport->tx_error = false;
    transport->tx_complete = false;
    transport->tx_pending = false;
    return true;
}

/*
 * The ISR and mainline fallback must not drain the HAL FIFO concurrently. The
 * application integration serializes them by polling with the TX-event IRQ
 * masked; a direct caller must provide the equivalent critical section.
 */
static void drain_tx_events(UdsC092FdcanTransport *transport, bool fifo_error) {
    FDCAN_TxEventFifoTypeDef event = {0};
    while (HAL_FDCAN_GetTxEvent(transport->hfdcan, &event) == HAL_OK) {
        if (event.MessageMarker != transport->tx_marker)
            continue;
        transport->tx_pending = false;
        if ((event.EventType == FDCAN_TX_EVENT) && !fifo_error && !transport->tx_error) {
            transport->tx_complete = true;
        } else {
            transport->tx_complete = false;
            transport->tx_error = true;
        }
    }
}

void uds_c092_fdcan_on_tx_event(UdsC092FdcanTransport *transport, uint32_t interrupt_flags) {
    if ((transport == NULL) || (transport->hfdcan == NULL))
        return;
    bool fifo_error =
        (interrupt_flags & (FDCAN_IT_TX_EVT_FIFO_ELT_LOST | FDCAN_IT_TX_EVT_FIFO_FULL)) != 0U;
    if (fifo_error) {
        transport->tx_pending = false;
        transport->tx_complete = false;
        transport->tx_error = true;
    }
    if ((interrupt_flags & FDCAN_IT_TX_EVT_FIFO_NEW_DATA) != 0U)
        drain_tx_events(transport, fifo_error);
}

void uds_c092_fdcan_poll_tx_events(UdsC092FdcanTransport *transport) {
    if ((transport == NULL) || (transport->hfdcan == NULL))
        return;
    drain_tx_events(transport, false);
}

uint32_t uds_c092_fdcan_clock(void *context) {
    (void)context;
    return HAL_GetTick();
}
