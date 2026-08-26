#include "can_transport_fdcan.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static FDCAN_TxEventFifoTypeDef event_queue[8];
static uint8_t event_head;
static uint8_t event_tail;
static FDCAN_TxHeaderTypeDef last_header;
static HAL_StatusTypeDef enqueue_status = HAL_OK;

uint32_t HAL_GetTick(void) {
    return 0U;
}

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan,
                                                const FDCAN_TxHeaderTypeDef *header,
                                                const uint8_t *data) {
    (void)hfdcan;
    (void)data;
    if (header != NULL)
        last_header = *header;
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

static void push_event(uint32_t marker, uint32_t event_type) {
    assert((uint8_t)((event_tail + 1U) % 8U) != event_head);
    event_queue[event_tail].MessageMarker = marker;
    event_queue[event_tail].EventType = event_type;
    event_tail = (uint8_t)((event_tail + 1U) % 8U);
}

static void clear_events(void) {
    event_head = 0U;
    event_tail = 0U;
}

int main(void) {
    FDCAN_HandleTypeDef hfdcan = {0};
    UdsC092FdcanTransport transport = {0};
    IsoTpCanFrame frame = {.can_id = 0x7E8U, .dlc = 3U, .data = {0x02U, 0x3EU, 0x00U}};

    uds_c092_fdcan_transport_init(&transport, &hfdcan, 0x7E0U, 0x7E8U);
    assert(!uds_c092_fdcan_tx_complete(&transport));
    assert(uds_c092_fdcan_send(&transport, &frame));
    assert(last_header.MessageMarker == transport.tx_marker);
    uint32_t first_marker = transport.tx_marker;
    uds_c092_fdcan_poll_tx_events(&transport);
    assert(!uds_c092_fdcan_tx_complete(&transport));

    push_event(first_marker, FDCAN_TX_EVENT);
    uds_c092_fdcan_on_tx_event(&transport, FDCAN_IT_TX_EVT_FIFO_NEW_DATA);
    assert(uds_c092_fdcan_tx_complete(&transport));
    clear_events();

    assert(uds_c092_fdcan_send(&transport, &frame));
    uint32_t second_marker = transport.tx_marker;
    assert(second_marker != first_marker);
    push_event(first_marker, FDCAN_TX_EVENT);
    uds_c092_fdcan_poll_tx_events(&transport);
    assert(transport.tx_pending);
    assert(!uds_c092_fdcan_tx_complete(&transport));
    push_event(second_marker, FDCAN_TX_EVENT);
    uds_c092_fdcan_poll_tx_events(&transport);
    assert(uds_c092_fdcan_tx_complete(&transport));
    clear_events();

    assert(uds_c092_fdcan_send(&transport, &frame));
    uint32_t third_marker = transport.tx_marker;
    push_event(third_marker, 1U);
    uds_c092_fdcan_poll_tx_events(&transport);
    assert(!uds_c092_fdcan_tx_complete(&transport));
    assert(transport.tx_error);

    enqueue_status = HAL_ERROR;
    assert(!uds_c092_fdcan_send(&transport, &frame));
    assert(transport.tx_error);
    assert(!uds_c092_fdcan_tx_complete(&transport));
    enqueue_status = HAL_OK;
    assert(uds_c092_fdcan_send(&transport, &frame));
    assert(!transport.tx_error);
    uint32_t recovered_marker = transport.tx_marker;
    push_event(recovered_marker, FDCAN_TX_EVENT);
    uds_c092_fdcan_on_tx_event(&transport, FDCAN_IT_TX_EVT_FIFO_NEW_DATA);
    assert(uds_c092_fdcan_tx_complete(&transport));

    assert(uds_c092_fdcan_send(&transport, &frame));
    uds_c092_fdcan_on_tx_event(&transport, FDCAN_IT_TX_EVT_FIFO_FULL);
    assert(!transport.tx_pending && transport.tx_error);
    assert(!uds_c092_fdcan_tx_complete(&transport));
    return 0;
}
