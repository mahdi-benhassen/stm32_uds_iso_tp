#include "uds_app_fdcan.h"
#include "uds_platform_fdcan.h"

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

static FDCAN_TxEventFifoTypeDef event_queue[8];

UdsCallbackResult uds_c092_platform_reset_prepare(void *context, uint8_t subfunction) {
    (void)context;
    (void)subfunction;
    return UDS_RESULT_OK;
}

void uds_c092_platform_reset_execute(void *context, uint8_t subfunction) {
    (void)context;
    (void)subfunction;
}

uint32_t uds_c092_platform_now_ms(void) {
    return HAL_GetTick();
}
static uint8_t event_head;
static uint8_t event_tail;
static HAL_StatusTypeDef enqueue_status = HAL_OK;

uint32_t HAL_GetTick(void) {
    return 100U;
}

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan,
                                                const FDCAN_TxHeaderTypeDef *header,
                                                const uint8_t *data) {
    (void)hfdcan;
    (void)header;
    (void)data;
    return enqueue_status;
}

HAL_StatusTypeDef HAL_FDCAN_GetTxEvent(FDCAN_HandleTypeDef *hfdcan,
                                       FDCAN_TxEventFifoTypeDef *event) {
    (void)hfdcan;
    if ((event == NULL) || (event_head == event_tail))
        return HAL_ERROR;
    *event = event_queue[event_head];
    event_head = (uint8_t)((event_head + 1U) % 8U);
    return HAL_OK;
}

static void mark_started_and_initialized(UdsC092DiagnosticTrace *trace) {
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_HAL_INIT_DONE, 1U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_CLOCK_READY, 2U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_GPIO_READY, 3U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_INIT_START, 4U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_INIT_DONE, 5U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_FILTER_DONE, 6U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_NOTIFICATION_DONE, 7U);
    uds_c092_diagnostic_mark(trace, UDS_C092_BOOT_FDCAN_START_DONE, 8U);
}

int main(void) {
    FDCAN_HandleTypeDef hfdcan = {0};
    UdsC092FdcanTransport transport = {0};
    UdsC092DiagnosticTrace trace;
    const uint8_t tester_present[] = {0x02U, 0x3EU, 0x00U};

    uds_c092_diagnostic_init(&trace, 0U);
    mark_started_and_initialized(&trace);
    uds_c092_fdcan_transport_init(&transport, &hfdcan, 0x7E0U, 0x7E8U);
    uds_c092_app_attach_diagnostics(&trace);
    uds_c092_app_init_default(&transport, 9U);
    assert(!uds_c092_app_is_diagnostic_ready());

    /* FDCAN is started, but the application has not marked READY yet. */
    uds_c092_app_rx_from_isr_ex(0x7E0U, tester_present, 3U, false, false, false, false);
    assert(trace.fdcan_rx_count == 1U);
    assert(trace.rx_accepted_count == 1U);
    assert(trace.rx_dropped_while_booting == 0U);
    assert(trace.rx_mailbox_full_count == 0U);

    /* The single bounded mailbox rejects only the next simultaneous frame. */
    uds_c092_app_rx_from_isr_ex(0x7E0U, tester_present, 3U, false, false, false, false);
    assert(trace.rx_mailbox_full_count == 1U);

    uds_c092_app_process(10U);
    assert(trace.isotp_rx_count == 1U);
    assert(trace.uds_request_count == 1U);
    assert(trace.uds_response_generated_count == 1U);
    assert(trace.uds_response_count == 1U);
    assert(enqueue_status == HAL_OK);
    return 0;
}
