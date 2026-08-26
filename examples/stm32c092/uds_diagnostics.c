#include "uds_diagnostics.h"

#include <stddef.h>

#define UDS_C092_DIAG_REQUIRED_MASK                                                                \
    (((1UL << (UDS_C092_BOOT_UDS_INIT_DONE + 1U)) - 1UL) & ~(1UL << UDS_C092_BOOT_RESET_ENTRY))

// cppcheck-suppress constParameterPointer
static void record_event(UdsC092DiagnosticTrace *trace, UdsC092DiagnosticEvent event,
                         uint32_t now_ms) {
    if ((trace == NULL) || (event >= UDS_C092_BOOT_EVENT_COUNT))
        return;
#if UDS_C092_DIAGNOSTIC_BOOT_TRACE
    if (trace->event_timestamp_ms[event] == UINT32_MAX)
        trace->event_timestamp_ms[event] = now_ms;
#else
    (void)now_ms;
#endif
}

void uds_c092_diagnostic_init(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace == NULL)
        return;
    trace->state = UDS_C092_DIAG_BOOTING;
    trace->stage_mask = 0U;
    trace->fdcan_rx_count = 0U;
    trace->rx_accepted_count = 0U;
    trace->isotp_rx_count = 0U;
    trace->uds_request_count = 0U;
    trace->uds_response_count = 0U;
    trace->uds_response_generated_count = 0U;
    trace->fdcan_tx_count = 0U;
    trace->tx_complete_count = 0U;
    trace->rx_dropped_while_booting = 0U;
    trace->rx_mailbox_full_count = 0U;
    trace->rx_rejected_count = 0U;
    for (size_t index = 0U; index < UDS_C092_BOOT_EVENT_COUNT; ++index)
        trace->event_timestamp_ms[index] = UINT32_MAX;
    record_event(trace, UDS_C092_BOOT_RESET_ENTRY, now_ms);
}

void uds_c092_diagnostic_mark(UdsC092DiagnosticTrace *trace, UdsC092DiagnosticEvent event,
                              uint32_t now_ms) {
    if ((trace == NULL) || (event >= UDS_C092_BOOT_EVENT_COUNT))
        return;
    record_event(trace, event, now_ms);
    if (event == UDS_C092_BOOT_DIAGNOSTIC_READY) {
        if ((trace->stage_mask & UDS_C092_DIAG_REQUIRED_MASK) == UDS_C092_DIAG_REQUIRED_MASK)
            trace->state = UDS_C092_DIAG_READY;
        return;
    }
    if ((event > UDS_C092_BOOT_RESET_ENTRY) && (event <= UDS_C092_BOOT_UDS_INIT_DONE))
        trace->stage_mask |= (uint32_t)(1UL << event);
}

void uds_c092_diagnostic_fault(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace == NULL)
        return;
    record_event(trace, UDS_C092_BOOT_RESET_ENTRY, now_ms);
    trace->state = UDS_C092_DIAG_FAULT;
}

bool uds_c092_diagnostic_is_ready(const UdsC092DiagnosticTrace *trace) {
    return (trace != NULL) && (trace->state == UDS_C092_DIAG_READY);
}

bool uds_c092_filter_accept(uint32_t can_id, uint32_t request_id, uint32_t functional_request_id,
                            bool is_fd, bool bit_rate_switch, bool is_extended_id,
                            bool is_remote_frame) {
    return !is_fd && !bit_rate_switch && !is_extended_id && !is_remote_frame &&
           ((can_id == request_id) ||
            ((functional_request_id != 0U) && (can_id == functional_request_id)));
}

void uds_c092_diagnostic_count_rx(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace == NULL)
        return;
    trace->fdcan_rx_count++;
    record_event(trace, UDS_C092_BOOT_FIRST_RX_AFTER_RESET, now_ms);
}

void uds_c092_diagnostic_count_rx_accepted(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace == NULL)
        return;
    trace->rx_accepted_count++;
    record_event(trace, UDS_C092_BOOT_RX_ACCEPTED, now_ms);
}

void uds_c092_diagnostic_count_isotp_rx(UdsC092DiagnosticTrace *trace) {
    uds_c092_diagnostic_count_isotp_rx_at(trace, 0U);
}

void uds_c092_diagnostic_count_isotp_rx_at(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace == NULL)
        return;
    trace->isotp_rx_count++;
    record_event(trace, UDS_C092_BOOT_ISOTP_RX, now_ms);
}

void uds_c092_diagnostic_count_uds_request(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace == NULL)
        return;
    trace->uds_request_count++;
    record_event(trace, UDS_C092_BOOT_UDS_REQUEST, now_ms);
}

void uds_c092_diagnostic_count_uds_response(UdsC092DiagnosticTrace *trace) {
    if (trace != NULL)
        trace->uds_response_count++;
}

void uds_c092_diagnostic_count_uds_response_generated(UdsC092DiagnosticTrace *trace,
                                                      uint32_t now_ms) {
    if (trace == NULL)
        return;
    trace->uds_response_generated_count++;
    trace->uds_response_count++;
    record_event(trace, UDS_C092_BOOT_UDS_RESPONSE_GENERATED, now_ms);
}

void uds_c092_diagnostic_count_fdcan_tx(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace == NULL)
        return;
    trace->fdcan_tx_count++;
    record_event(trace, UDS_C092_BOOT_TX_SUBMITTED, now_ms);
}

void uds_c092_diagnostic_count_tx_complete(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace == NULL)
        return;
    trace->tx_complete_count++;
    record_event(trace, UDS_C092_BOOT_TX_COMPLETE, now_ms);
}

void uds_c092_diagnostic_count_rx_drop(UdsC092DiagnosticTrace *trace) {
    uds_c092_diagnostic_count_rx_drop_at(trace, 0U);
}

void uds_c092_diagnostic_count_rx_drop_at(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace != NULL) {
        trace->rx_dropped_while_booting++;
        record_event(trace, UDS_C092_BOOT_RX_DROPPED_NOT_READY, now_ms);
    }
}

void uds_c092_diagnostic_count_rx_mailbox_full(UdsC092DiagnosticTrace *trace) {
    uds_c092_diagnostic_count_rx_mailbox_full_at(trace, 0U);
}

void uds_c092_diagnostic_count_rx_mailbox_full_at(UdsC092DiagnosticTrace *trace, uint32_t now_ms) {
    if (trace != NULL) {
        trace->rx_mailbox_full_count++;
        record_event(trace, UDS_C092_BOOT_RX_MAILBOX_FULL, now_ms);
    }
}

void uds_c092_diagnostic_count_rx_reject(UdsC092DiagnosticTrace *trace) {
    if (trace != NULL)
        trace->rx_rejected_count++;
}
