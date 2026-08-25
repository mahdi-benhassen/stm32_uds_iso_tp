#ifndef STM32_UDS_ISO_TP_UDS_APP_H
#define STM32_UDS_ISO_TP_UDS_APP_H

#include "can_transport.h"

#include <stdint.h>

void uds_app_init(UdsCanTransport *transport, uint32_t now_ms);
void uds_app_process(uint32_t now_ms);
void uds_app_rx_from_isr(uint32_t can_id, const uint8_t *data, uint8_t dlc);

#endif
