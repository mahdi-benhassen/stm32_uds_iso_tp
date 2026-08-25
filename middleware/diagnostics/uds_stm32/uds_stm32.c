/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "uds_stm32.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>

#if defined(USE_HAL_CAN_REGISTER_CALLBACKS) && (USE_HAL_CAN_REGISTER_CALLBACKS != 0U)
static UdsStm32Can *s_attached_adapter;

static void uds_stm32_can_rx_fifo1_callback(CAN_HandleTypeDef *hcan) {
    if ((s_attached_adapter == NULL) || (hcan == NULL) || (hcan != s_attached_adapter->hcan)) {
        return;
    }
    while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1) != 0U) {
        CAN_RxHeaderTypeDef header = {0};
        uint8_t data[ISOTP_MAX_FRAME_DATA] = {0};
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &header, data) != HAL_OK) {
            uds_stm32_can_note_error(s_attached_adapter, 1U, false);
            break;
        }
        uds_stm32_can_rx_from_isr(s_attached_adapter, header.StdId, data, (uint8_t)header.DLC);
    }
}
#endif

static uint32_t next_index(uint32_t index, uint32_t capacity) {
    return (index + 1U == capacity) ? 0U : index + 1U;
}

static bool valid_frame(uint32_t id, const uint8_t *data, uint8_t dlc) {
    return (data != NULL) && (dlc <= ISOTP_MAX_FRAME_DATA) && (id <= 0x7FFU);
}

static void clear_stats(UdsStm32CanStats *stats) {
    (void)memset(stats, 0, sizeof(*stats));
}

int uds_stm32_can_bind(UdsStm32Can *adapter, CAN_HandleTypeDef *hcan, uint32_t request_id,
                       uint32_t response_id) {
    if ((adapter == NULL) || (hcan == NULL) || (request_id > 0x7FFU) || (response_id > 0x7FFU) ||
        (request_id == response_id)) {
        return -EINVAL;
    }
    if (adapter->bound) {
        return -EBUSY;
    }
    adapter->hcan = hcan;
    adapter->request_id = request_id;
    adapter->response_id = response_id;
    adapter->bound = true;
    uds_stm32_can_reset(adapter);
    return 0;
}

int uds_stm32_can_attach(UdsStm32Can *adapter) {
    if ((adapter == NULL) || !adapter->bound || (adapter->hcan == NULL)) {
        return -EINVAL;
    }
#if defined(USE_HAL_CAN_REGISTER_CALLBACKS) && (USE_HAL_CAN_REGISTER_CALLBACKS != 0U)
    if ((s_attached_adapter != NULL) && (s_attached_adapter != adapter)) {
        return -EBUSY;
    }
    /* UDS filters are assigned to FIFO1. CANopenNode continues to own FIFO0,
     * so this registration does not replace the CANopen receive callback. */
    if (HAL_CAN_RegisterCallback(adapter->hcan, HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID,
                                 uds_stm32_can_rx_fifo1_callback) != HAL_OK) {
        return -EIO;
    }
    s_attached_adapter = adapter;
    return 0;
#else
    return -ENOTSUP;
#endif
}

void uds_stm32_can_detach(UdsStm32Can *adapter) {
#if defined(USE_HAL_CAN_REGISTER_CALLBACKS) && (USE_HAL_CAN_REGISTER_CALLBACKS != 0U)
    if ((s_attached_adapter == adapter) && (adapter != NULL) && (adapter->hcan != NULL)) {
        (void)HAL_CAN_UnRegisterCallback(adapter->hcan, HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID);
        s_attached_adapter = NULL;
    }
#else
    (void)adapter;
#endif
}

void uds_stm32_can_reset(UdsStm32Can *adapter) {
    if (adapter == NULL) {
        return;
    }
    __atomic_store_n(&adapter->rx_head, 0U, __ATOMIC_RELEASE);
    __atomic_store_n(&adapter->rx_tail, 0U, __ATOMIC_RELEASE);
    __atomic_store_n(&adapter->tx_head, 0U, __ATOMIC_RELEASE);
    __atomic_store_n(&adapter->tx_tail, 0U, __ATOMIC_RELEASE);
    clear_stats(&adapter->stats);
}

void uds_stm32_can_rx_from_isr(UdsStm32Can *adapter, uint32_t id, const uint8_t *data,
                               uint8_t dlc) {
    if ((adapter == NULL) || !adapter->bound || !valid_frame(id, data, dlc)) {
        if (adapter != NULL) {
            (void)__atomic_fetch_add(&adapter->stats.rx_invalid, 1U, __ATOMIC_RELAXED);
        }
        return;
    }
    if ((id != adapter->request_id) && (id != adapter->response_id)) {
        return;
    }

    uint32_t head = __atomic_load_n(&adapter->rx_head, __ATOMIC_RELAXED);
    uint32_t next = next_index(head, UDS_STM32_RX_QUEUE_CAPACITY);
    uint32_t tail = __atomic_load_n(&adapter->rx_tail, __ATOMIC_ACQUIRE);
    if (next == tail) {
        (void)__atomic_fetch_add(&adapter->stats.rx_overflow, 1U, __ATOMIC_RELAXED);
        return;
    }

    UdsStm32CanQueuedFrame *slot = &adapter->rx_queue[head];
    slot->id = id;
    slot->dlc = dlc;
    (void)memcpy(slot->data, data, dlc);
    __atomic_store_n(&adapter->rx_head, next, __ATOMIC_RELEASE);
    (void)__atomic_fetch_add(&adapter->stats.rx_frames, 1U, __ATOMIC_RELAXED);
}

int uds_stm32_can_rx_pop(UdsStm32Can *adapter, IsoTpCanFrame *frame) {
    if ((adapter == NULL) || (frame == NULL) || !adapter->bound) {
        return -EINVAL;
    }
    uint32_t tail = __atomic_load_n(&adapter->rx_tail, __ATOMIC_RELAXED);
    uint32_t head = __atomic_load_n(&adapter->rx_head, __ATOMIC_ACQUIRE);
    if (tail == head) {
        return 0;
    }

    const UdsStm32CanQueuedFrame *slot = &adapter->rx_queue[tail];
    frame->can_id = slot->id;
    frame->dlc = slot->dlc;
    (void)memcpy(frame->data, slot->data, slot->dlc);
    __atomic_store_n(&adapter->rx_tail, next_index(tail, UDS_STM32_RX_QUEUE_CAPACITY),
                     __ATOMIC_RELEASE);
    return 1;
}

int uds_stm32_can_tx_queue(UdsStm32Can *adapter, const IsoTpCanFrame *frame) {
    if ((adapter == NULL) || (frame == NULL) || !adapter->bound ||
        !valid_frame(frame->can_id, frame->data, frame->dlc) ||
        (frame->can_id != adapter->response_id)) {
        return -EINVAL;
    }
    uint32_t head = __atomic_load_n(&adapter->tx_head, __ATOMIC_RELAXED);
    uint32_t next = next_index(head, UDS_STM32_TX_QUEUE_CAPACITY);
    uint32_t tail = __atomic_load_n(&adapter->tx_tail, __ATOMIC_ACQUIRE);
    if (next == tail) {
        (void)__atomic_fetch_add(&adapter->stats.tx_overflow, 1U, __ATOMIC_RELAXED);
        return -EAGAIN;
    }

    UdsStm32CanQueuedFrame *slot = &adapter->tx_queue[head];
    slot->id = frame->can_id;
    slot->dlc = frame->dlc;
    (void)memcpy(slot->data, frame->data, frame->dlc);
    __atomic_store_n(&adapter->tx_head, next, __ATOMIC_RELEASE);
    return 0;
}

int uds_stm32_can_process_tx(UdsStm32Can *adapter, uint32_t budget) {
    if ((adapter == NULL) || !adapter->bound) {
        return -EINVAL;
    }
    uint32_t processed = 0U;
    while (processed < budget) {
        uint32_t tail = __atomic_load_n(&adapter->tx_tail, __ATOMIC_RELAXED);
        uint32_t head = __atomic_load_n(&adapter->tx_head, __ATOMIC_ACQUIRE);
        if (tail == head) {
            break;
        }
        if (HAL_CAN_GetTxMailboxesFreeLevel(adapter->hcan) == 0U) {
            (void)__atomic_fetch_add(&adapter->stats.tx_mailbox_exhausted, 1U, __ATOMIC_RELAXED);
            break;
        }

        UdsStm32CanQueuedFrame *slot = &adapter->tx_queue[tail];
        CAN_TxHeaderTypeDef header = {0};
        uint32_t mailbox = 0U;
        header.StdId = slot->id;
        header.ExtId = 0U;
        header.IDE = CAN_ID_STD;
        header.RTR = CAN_RTR_DATA;
        header.DLC = slot->dlc;
        header.TransmitGlobalTime = DISABLE;
        if (HAL_CAN_AddTxMessage(adapter->hcan, &header, slot->data, &mailbox) != HAL_OK) {
            (void)__atomic_fetch_add(&adapter->stats.bus_errors, 1U, __ATOMIC_RELAXED);
            break;
        }
        __atomic_store_n(&adapter->tx_tail, next_index(tail, UDS_STM32_TX_QUEUE_CAPACITY),
                         __ATOMIC_RELEASE);
        (void)__atomic_fetch_add(&adapter->stats.tx_frames, 1U, __ATOMIC_RELAXED);
        ++processed;
    }
    return (int)processed;
}

void uds_stm32_can_note_rx_timeout(UdsStm32Can *adapter) {
    if (adapter != NULL) {
        (void)__atomic_fetch_add(&adapter->stats.rx_timeouts, 1U, __ATOMIC_RELAXED);
    }
}

void uds_stm32_can_note_tx_timeout(UdsStm32Can *adapter) {
    if (adapter != NULL) {
        (void)__atomic_fetch_add(&adapter->stats.tx_timeouts, 1U, __ATOMIC_RELAXED);
    }
}

void uds_stm32_can_note_error(UdsStm32Can *adapter, uint32_t hal_error, bool bus_off) {
    if (adapter == NULL) {
        return;
    }
    if (hal_error != 0U) {
        (void)__atomic_fetch_add(&adapter->stats.bus_errors, 1U, __ATOMIC_RELAXED);
    }
    if (bus_off) {
        (void)__atomic_fetch_add(&adapter->stats.bus_off_events, 1U, __ATOMIC_RELAXED);
    }
}

void uds_stm32_can_get_stats(const UdsStm32Can *adapter, UdsStm32CanStats *stats) {
    if ((adapter == NULL) || (stats == NULL)) {
        return;
    }
    stats->rx_frames = __atomic_load_n(&adapter->stats.rx_frames, __ATOMIC_ACQUIRE);
    stats->tx_frames = __atomic_load_n(&adapter->stats.tx_frames, __ATOMIC_ACQUIRE);
    stats->rx_overflow = __atomic_load_n(&adapter->stats.rx_overflow, __ATOMIC_ACQUIRE);
    stats->tx_overflow = __atomic_load_n(&adapter->stats.tx_overflow, __ATOMIC_ACQUIRE);
    stats->tx_mailbox_exhausted =
        __atomic_load_n(&adapter->stats.tx_mailbox_exhausted, __ATOMIC_ACQUIRE);
    stats->rx_invalid = __atomic_load_n(&adapter->stats.rx_invalid, __ATOMIC_ACQUIRE);
    stats->rx_timeouts = __atomic_load_n(&adapter->stats.rx_timeouts, __ATOMIC_ACQUIRE);
    stats->tx_timeouts = __atomic_load_n(&adapter->stats.tx_timeouts, __ATOMIC_ACQUIRE);
    stats->bus_errors = __atomic_load_n(&adapter->stats.bus_errors, __ATOMIC_ACQUIRE);
    stats->bus_off_events = __atomic_load_n(&adapter->stats.bus_off_events, __ATOMIC_ACQUIRE);
}

bool uds_stm32_can_filter_match(const UdsStm32Can *adapter, uint32_t id) {
    return (adapter != NULL) && adapter->bound &&
           ((id == adapter->request_id) || (id == adapter->response_id));
}

uint32_t uds_stm32_can_request_id(const UdsStm32Can *adapter) {
    return (adapter != NULL) ? adapter->request_id : UDS_RX_CAN_ID;
}

uint32_t uds_stm32_can_response_id(const UdsStm32Can *adapter) {
    return (adapter != NULL) ? adapter->response_id : UDS_TX_CAN_ID;
}
