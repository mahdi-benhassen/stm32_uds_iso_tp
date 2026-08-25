#ifndef STM32_UDS_ISO_TP_C092_UDS_APP_FDCAN_H
#define STM32_UDS_ISO_TP_C092_UDS_APP_FDCAN_H

#include "can_transport_fdcan.h"
#include "uds_iso_tp/endpoint.h"
#include "uds_iso_tp/uds.h"

#include <stdbool.h>
#include <stdint.h>

void uds_c092_app_init(UdsC092FdcanTransport *transport, uint32_t now_ms,
                       const UdsCallbacks *application_callbacks, void *uds_context,
                       UdsIsoTpResetEventFn reset_event, void *reset_event_context);
void uds_c092_app_rx_from_isr(uint32_t can_id, const uint8_t *data, uint8_t dlc, bool is_fd,
                              bool bit_rate_switch);
void uds_c092_app_process(uint32_t now_ms);

#endif
