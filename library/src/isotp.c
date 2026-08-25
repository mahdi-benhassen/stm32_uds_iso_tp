#include "uds_iso_tp/isotp.h"

#include <stddef.h>

static bool expired(uint32_t now, uint32_t deadline) {
    return (int32_t)(now - deadline) >= 0;
}

static uint32_t min_u32(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

static uint32_t deadline(uint32_t now, uint32_t timeout) {
    return now + timeout;
}

static bool valid_fd_dl(uint8_t dl) {
    return (dl == 8U) || (dl == 12U) || (dl == 16U) || (dl == 20U) || (dl == 24U) || (dl == 32U) ||
           (dl == 48U) || (dl == 64U);
}

static bool valid_tx_config(const IsoTpConfig *config) {
    if (config == NULL)
        return false;
    return config->can_fd ? (valid_fd_dl(config->tx_dl) && valid_fd_dl(config->rx_dl))
                          : ((config->tx_dl == 8U) && (config->rx_dl == 8U));
}

static uint8_t fd_dl_for_length(uint32_t length) {
    static const uint8_t values[] = {8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};
    for (size_t index = 0U; index < (sizeof(values) / sizeof(values[0])); ++index) {
        if (length <= values[index]) {
            return values[index];
        }
    }
    return 64U;
}

static bool frame_valid(const IsoTpConfig *config, const IsoTpCanFrame *frame) {
    if ((frame->dlc == 0U) || (frame->dlc > ISOTP_MAX_FRAME_DATA)) {
        return false;
    }
    return !frame->is_fd ? (frame->dlc <= 8U) : (config->can_fd && valid_fd_dl(frame->dlc));
}

static void clear_frame(IsoTpCanFrame *frame, uint32_t id, const IsoTpConfig *config) {
    frame->can_id = id;
    frame->is_fd = config->can_fd;
    frame->bit_rate_switch = config->can_fd && config->bit_rate_switch;
    frame->dlc = config->can_fd ? config->tx_dl : 8U;
    for (uint8_t index = 0U; index < ISOTP_MAX_FRAME_DATA; ++index) {
        frame->data[index] = 0U;
    }
}

static bool valid_st_min(uint8_t value) {
    return (value <= 0x7FU) || ((value >= 0xF1U) && (value <= 0xF9U));
}

static uint32_t st_min_ms(uint8_t value) {
    return (value <= 0x7FU) ? (uint32_t)value : 1U;
}

void isotp_config_classic_can(IsoTpConfig *config) {
    if (config == NULL)
        return;
    config->block_size = 8U;
    config->st_min = 0U;
    config->rx_timeout_ms = ISOTP_DEFAULT_RX_TIMEOUT_MS;
    config->tx_timeout_ms = ISOTP_DEFAULT_TX_TIMEOUT_MS;
    config->max_wait_frames = ISOTP_DEFAULT_MAX_WAIT_FRAMES;
    config->tx_dl = 8U;
    config->rx_dl = 8U;
    config->can_fd = false;
    config->bit_rate_switch = false;
}

void isotp_config_default(IsoTpConfig *config) {
    isotp_config_classic_can(config);
}

void isotp_config_can_fd(IsoTpConfig *config, uint8_t tx_dl, uint8_t rx_dl) {
    if (config == NULL)
        return;
    isotp_config_classic_can(config);
    config->can_fd = true;
    config->tx_dl = valid_fd_dl(tx_dl) ? tx_dl : 64U;
    config->rx_dl = valid_fd_dl(rx_dl) ? rx_dl : 64U;
}

void isotp_rx_reset(IsoTpRx *rx) {
    if (rx == NULL)
        return;
    rx->expected_len = 0U;
    rx->received_len = 0U;
    rx->next_sequence = 1U;
    rx->block_count = 0U;
    rx->deadline_ms = 0U;
    rx->active = false;
}

void isotp_rx_init(IsoTpRx *rx, const IsoTpConfig *config, uint32_t request_id,
                   uint32_t response_id) {
    if (rx == NULL)
        return;
    if (config != NULL)
        rx->config = *config;
    else
        isotp_config_default(&rx->config);
    rx->request_id = request_id;
    rx->response_id = response_id;
    isotp_rx_reset(rx);
}

static void make_fc(const IsoTpRx *rx, IsoTpRxEvent *event) {
    clear_frame(&event->flow_control, rx->response_id, &rx->config);
    event->flow_control.dlc = rx->config.can_fd ? rx->config.tx_dl : 3U;
    event->flow_control.data[0] = 0x30U;
    event->flow_control.data[1] = rx->config.block_size;
    event->flow_control.data[2] = rx->config.st_min;
    event->has_flow_control = true;
}

static bool decode_sf(const IsoTpCanFrame *frame, uint32_t *length, uint8_t *header) {
    uint8_t low = (uint8_t)(frame->data[0] & 0x0FU);
    if (low != 0U) {
        *length = low;
        *header = 1U;
        return true;
    }
    if (!frame->is_fd || (frame->dlc < 2U) || (frame->data[1] == 0U))
        return false;
    *length = frame->data[1];
    *header = 2U;
    return true;
}

static bool decode_ff(const IsoTpCanFrame *frame, uint32_t *length, uint8_t *header) {
    uint8_t low = (uint8_t)(frame->data[0] & 0x0FU);
    if (!frame->is_fd || (low != 0U) || (frame->data[1] != 0U)) {
        *length = ((uint32_t)low << 8U) | frame->data[1];
        *header = 2U;
        return true;
    }
    if ((frame->dlc < 6U) || (frame->data[1] != 0U))
        return false;
    *length = ((uint32_t)frame->data[2] << 24U) | ((uint32_t)frame->data[3] << 16U) |
              ((uint32_t)frame->data[4] << 8U) | frame->data[5];
    *header = 6U;
    return true;
}

IsoTpStatus isotp_rx_feed(IsoTpRx *rx, const IsoTpCanFrame *frame, uint32_t now_ms,
                          IsoTpRxEvent *event) {
    if ((rx == NULL) || (frame == NULL) || (event == NULL) || !frame_valid(&rx->config, frame)) {
        return ISOTP_ERR_ARGUMENT;
    }
    event->payload = NULL;
    event->length = 0U;
    event->has_flow_control = false;
    if (frame->can_id != rx->request_id)
        return ISOTP_OK;
    uint8_t type = (uint8_t)(frame->data[0] >> 4U);
    if (type == 0U) {
        uint32_t length = 0U;
        uint8_t header = 0U;
        if (!decode_sf(frame, &length, &header) || (length > ((uint32_t)frame->dlc - header))) {
            return ISOTP_ERR_FORMAT;
        }
        if (length > ISOTP_MAX_PAYLOAD)
            return ISOTP_ERR_OVERFLOW;
        isotp_rx_reset(rx);
        for (uint32_t index = 0U; index < length; ++index)
            rx->buffer[index] = frame->data[index + header];
        event->payload = rx->buffer;
        event->length = length;
        return ISOTP_COMPLETE;
    }
    if (type == 1U) {
        if (frame->dlc < 2U)
            return ISOTP_ERR_FORMAT;
        uint32_t length = 0U;
        uint8_t header = 0U;
        if (!decode_ff(frame, &length, &header))
            return ISOTP_ERR_FORMAT;
        if (length <= 7U)
            return ISOTP_ERR_FORMAT;
        if (length > ISOTP_MAX_PAYLOAD) {
            isotp_rx_reset(rx);
            return ISOTP_ERR_OVERFLOW;
        }
        rx->expected_len = length;
        rx->received_len = min_u32((uint32_t)frame->dlc - header, length);
        for (uint32_t index = 0U; index < rx->received_len; ++index)
            rx->buffer[index] = frame->data[index + header];
        rx->next_sequence = 1U;
        rx->block_count = 0U;
        rx->deadline_ms = deadline(now_ms, rx->config.rx_timeout_ms);
        rx->active = rx->received_len < rx->expected_len;
        if (!rx->active) {
            event->payload = rx->buffer;
            event->length = rx->expected_len;
            return ISOTP_COMPLETE;
        }
        make_fc(rx, event);
        return ISOTP_NEED_FLOW_CONTROL;
    }
    if (type == 2U) {
        if (!rx->active)
            return ISOTP_ERR_STATE;
        if ((uint8_t)(frame->data[0] & 0x0FU) != rx->next_sequence) {
            isotp_rx_reset(rx);
            return ISOTP_ERR_SEQUENCE;
        }
        uint32_t copy_len = min_u32((uint32_t)frame->dlc - 1U, rx->expected_len - rx->received_len);
        for (uint32_t index = 0U; index < copy_len; ++index)
            rx->buffer[rx->received_len + index] = frame->data[index + 1U];
        rx->received_len += copy_len;
        rx->next_sequence = (uint8_t)((rx->next_sequence + 1U) & 0x0FU);
        rx->block_count = (uint8_t)(rx->block_count + 1U);
        rx->deadline_ms = deadline(now_ms, rx->config.rx_timeout_ms);
        if (rx->received_len >= rx->expected_len) {
            rx->active = false;
            event->payload = rx->buffer;
            event->length = rx->expected_len;
            return ISOTP_COMPLETE;
        }
        if ((rx->config.block_size != 0U) && (rx->block_count >= rx->config.block_size)) {
            rx->block_count = 0U;
            make_fc(rx, event);
            return ISOTP_NEED_FLOW_CONTROL;
        }
        return ISOTP_OK;
    }
    return (type == 3U) ? ISOTP_ERR_STATE : ISOTP_ERR_FORMAT;
}

IsoTpStatus isotp_rx_tick(IsoTpRx *rx, uint32_t now_ms) {
    if ((rx == NULL) || !rx->active)
        return ISOTP_OK;
    if (expired(now_ms, rx->deadline_ms)) {
        isotp_rx_reset(rx);
        return ISOTP_ERR_TIMEOUT;
    }
    return ISOTP_OK;
}

void isotp_tx_reset(IsoTpTx *tx) {
    if (tx == NULL)
        return;
    tx->payload_len = 0U;
    tx->offset = 0U;
    tx->next_sequence = 1U;
    tx->block_count = 0U;
    tx->remote_block_size = 0U;
    tx->remote_st_min = 0U;
    tx->deadline_ms = 0U;
    tx->next_frame_ms = 0U;
    tx->state = ISOTP_TX_STATE_IDLE;
    tx->wait_frames = 0U;
}

void isotp_tx_init(IsoTpTx *tx, const IsoTpConfig *config, uint32_t request_id,
                   uint32_t response_id) {
    if (tx == NULL)
        return;
    if (config != NULL)
        tx->config = *config;
    else
        isotp_config_default(&tx->config);
    tx->request_id = request_id;
    tx->response_id = response_id;
    isotp_tx_reset(tx);
}

IsoTpStatus isotp_tx_start(IsoTpTx *tx, const uint8_t *payload, uint32_t length, uint32_t now_ms,
                           IsoTpCanFrame *frame) {
    if ((tx == NULL) || (payload == NULL) || (frame == NULL) || (length == 0U) ||
        !valid_tx_config(&tx->config))
        return ISOTP_ERR_ARGUMENT;
    if (length > ISOTP_MAX_PAYLOAD)
        return ISOTP_ERR_OVERFLOW;
    for (uint32_t index = 0U; index < length; ++index)
        tx->buffer[index] = payload[index];
    tx->payload_len = length;
    tx->offset = 0U;
    tx->next_sequence = 1U;
    tx->block_count = 0U;
    tx->deadline_ms = deadline(now_ms, tx->config.tx_timeout_ms);
    if (length <= 7U) {
        clear_frame(frame, tx->response_id, &tx->config);
        frame->dlc = tx->config.can_fd ? fd_dl_for_length(length + 1U) : (uint8_t)(length + 1U);
        frame->data[0] = (uint8_t)length;
        for (uint32_t index = 0U; index < length; ++index)
            frame->data[index + 1U] = payload[index];
        isotp_tx_reset(tx);
        return ISOTP_TX_FRAME_READY;
    }
    uint32_t single_capacity = tx->config.can_fd ? ((uint32_t)tx->config.tx_dl - 2U) : 0U;
    if (tx->config.can_fd && (length <= single_capacity) && (length <= 255U)) {
        clear_frame(frame, tx->response_id, &tx->config);
        frame->dlc = fd_dl_for_length(length + 2U);
        frame->data[0] = 0U;
        frame->data[1] = (uint8_t)length;
        for (uint32_t index = 0U; index < length; ++index)
            frame->data[index + 2U] = payload[index];
        isotp_tx_reset(tx);
        return ISOTP_TX_FRAME_READY;
    }
    clear_frame(frame, tx->response_id, &tx->config);
    uint8_t header = (length <= 4095U) ? 2U : 6U;
    frame->data[0] = (length <= 4095U) ? (uint8_t)(0x10U | ((length >> 8U) & 0x0FU)) : 0x10U;
    frame->data[1] = (length <= 4095U) ? (uint8_t)length : 0U;
    if (length > 4095U) {
        frame->data[2] = (uint8_t)(length >> 24U);
        frame->data[3] = (uint8_t)(length >> 16U);
        frame->data[4] = (uint8_t)(length >> 8U);
        frame->data[5] = (uint8_t)length;
    }
    uint32_t first = min_u32((uint32_t)tx->config.tx_dl - header, length);
    for (uint32_t index = 0U; index < first; ++index)
        frame->data[index + header] = payload[index];
    tx->offset = first;
    tx->state = ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL;
    return ISOTP_TX_FRAME_READY;
}

IsoTpStatus isotp_tx_feed_flow_control(IsoTpTx *tx, const IsoTpCanFrame *frame, uint32_t now_ms) {
    if ((tx == NULL) || (frame == NULL) || !frame_valid(&tx->config, frame))
        return ISOTP_ERR_ARGUMENT;
    if (frame->can_id != tx->request_id)
        return ISOTP_ERR_ARGUMENT;
    if ((tx->state != ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL) &&
        (tx->state != ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL))
        return ISOTP_ERR_STATE;
    if ((frame->dlc < 3U) || ((frame->data[0] >> 4U) != 3U))
        return ISOTP_ERR_FORMAT;
    uint8_t flow = (uint8_t)(frame->data[0] & 0x0FU);
    if (flow == ISOTP_FC_OVERFLOW) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_FLOW_OVERFLOW;
    }
    if ((flow != ISOTP_FC_CTS) && (flow != ISOTP_FC_WAIT)) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_FLOW_CONTROL;
    }
    if (!valid_st_min(frame->data[2])) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_FLOW_CONTROL;
    }
    /* BS is an unsigned octet: zero means unlimited and every non-zero value is valid. */
    if (flow == ISOTP_FC_WAIT) {
        if (tx->wait_frames >= tx->config.max_wait_frames) {
            isotp_tx_reset(tx);
            return ISOTP_ERR_FLOW_CONTROL;
        }
        tx->wait_frames = (uint8_t)(tx->wait_frames + 1U);
        tx->deadline_ms = deadline(now_ms, tx->config.tx_timeout_ms);
        return ISOTP_OK;
    }
    tx->remote_block_size = frame->data[1];
    tx->remote_st_min = frame->data[2];
    tx->wait_frames = 0U;
    tx->state = ISOTP_TX_STATE_SEND_CONSECUTIVE;
    tx->block_count = 0U;
    tx->next_frame_ms = now_ms;
    tx->deadline_ms = deadline(now_ms, tx->config.tx_timeout_ms);
    return ISOTP_OK;
}

IsoTpStatus isotp_tx_next(IsoTpTx *tx, uint32_t now_ms, IsoTpCanFrame *frame) {
    if ((tx == NULL) || (frame == NULL))
        return ISOTP_ERR_ARGUMENT;
    if (tx->state == ISOTP_TX_STATE_IDLE)
        return ISOTP_OK;
    if ((tx->state == ISOTP_TX_STATE_WAIT_FIRST_FLOW_CONTROL) ||
        (tx->state == ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL))
        return ISOTP_OK;
    if (expired(now_ms, tx->deadline_ms)) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_TIMEOUT;
    }
    if (!expired(now_ms, tx->next_frame_ms))
        return ISOTP_OK;
    if (tx->offset >= tx->payload_len) {
        isotp_tx_reset(tx);
        return ISOTP_COMPLETE;
    }
    uint32_t copy = min_u32(tx->payload_len - tx->offset, (uint32_t)tx->config.tx_dl - 1U);
    clear_frame(frame, tx->response_id, &tx->config);
    frame->data[0] = (uint8_t)(0x20U | tx->next_sequence);
    for (uint32_t index = 0U; index < copy; ++index)
        frame->data[index + 1U] = tx->buffer[tx->offset + index];
    tx->offset += copy;
    tx->next_sequence = (uint8_t)((tx->next_sequence + 1U) & 0x0FU);
    tx->block_count = (uint8_t)(tx->block_count + 1U);
    tx->deadline_ms = deadline(now_ms, tx->config.tx_timeout_ms);
    tx->next_frame_ms = now_ms + st_min_ms(tx->remote_st_min);
    if ((tx->remote_block_size != 0U) && (tx->block_count >= tx->remote_block_size) &&
        (tx->offset < tx->payload_len)) {
        tx->block_count = 0U;
        tx->state = ISOTP_TX_STATE_WAIT_BLOCK_FLOW_CONTROL;
    }
    return ISOTP_TX_FRAME_READY;
}

IsoTpTxState isotp_tx_state(const IsoTpTx *tx) {
    return (tx != NULL) ? tx->state : ISOTP_TX_STATE_IDLE;
}

IsoTpStatus isotp_tx_tick(IsoTpTx *tx, uint32_t now_ms) {
    if ((tx == NULL) || (tx->state == ISOTP_TX_STATE_IDLE))
        return ISOTP_OK;
    if (expired(now_ms, tx->deadline_ms)) {
        isotp_tx_reset(tx);
        return ISOTP_ERR_TIMEOUT;
    }
    return ISOTP_OK;
}
