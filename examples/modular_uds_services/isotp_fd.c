/**
 * @file isotp_fd.c
 * @brief ISO 15765-2:2016 ISO-TP Transport Layer Implementation (CAN 2.0B & CAN-FD 64-byte)
 */

#include "isotp_fd.h"
#include <string.h>

static bool send_can_frame(isotp_fd_channel_t *ch, uint32_t id, const uint8_t *data, uint8_t len) {
    can_frame_t f;
    memset(&f, 0, sizeof(f));
    bool is_ext = ch->tx_is_ext || (id > 0x7FFU);
    f.id     = id & (is_ext ? ISOTP_CAN_ID_MASK_EXT : ISOTP_CAN_ID_MASK_STD);
    f.is_ext = is_ext;
    f.is_rtr = false;
    f.is_fd  = ch->use_fd;

    uint8_t target_len = ch->use_fd ? can_fd_pad_len(len) : (len < 8U ? 8U : len);
    if (target_len > CAN_FD_MAX_DLEN) target_len = CAN_FD_MAX_DLEN;
    f.dlc = target_len;

    if (len > 0U) {
        memcpy(f.data, data, len);
    }
    /* Pad with 0xCC */
    for (uint8_t i = len; i < target_len; i++) {
        f.data[i] = 0xCCU;
    }

    if (ch->tx_cb == NULL) return false;
    return ch->tx_cb(&f, ch->tx_user);
}

static void send_fc(isotp_fd_channel_t *ch, isotp_fc_status_t fs) {
    uint8_t buf[8];
    buf[0] = (uint8_t)((ISOTP_PCI_FC << 4U) | ((uint8_t)fs & 0x0FU));
    buf[1] = ISOTP_FD_BS;
    buf[2] = ISOTP_FD_STMIN_MS;
    send_can_frame(ch, ch->tx_id, buf, 3U);
}

void isotp_fd_init(isotp_fd_channel_t *ch, uint32_t rx_id, uint32_t tx_id,
                   bool use_fd, isotp_fd_tx_fn tx_cb, void *tx_user) {
    bool rx_ext = (rx_id > 0x7FFU);
    bool tx_ext = (tx_id > 0x7FFU);
    isotp_fd_init_ext(ch, rx_id, tx_id, rx_ext, tx_ext, use_fd, tx_cb, tx_user);
}

void isotp_fd_init_ext(isotp_fd_channel_t *ch, uint32_t rx_id, uint32_t tx_id,
                       bool rx_is_ext, bool tx_is_ext,
                       bool use_fd, isotp_fd_tx_fn tx_cb, void *tx_user) {
    memset(ch, 0, sizeof(*ch));
    ch->rx_id     = rx_id;
    ch->tx_id     = tx_id;
    ch->rx_is_ext = rx_is_ext;
    ch->tx_is_ext = tx_is_ext;
    ch->use_fd    = use_fd;
    ch->tx_cb     = tx_cb;
    ch->tx_user   = tx_user;
    ch->rx.state  = ISOTP_RX_IDLE;
    ch->tx.state  = ISOTP_TX_IDLE;
}

static void handle_sf(isotp_fd_channel_t *ch, const can_frame_t *frame) {
    uint8_t pci_dl = frame->data[0] & 0x0FU;
    uint32_t sf_len = 0U;
    uint8_t  data_offset = 0U;

    if (pci_dl != 0U) {
        /* Classic CAN SF (or CAN FD short SF) */
        sf_len = pci_dl;
        data_offset = 1U;
    } else {
        /* CAN-FD Single Frame with 8-bit length (ISO 15765-2:2016) */
        if (frame->dlc < 2U) return;
        sf_len = frame->data[1];
        data_offset = 2U;
    }

    if ((sf_len == 0U) || (frame->dlc < data_offset) || (sf_len > (uint32_t)(frame->dlc - data_offset)) || (sf_len > ISOTP_FD_RX_BUF_SIZE)) {
        return;
    }

    memcpy(ch->rx.buf, &frame->data[data_offset], sf_len);
    ch->rx.msg_len = sf_len;
    ch->rx.offset  = sf_len;
    ch->rx.state   = ISOTP_RX_IDLE;
}

static void handle_ff(isotp_fd_channel_t *ch, const can_frame_t *frame, uint32_t now_ms) {
    uint16_t ff_dl_12 = ((uint16_t)(frame->data[0] & 0x0FU) << 8U) | (uint16_t)frame->data[1];
    uint32_t total_len = 0U;
    uint8_t  data_offset = 0U;

    if (ff_dl_12 != 0U) {
        /* Standard 12-bit First Frame (<= 4095 bytes) */
        total_len = ff_dl_12;
        data_offset = 2U;
    } else {
        /* Extended 32-bit First Frame (> 4095 bytes) */
        if (frame->dlc < 6U) return;
        total_len = ((uint32_t)frame->data[2] << 24U) |
                    ((uint32_t)frame->data[3] << 16U) |
                    ((uint32_t)frame->data[4] << 8U)  |
                    (uint32_t)frame->data[5];
        data_offset = 6U;
    }

    if (total_len > ISOTP_FD_RX_BUF_SIZE) {
        send_fc(ch, ISOTP_FC_OVFLW);
        return;
    }

    uint8_t first_bytes = (frame->dlc > data_offset) ? (uint8_t)(frame->dlc - data_offset) : 0U;
    if (first_bytes > total_len) first_bytes = (uint8_t)total_len;

    memcpy(ch->rx.buf, &frame->data[data_offset], first_bytes);
    ch->rx.msg_len  = total_len;
    ch->rx.offset   = first_bytes;
    ch->rx.seq      = 1U;
    ch->rx.bs       = ISOTP_FD_BS;
    ch->rx.bs_count = 0U;
    ch->rx.state    = ISOTP_RX_WAIT_CF;
    ch->rx.timer_ms = now_ms;

    send_fc(ch, ISOTP_FC_CTS);
}

static void handle_cf(isotp_fd_channel_t *ch, const can_frame_t *frame, uint32_t now_ms) {
    if (ch->rx.state != ISOTP_RX_WAIT_CF) return;

    uint8_t sn = frame->data[0] & 0x0FU;
    if (sn != (ch->rx.seq & 0x0FU)) {
        ch->rx.state = ISOTP_RX_IDLE; /* Sequence error abort */
        return;
    }

    uint32_t remaining = ch->rx.msg_len - ch->rx.offset;
    uint8_t  cf_bytes  = (frame->dlc > 1U) ? (frame->dlc - 1U) : 0U;
    if (cf_bytes > remaining) cf_bytes = (uint8_t)remaining;

    memcpy(&ch->rx.buf[ch->rx.offset], &frame->data[1], cf_bytes);
    ch->rx.offset += cf_bytes;
    ch->rx.seq++;
    ch->rx.bs_count++;
    ch->rx.timer_ms = now_ms;

    if (ch->rx.offset >= ch->rx.msg_len) {
        ch->rx.state = ISOTP_RX_IDLE;
        return;
    }

    if ((ch->rx.bs != 0U) && (ch->rx.bs_count >= ch->rx.bs)) {
        ch->rx.bs_count = 0U;
        send_fc(ch, ISOTP_FC_CTS);
    }
}

static void handle_fc_rx(isotp_fd_channel_t *ch, const can_frame_t *frame, uint32_t now_ms) {
    if (ch->tx.state != ISOTP_TX_WAIT_FC) return;

    isotp_fc_status_t fs = (isotp_fc_status_t)(frame->data[0] & 0x0FU);
    switch (fs) {
    case ISOTP_FC_CTS:
        ch->tx.bs       = frame->data[1];
        ch->tx.stmin    = frame->data[2];
        ch->tx.bs_count = 0U;
        ch->tx.state    = ISOTP_TX_SEND_CF;
        ch->tx.timer_ms = now_ms;
        break;
    case ISOTP_FC_WAIT:
        ch->tx.wft_count++;
        if (ch->tx.wft_count > ISOTP_FD_WFT_MAX) {
            ch->tx.state = ISOTP_TX_IDLE;
        } else {
            ch->tx.timer_ms = now_ms;
        }
        break;
    case ISOTP_FC_OVFLW:
    default:
        ch->tx.state = ISOTP_TX_IDLE;
        break;
    }
}

void isotp_fd_on_rx(isotp_fd_channel_t *ch, const can_frame_t *frame, uint32_t now_ms) {
    if ((ch == NULL) || (frame == NULL) || (frame->dlc < 1U)) return;

    /* Extended / Standard ID matching (Issue #123) */
    if (frame->is_ext != ch->rx_is_ext) return;
    uint32_t mask = ch->rx_is_ext ? ISOTP_CAN_ID_MASK_EXT : ISOTP_CAN_ID_MASK_STD;
    if ((frame->id & mask) != (ch->rx_id & mask)) return;

    isotp_pci_type_t pci = (isotp_pci_type_t)(frame->data[0] >> 4U);
    switch (pci) {
    case ISOTP_PCI_SF: handle_sf(ch, frame); break;
    case ISOTP_PCI_FF: handle_ff(ch, frame, now_ms); break;
    case ISOTP_PCI_CF: handle_cf(ch, frame, now_ms); break;
    case ISOTP_PCI_FC: handle_fc_rx(ch, frame, now_ms); break;
    default: break;
    }
}

static void tx_send_cf(isotp_fd_channel_t *ch, uint32_t now_ms) {
    uint8_t buf[CAN_FD_MAX_DLEN];
    uint32_t remaining = ch->tx.msg_len - ch->tx.offset;
    uint8_t max_cf_payload = ch->use_fd ? (CAN_FD_MAX_DLEN - 1U) : 7U;
    uint8_t cf_bytes = (remaining > max_cf_payload) ? max_cf_payload : (uint8_t)remaining;

    buf[0] = (uint8_t)((ISOTP_PCI_CF << 4U) | (ch->tx.seq & 0x0FU));
    memcpy(&buf[1], &ch->tx.buf[ch->tx.offset], cf_bytes);

    if (!send_can_frame(ch, ch->tx_id, buf, (uint8_t)(1U + cf_bytes))) {
        return;
    }

    ch->tx.offset += cf_bytes;
    ch->tx.seq++;
    ch->tx.bs_count++;
    ch->tx.timer_ms = now_ms;

    if (ch->tx.offset >= ch->tx.msg_len) {
        ch->tx.state = ISOTP_TX_IDLE;
        return;
    }

    if ((ch->tx.bs != 0U) && (ch->tx.bs_count >= ch->tx.bs)) {
        ch->tx.state = ISOTP_TX_WAIT_FC;
        ch->tx.wft_count = 0U;
        ch->tx.timer_ms = now_ms;
    }
}

void isotp_fd_poll(isotp_fd_channel_t *ch, uint32_t now_ms) {
    if (ch->rx.state == ISOTP_RX_WAIT_CF) {
        if ((now_ms - ch->rx.timer_ms) >= ISOTP_FD_TIMEOUT_MS) {
            ch->rx.state = ISOTP_RX_IDLE;
        }
    }
    if (ch->tx.state == ISOTP_TX_WAIT_FC) {
        if ((now_ms - ch->tx.timer_ms) >= ISOTP_FD_TIMEOUT_MS) {
            ch->tx.state = ISOTP_TX_IDLE;
        }
    }
    if (ch->tx.state == ISOTP_TX_SEND_CF) {
        if ((now_ms - ch->tx.timer_ms) >= ch->tx.stmin) {
            tx_send_cf(ch, now_ms);
        }
    }
}

isotp_result_t isotp_fd_send(isotp_fd_channel_t *ch, const uint8_t *data, uint32_t len, uint32_t now_ms) {
    if (ch->tx.state != ISOTP_TX_IDLE) return ISOTP_BUSY;
    if ((len == 0U) || (len > ISOTP_FD_TX_BUF_SIZE)) return ISOTP_ERROR;

    /* CAN Single Frame transmission */
    if (!ch->use_fd && (len <= 7U)) {
        uint8_t buf[8];
        buf[0] = (uint8_t)((ISOTP_PCI_SF << 4U) | (len & 0x0FU));
        memcpy(&buf[1], data, len);
        if (!send_can_frame(ch, ch->tx_id, buf, (uint8_t)(1U + len))) return ISOTP_ERROR;
        return ISOTP_OK;
    }

    /* CAN-FD Single Frame transmission up to 62 bytes */
    if (ch->use_fd && (len <= 62U)) {
        uint8_t buf[CAN_FD_MAX_DLEN];
        uint8_t hdr_len = 0U;
        if (len <= 7U) {
            buf[0] = (uint8_t)((ISOTP_PCI_SF << 4U) | (len & 0x0FU));
            hdr_len = 1U;
        } else {
            buf[0] = (uint8_t)(ISOTP_PCI_SF << 4U);
            buf[1] = (uint8_t)len;
            hdr_len = 2U;
        }
        memcpy(&buf[hdr_len], data, len);
        if (!send_can_frame(ch, ch->tx_id, buf, (uint8_t)(hdr_len + len))) return ISOTP_ERROR;
        return ISOTP_OK;
    }

    /* Multi-frame transmission */
    memcpy(ch->tx.buf, data, len);
    ch->tx.msg_len = len;

    uint8_t buf[CAN_FD_MAX_DLEN];
    uint8_t hdr_len = 0U;

    if (len <= 4095U) {
        buf[0] = (uint8_t)((ISOTP_PCI_FF << 4U) | ((len >> 8U) & 0x0FU));
        buf[1] = (uint8_t)(len & 0xFFU);
        hdr_len = 2U;
    } else {
        buf[0] = (uint8_t)(ISOTP_PCI_FF << 4U);
        buf[1] = 0x00U;
        buf[2] = (uint8_t)(len >> 24U);
        buf[3] = (uint8_t)(len >> 16U);
        buf[4] = (uint8_t)(len >> 8U);
        buf[5] = (uint8_t)(len & 0xFFU);
        hdr_len = 6U;
    }

    uint8_t max_first_payload = ch->use_fd ? (CAN_FD_MAX_DLEN - hdr_len) : (8U - hdr_len);
    uint8_t first_bytes = (len > max_first_payload) ? max_first_payload : (uint8_t)len;
    memcpy(&buf[hdr_len], data, first_bytes);

    if (!send_can_frame(ch, ch->tx_id, buf, (uint8_t)(hdr_len + first_bytes))) {
        return ISOTP_ERROR;
    }

    ch->tx.offset    = first_bytes;
    ch->tx.seq       = 1U;
    ch->tx.state     = ISOTP_TX_WAIT_FC;
    ch->tx.wft_count = 0U;
    ch->tx.timer_ms  = now_ms;

    return ISOTP_OK;
}

bool isotp_fd_rx_ready(const isotp_fd_channel_t *ch) {
    return ((ch->rx.state == ISOTP_RX_IDLE) &&
            (ch->rx.offset > 0U) &&
            (ch->rx.offset >= ch->rx.msg_len));
}

const uint8_t *isotp_fd_rx_data(const isotp_fd_channel_t *ch, uint32_t *len) {
    if (len != NULL) *len = ch->rx.msg_len;
    return ch->rx.buf;
}

void isotp_fd_rx_done(isotp_fd_channel_t *ch) {
    ch->rx.offset  = 0U;
    ch->rx.msg_len = 0U;
}

bool isotp_fd_tx_idle(const isotp_fd_channel_t *ch) {
    return (ch->tx.state == ISOTP_TX_IDLE);
}
