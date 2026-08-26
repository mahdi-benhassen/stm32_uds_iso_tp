#ifndef STM32_UDS_ISO_TP_C092_CAN_TRANSPORT_FDCAN_H
#define STM32_UDS_ISO_TP_C092_CAN_TRANSPORT_FDCAN_H

#include "fdcan.h"
#include "uds_iso_tp/isotp.h"
#include "uds_diagnostics.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    FDCAN_HandleTypeDef *hfdcan;
    uint32_t request_id;
    uint32_t response_id;
    volatile bool tx_pending;
    volatile bool tx_complete;
    volatile bool tx_error;
    uint32_t tx_marker;
    UdsC092DiagnosticTrace *diagnostics;
} UdsC092FdcanTransport;

void uds_c092_fdcan_transport_init(UdsC092FdcanTransport *transport, FDCAN_HandleTypeDef *hfdcan,
                                   uint32_t request_id, uint32_t response_id);
void uds_c092_fdcan_attach_diagnostics(UdsC092FdcanTransport *transport,
                                       UdsC092DiagnosticTrace *diagnostics);
bool uds_c092_fdcan_send(void *context, const IsoTpCanFrame *frame);
uint8_t uds_c092_fdcan_data_length_bytes(uint32_t data_length_code);
bool uds_c092_fdcan_tx_complete(void *context);
bool uds_c092_fdcan_tx_error(void *context);
void uds_c092_fdcan_on_tx_event(UdsC092FdcanTransport *transport, uint32_t interrupt_flags);
/* Mainline fallback: drains stored TX events even when TX-event IRQ wiring is absent. */
void uds_c092_fdcan_poll_tx_events(UdsC092FdcanTransport *transport);
uint32_t uds_c092_fdcan_clock(void *context);

#endif
