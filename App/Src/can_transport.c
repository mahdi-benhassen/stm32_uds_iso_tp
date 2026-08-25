#include "can_transport.h"

#include <stddef.h>

void uds_can_transport_init(UdsCanTransport *transport, CAN_HandleTypeDef *hcan,
                            uint32_t request_id, uint32_t response_id) {
    if (transport == NULL)
        return;
    transport->hcan = hcan;
    transport->request_id = request_id;
    transport->response_id = response_id;
    transport->tx_mailbox_mask = 0U;
}

bool uds_can_transport_send(void *context, const IsoTpCanFrame *frame) {
    UdsCanTransport *transport = (UdsCanTransport *)context;
    if ((transport == NULL) || (transport->hcan == NULL) || (frame == NULL) || frame->is_fd ||
        (frame->dlc > 8U))
        return false;

    CAN_TxHeaderTypeDef header = {0};
    header.StdId = frame->can_id;
    header.ExtId = 0U;
    header.RTR = CAN_RTR_DATA;
    header.IDE = CAN_ID_STD;
    header.DLC = frame->dlc;
    header.TransmitGlobalTime = DISABLE;
    uint32_t mailbox = 0U;
    if (HAL_CAN_AddTxMessage(transport->hcan, &header, (uint8_t *)frame->data, &mailbox) != HAL_OK)
        return false;
    transport->tx_mailbox_mask |= 1UL << mailbox;
    return true;
}

bool uds_can_transport_tx_complete(void *context) {
    UdsCanTransport *transport = (UdsCanTransport *)context;
    if ((transport == NULL) || (transport->hcan == NULL))
        return false;
    if (transport->tx_mailbox_mask == 0U)
        return true;
    if (HAL_CAN_IsTxMessagePending(transport->hcan, transport->tx_mailbox_mask) != 0U)
        return false;
    transport->tx_mailbox_mask = 0U;
    return true;
}

uint32_t uds_can_transport_clock(void *context) {
    (void)context;
    return HAL_GetTick();
}
