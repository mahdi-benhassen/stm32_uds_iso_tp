/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "isotp.h"

#include <stddef.h>

static bool deadline_expired(uint32_t now_ms, uint32_t deadline_ms) {
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static uint16_t min_u16(uint16_t left, uint16_t right) {
    return (left < right) ? left : right;
}

static void clear_frame(IsoTpCanFrame *frame, uint32_t can_id) {
    frame->can_id = can_id;
    frame->dlc = 0U;
    for (uint8_t index = 0U; index < ISOTP_MAX_FRAME_DATA; ++index) {
        frame->data[index] = 0U;
    }
}

static uint32_t add_timeout(uint32_t now_ms, uint32_t timeout_ms) {
    return now_ms + timeout_ms;
}

static uint32_t st_min_to_ms(uint8_t st_min) {
    if (st_min <= 0x7FU) {
        return (uint32_t)st_min;
    }
    /* 0xF1..0xF9 encode 100..900 us. A millisecond clock uses the
     * conservative one-millisecond minimum rather than sending early. */
    return 1U;
}

void isotp_config_default(IsoTpConfig *config) {
    if (config == NULL) {
        return;
    }
    config->block_size = 8U;
    config->st_min = 0U;
    config->rx_timeout_ms = ISOTP_DEFAULT_RX_TIMEOUT_MS;
    config->tx_timeout_ms = ISOTP_DEFAULT_TX_TIMEOUT_MS;
    config->max_wait_frames = ISOTP_DEFAULT_MAX_WAIT_FRAMES;
}

void isotp_rx_init(IsoTpRx *rx, const IsoTpConfig *config, uint32_t request_id,
                   uint32_t response_id) {
    if (rx == NULL) {
        return;
    }
    rx->config = (config != NULL) ? *config : (IsoTpConfig){0};
    if (config == NULL) {
        isotp_config_default(&rx->config);
    }
    rx->request_id = request_id;
    rx->response_id = response_id;
    isotp_rx_reset(rx);
}

void isotp_rx_reset(IsoTpRx *rx) {
    if (rx == NULL) {
        return;
    }
    rx->expected_len = 0U;
    rx->received_len = 0U;
    rx->next_sequence = 1U;
    rx->block_count = 0U;
    rx->deadline_ms = 0U;
    rx->active = false;
}

static void rx_make_flow_control(const IsoTpRx *rx, IsoTpRxEvent *event) {
    clear_frame(&event->flow_control, rx->response_id);
    event->flow_control.dlc = 3U;
    event->flow_control.data[0] = 0x30U;
    event->flow_control.data[1] = rx->config.block_size;
    event->flow_control.data[2] = rx->config.st_min;
    event->has_flow_control = true;
}

IsoTpStatus isotp_rx_feed(IsoTpRx *rx, const IsoTpCanFrame *frame, uint32_t now_ms,
                          IsoTpRxEvent *event) {
    if ((rx == NULL) || (frame == NULL) || (event == NULL) || (frame->dlc > ISOTP_MAX_FRAME_DATA)) {
        return ISOTP_ERR_ARGUMENT;
    }
    event->payload = NULL;
    event->length = 0U;
    event->has_flow_control = false;
    if (frame->can_id != rx->request_id) {
        return ISOTP_OK;
    }
    if (frame->dlc == 0U) {
        return ISOTP_ERR_FORMAT;
    }

    uint8_t pci_type = (uint8_t)(frame->data[0] >> 4U);
    if (pci_type == 0U) {
        uint8_t length = (uint8_t)(frame->data[0] & 0x0FU);
        if ((length == 0U) || (length > 7U) || ((uint16_t)length > (uint16_t)frame->dlc - 1U)) {
            return ISOTP_ERR_FORMAT;
        }
        isotp_rx_reset(rx);
        for (uint8_t index = 0U; index < length; ++index) {
            rx->buffer[index] = frame->data[(uint8_t)(index + 1U)];
        }
        event->payload = rx->buffer;
        event->length = length;
        return ISOTP_COMPLETE;
    }

    if (pci_type == 1U) {
        if (frame->dlc < 2U) {
            return ISOTP_ERR_FORMAT;
        }
        uint16_t length = (uint16_t)(((uint16_t)(frame->data[0] & 0x0FU) << 8U) | frame->data[1]);
        if ((length <= 7U) || (length > ISOTP_MAX_PAYLOAD)) {
            isotp_rx_reset(rx);
            return (length > ISOTP_MAX_PAYLOAD) ? ISOTP_ERR_OVERFLOW : ISOTP_ERR_FORMAT;
        }
        rx->expected_len = length;
        rx->received_len = min_u16((uint16_t)(frame->dlc - 2U), length);
        for (uint16_t index = 0U; index < rx->received_len; ++index) {
            rx->buffer[index] = frame->data[index + 2U];
        }
        rx->next_sequence = 1U;
        rx->block_count = 0U;
        rx->deadline_ms = add_timeout(now_ms, rx->config.rx_timeout_ms);
        rx->active = true;
        rx_make_flow_control(rx, event);
        return ISOTP_NEED_FLOW_CONTROL;
    }

    if (pci_type == 2U) {
        if (!rx->active) {
            return ISOTP_ERR_STATE;
        }
        uint8_t sequence = (uint8_t)(frame->data[0] & 0x0FU);
        if (sequence != rx->next_sequence) {
            isotp_rx_reset(rx);
            return ISOTP_ERR_SEQUENCE;
        }
        uint16_t available = (frame->dlc > 1U) ? (uint16_t)(frame->dlc - 1U) : 0U;
        uint16_t remaining = (uint16_t)(rx->expected_len - rx->received_len);
        uint16_t copy_len = min_u16(available, remaining);
        for (uint16_t index = 0U; index < copy_len; ++index) {
            rx->buffer[rx->received_len + index] = frame->data[index + 1U];
        }
        rx->received_len = (uint16_t)(rx->received_len + copy_len);
        rx->next_sequence = (uint8_t)((rx->next_sequence + 1U) & 0x0FU);
        rx->block_count = (uint8_t)(rx->block_count + 1U);
        rx->deadline_ms = add_timeout(now_ms, rx->config.rx_timeout_ms);
        if (rx->received_len >= rx->expected_len) {
            event->payload = rx->buffer;
            event->length = rx->expected_len;
            rx->active = false;
            return ISOTP_COMPLETE;
        }
        if ((rx->config.block_size != 0U) && (rx->block_count >= rx->config.block_size)) {
            rx->block_count = 0U;
            rx_make_flow_control(rx, event);
            return ISOTP_NEED_FLOW_CONTROL;
        }
        return ISOTP_OK;
    }

    /* A flow-control frame belongs to the transmitter, not this receiver. */
    return (pci_type == 3U) ? ISOTP_ERR_STATE : ISOTP_ERR_FORMAT;
}

IsoTpStatus isotp_rx_tick(IsoTpRx *rx, uint32_t now_ms) {
    if ((rx == NULL) || !rx->active) {
        return ISOTP_OK;
    }
    if (deadline_expired(now_ms, rx->deadline_ms)) {
        isotp_rx_reset(rx);
        return ISOTP_ERR_TIMEOUT;
    }
    return ISOTP_OK;
}

void isotp_tx_init(IsoTpTx *tx, const IsoTpConfig *config, uint32_t request_id,
                   uint32_t response_id) {
    if (tx == NULL) {
        return;
    }
    tx->config = (config != NULL) ? *config : (IsoTpConfig){0};
    if (config == NULL) {
        isotp_config_default(&tx->config);
    }
    tx->request_id = request_id;
    tx->response_id = response_id;
    isotp_tx_reset(tx);
}

void isotp_tx_reset(IsoTpTx *tx) {
    if (tx == NULL) {
        return;
    }
    tx->payload_len = 0U;
    tx->offset = 0U;
    tx->next_sequence = 1U;
    tx->block_count = 0U;
    tx->remote_block_size = 0U;
    tx->remote_st_min = 0U;
    tx->deadline_ms = 0U;
    tx->next_frame_ms = 0U;
    tx->active = false;
    tx->waiting_flow_control = false;
    tx->wait_frames = 0U;
}

IsoTpStatus isotp_tx_start(IsoTpTx *tx, const uint8_t *payload, uint16_t length, uint32_t now_ms,
                           IsoTpCanFrame *frame) {
    if ((tx == NULL) || (payload == NULL) || (frame == NULL) || (length == 0U)) {
        return ISOTP_ERR_ARGUMENT;
    }
    if (length > ISOTP_MAX_PAYLOAD) {
        return ISOTP_ERR_OVERFLOW;
    }
    for (uint16_t index = 0U; index < length; ++index) {
        tx->buffer[index] = payload[index];
    }
    tx->payload_len = length;
    tx->offset = 0U;
    tx->next_sequence = 1U;
    tx->block_count = 0U;
    tx->deadline_ms = add_timeout(now_ms, tx->config.tx_timeout_ms);
    clear_frame(frame, tx->response_id);
    if (length <= 7U) {
        frame->dlc = (uint8_t)(length + 1U);
        frame->data[0] = (uint8_t)length;
        for (uint16_t index = 0U; index < length; ++index) {
            frame->data[index + 1U] = payload[index];
        }
        isotp_tx_reset(tx);
        return ISOTP_TX_FRAME_READY;
    }

    frame->dlc = 8U;
    frame->data[0] = (uint8_t)(0x10U | ((length >> 8U) & 0x0FU));
    frame->data[1] = (uint8_t)(length & 0xFFU);
    for (uint8_t index = 0U; index < 6U; ++index) {
        frame->data[index + 2U] = payload[index];
    }
    tx->offset = 6U;
    tx->active = true;
    tx->waiting_flow_control = true;
    return ISOTP_TX_FRAME_READY;
}

IsoTpStatus isotp_tx_feed_flow_control(IsoTpTx *tx, const IsoTpCanFrame *frame, uint32_t now_ms) {
    if ((tx == NULL) || (frame == NULL) || (frame->dlc > ISOTP_MAX_FRAME_DATA)) {
        return ISOTP_ERR_ARGUMENT;
    }
    if (frame->can_id != tx->request_id) {
        return ISOTP_OK;
    }
    if (!tx->active || !tx->waiting_flow_control || (frame->dlc < 3U) ||
        ((frame->data[0] >> 4U) != 3U)) {
        return ISOTP_ERR_STATE;
    }
    uint8_t flow_status = (uint8_t)(frame->data[0] & 0x0FU);
    if (flow_status == ISOTP_FC_OVERFLOW) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_FLOW_CONTROL;
    }
    if ((flow_status != ISOTP_FC_CTS) && (flow_status != ISOTP_FC_WAIT)) {
        return ISOTP_ERR_FLOW_CONTROL;
    }
    uint8_t st_min = frame->data[2];
    if (!((st_min <= 0x7FU) || ((st_min >= 0xF1U) && (st_min <= 0xF9U)))) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_FLOW_CONTROL;
    }
    tx->remote_block_size = frame->data[1];
    tx->remote_st_min = st_min;
    tx->deadline_ms = add_timeout(now_ms, tx->config.tx_timeout_ms);
    if (flow_status == ISOTP_FC_WAIT) {
        if (tx->wait_frames >= tx->config.max_wait_frames) {
            isotp_tx_reset(tx);
            return ISOTP_ERR_FLOW_CONTROL;
        }
        tx->wait_frames = (uint8_t)(tx->wait_frames + 1U);
        tx->waiting_flow_control = true;
        return ISOTP_OK;
    }
    tx->wait_frames = 0U;
    tx->waiting_flow_control = false;
    tx->block_count = 0U;
    tx->next_frame_ms = now_ms;
    return ISOTP_OK;
}

IsoTpStatus isotp_tx_next(IsoTpTx *tx, uint32_t now_ms, IsoTpCanFrame *frame) {
    if ((tx == NULL) || (frame == NULL)) {
        return ISOTP_ERR_ARGUMENT;
    }
    if (!tx->active) {
        return ISOTP_OK;
    }
    if (tx->waiting_flow_control) {
        return ISOTP_OK;
    }
    if (deadline_expired(now_ms, tx->deadline_ms)) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_TIMEOUT;
    }
    if (!deadline_expired(now_ms, tx->next_frame_ms)) {
        return ISOTP_OK;
    }
    if (tx->offset >= tx->payload_len) {
        isotp_tx_reset(tx);
        return ISOTP_COMPLETE;
    }
    uint16_t remaining = (uint16_t)(tx->payload_len - tx->offset);
    uint8_t copy_len = (remaining > 7U) ? 7U : (uint8_t)remaining;
    clear_frame(frame, tx->response_id);
    frame->dlc = (uint8_t)(copy_len + 1U);
    frame->data[0] = (uint8_t)(0x20U | tx->next_sequence);
    for (uint8_t index = 0U; index < copy_len; ++index) {
        frame->data[index + 1U] = tx->buffer[tx->offset + index];
    }
    tx->offset = (uint16_t)(tx->offset + copy_len);
    tx->next_sequence = (uint8_t)((tx->next_sequence + 1U) & 0x0FU);
    tx->block_count = (uint8_t)(tx->block_count + 1U);
    tx->deadline_ms = add_timeout(now_ms, tx->config.tx_timeout_ms);
    tx->next_frame_ms = now_ms + st_min_to_ms(tx->remote_st_min);
    if ((tx->remote_block_size != 0U) && (tx->block_count >= tx->remote_block_size) &&
        (tx->offset < tx->payload_len)) {
        tx->block_count = 0U;
        tx->waiting_flow_control = true;
    }
    if (tx->offset >= tx->payload_len) {
        /* Keep active until the caller has consumed this final CF; the next
         * call returns ISOTP_COMPLETE and resets state. */
    }
    return ISOTP_TX_FRAME_READY;
}

IsoTpStatus isotp_tx_tick(IsoTpTx *tx, uint32_t now_ms) {
    if ((tx == NULL) || !tx->active) {
        return ISOTP_OK;
    }
    if (deadline_expired(now_ms, tx->deadline_ms)) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_TIMEOUT;
    }
    return ISOTP_OK;
}
