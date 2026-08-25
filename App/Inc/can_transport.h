#ifndef STM32_UDS_ISO_TP_CAN_TRANSPORT_H
#define STM32_UDS_ISO_TP_CAN_TRANSPORT_H

#include "main.h"
#include "uds_iso_tp/isotp.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    CAN_HandleTypeDef *hcan;
    uint32_t request_id;
    uint32_t response_id;
} UdsCanTransport;

void uds_can_transport_init(UdsCanTransport *transport, CAN_HandleTypeDef *hcan,
                            uint32_t request_id, uint32_t response_id);
bool uds_can_transport_send(void *context, const IsoTpCanFrame *frame);
uint32_t uds_can_transport_clock(void *context);

#endif
