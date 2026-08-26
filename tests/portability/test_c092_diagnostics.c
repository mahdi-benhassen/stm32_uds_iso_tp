#include "uds_diagnostics.h"

#include <assert.h>
#include <stdint.h>

static void mark_required_stages(UdsC092DiagnosticTrace *trace) {
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_HAL_INIT_DONE, 1U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_CLOCK_READY, 2U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_GPIO_READY, 3U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_INIT_DONE, 4U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_FILTER_READY, 5U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_NOTIFICATION_READY, 6U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_STARTED, 7U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_UDS_INIT_DONE, 8U);
}

int main(void) {
    UdsC092DiagnosticTrace trace;
    uds_c092_diagnostic_init(&trace, 100U);
    assert(trace.state == UDS_C092_DIAG_BOOTING);
    assert(trace.stage_mask == 0U);
    assert(!uds_c092_diagnostic_is_ready(&trace));
    assert(trace.event_timestamp_ms[UDS_C092_BOOT_RESET_ENTRY] == UINT32_MAX);
    assert(uds_c092_filter_accept(0x7E0U, 0x7E0U, 0x7DFU, false, false, false, false));
    assert(uds_c092_filter_accept(0x7DFU, 0x7E0U, 0x7DFU, false, false, false, false));
    assert(!uds_c092_filter_accept(0x123U, 0x7E0U, 0x7DFU, false, false, false, false));
    assert(!uds_c092_filter_accept(0x7E0U, 0x7E0U, 0x7DFU, true, false, false, false));
    assert(!uds_c092_filter_accept(0x7E0U, 0x7E0U, 0x7DFU, false, true, false, false));
    assert(!uds_c092_filter_accept(0x7E0U, 0x7E0U, 0x7DFU, false, false, true, false));
    assert(!uds_c092_filter_accept(0x7E0U, 0x7E0U, 0x7DFU, false, false, false, true));

    uds_c092_diagnostic_mark(&trace, UDS_C092_BOOT_DIAGNOSTIC_READY, 101U);
    assert(!uds_c092_diagnostic_is_ready(&trace));
    uds_c092_diagnostic_count_rx_drop(&trace);
    assert(trace.rx_dropped_while_booting == 1U);

    mark_required_stages(&trace);
    uds_c092_diagnostic_mark(&trace, UDS_C092_BOOT_DIAGNOSTIC_READY, 109U);
    assert(uds_c092_diagnostic_is_ready(&trace));
    assert(trace.state == UDS_C092_DIAG_READY);
    assert(trace.stage_mask != 0U);

    uds_c092_diagnostic_count_rx(&trace, 110U);
    uds_c092_diagnostic_count_isotp_rx(&trace);
    uds_c092_diagnostic_count_uds_request(&trace, 111U);
    uds_c092_diagnostic_count_uds_response(&trace);
    uds_c092_diagnostic_count_fdcan_tx(&trace, 112U);
    uds_c092_diagnostic_count_rx_reject(&trace);
    assert(trace.fdcan_rx_count == 1U);
    assert(trace.isotp_rx_count == 1U);
    assert(trace.uds_request_count == 1U);
    assert(trace.uds_response_count == 1U);
    assert(trace.fdcan_tx_count == 1U);
    assert(trace.rx_rejected_count == 1U);
#if UDS_C092_DIAGNOSTIC_BOOT_TRACE
    assert(trace.event_timestamp_ms[UDS_C092_BOOT_FIRST_RX_AFTER_RESET] == 110U);
    assert(trace.event_timestamp_ms[UDS_C092_BOOT_FIRST_UDS_REQUEST_AFTER_RESET] == 111U);
    assert(trace.event_timestamp_ms[UDS_C092_BOOT_FIRST_TX_AFTER_RESET] == 112U);
#endif

    uds_c092_diagnostic_fault(&trace, 120U);
    assert(trace.state == UDS_C092_DIAG_FAULT);
    assert(!uds_c092_diagnostic_is_ready(&trace));

    uds_c092_diagnostic_init(&trace, 200U);
    assert(trace.state == UDS_C092_DIAG_BOOTING);
    assert(trace.stage_mask == 0U);
    assert(trace.fdcan_rx_count == 0U);
    assert(trace.uds_request_count == 0U);
    assert(trace.event_timestamp_ms[UDS_C092_BOOT_RESET_ENTRY] == UINT32_MAX);
    return 0;
}
