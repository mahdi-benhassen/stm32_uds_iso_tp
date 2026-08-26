#ifndef STM32_UDS_ISO_TP_C092_UDS_DIAGNOSTICS_H
#define STM32_UDS_ISO_TP_C092_UDS_DIAGNOSTICS_H

#include <stdbool.h>
#include <stdint.h>

#ifndef UDS_C092_DIAGNOSTIC_BOOT_TRACE
#define UDS_C092_DIAGNOSTIC_BOOT_TRACE 0U
#endif

typedef enum {
    UDS_C092_DIAG_BOOTING = 0U,
    UDS_C092_DIAG_READY,
    UDS_C092_DIAG_FAULT
} UdsC092DiagnosticState;

typedef enum {
    UDS_C092_BOOT_RESET_ENTRY = 0U,
    UDS_C092_BOOT_HAL_INIT_DONE,
    UDS_C092_BOOT_CLOCK_READY,
    UDS_C092_BOOT_GPIO_READY,
    UDS_C092_BOOT_FDCAN_INIT_DONE,
    UDS_C092_BOOT_FDCAN_FILTER_READY,
    UDS_C092_BOOT_FDCAN_NOTIFICATION_READY,
    UDS_C092_BOOT_FDCAN_STARTED,
    UDS_C092_BOOT_UDS_INIT_DONE,
    UDS_C092_BOOT_DIAGNOSTIC_READY,
    UDS_C092_BOOT_FIRST_RX_AFTER_RESET,
    UDS_C092_BOOT_FIRST_UDS_REQUEST_AFTER_RESET,
    UDS_C092_BOOT_FIRST_TX_AFTER_RESET,
    UDS_C092_BOOT_EVENT_COUNT
} UdsC092DiagnosticEvent;

typedef struct {
    volatile UdsC092DiagnosticState state;
    volatile uint32_t stage_mask;
    volatile uint32_t event_timestamp_ms[UDS_C092_BOOT_EVENT_COUNT];
    volatile uint32_t fdcan_rx_count;
    volatile uint32_t isotp_rx_count;
    volatile uint32_t uds_request_count;
    volatile uint32_t uds_response_count;
    volatile uint32_t fdcan_tx_count;
    volatile uint32_t rx_dropped_while_booting;
    volatile uint32_t rx_rejected_count;
} UdsC092DiagnosticTrace;

void uds_c092_diagnostic_init(UdsC092DiagnosticTrace *trace, uint32_t now_ms);
void uds_c092_diagnostic_mark(UdsC092DiagnosticTrace *trace, UdsC092DiagnosticEvent event,
                              uint32_t now_ms);
void uds_c092_diagnostic_fault(UdsC092DiagnosticTrace *trace, uint32_t now_ms);
bool uds_c092_diagnostic_is_ready(const UdsC092DiagnosticTrace *trace);
bool uds_c092_filter_accept(uint32_t can_id, uint32_t request_id, uint32_t functional_request_id,
                            bool is_fd, bool bit_rate_switch, bool is_extended_id,
                            bool is_remote_frame);
void uds_c092_diagnostic_count_rx(UdsC092DiagnosticTrace *trace, uint32_t now_ms);
void uds_c092_diagnostic_count_isotp_rx(UdsC092DiagnosticTrace *trace);
void uds_c092_diagnostic_count_uds_request(UdsC092DiagnosticTrace *trace, uint32_t now_ms);
void uds_c092_diagnostic_count_uds_response(UdsC092DiagnosticTrace *trace);
void uds_c092_diagnostic_count_fdcan_tx(UdsC092DiagnosticTrace *trace, uint32_t now_ms);
void uds_c092_diagnostic_count_rx_drop(UdsC092DiagnosticTrace *trace);
void uds_c092_diagnostic_count_rx_reject(UdsC092DiagnosticTrace *trace);

#endif
