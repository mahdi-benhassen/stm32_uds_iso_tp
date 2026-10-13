/**
 * @file isotp_fd.h
 * @brief ISO 15765-2:2016 ISO-TP Transport Layer with 64-byte CAN-FD Support
 */

#ifndef ISOTP_FD_H
#define ISOTP_FD_H

#include <stdint.h>
#include <stdbool.h>
#include "can_frame_fd.h"

#ifndef ISOTP_FD_RX_BUF_SIZE
#define ISOTP_FD_RX_BUF_SIZE 4096U
#endif

#ifndef ISOTP_FD_TX_BUF_SIZE
#define ISOTP_FD_TX_BUF_SIZE 4096U
#endif

#define ISOTP_FD_BS          8U
#define ISOTP_FD_STMIN_MS    0U
#define ISOTP_FD_TIMEOUT_MS  1000U
#define ISOTP_FD_WFT_MAX     16U

typedef bool (*isotp_fd_tx_fn)(const can_frame_t *frame, void *user);

typedef enum {
    ISOTP_PCI_SF = 0,   /* Single Frame */
    ISOTP_PCI_FF = 1,   /* First Frame */
    ISOTP_PCI_CF = 2,   /* Consecutive Frame */
    ISOTP_PCI_FC = 3    /* Flow Control */
} isotp_pci_type_t;

typedef enum {
    ISOTP_FC_CTS   = 0,
    ISOTP_FC_WAIT  = 1,
    ISOTP_FC_OVFLW = 2
} isotp_fc_status_t;

typedef enum {
    ISOTP_RX_IDLE,
    ISOTP_RX_WAIT_CF
} isotp_rx_state_t;

typedef enum {
    ISOTP_TX_IDLE,
    ISOTP_TX_WAIT_FC,
    ISOTP_TX_SEND_CF
} isotp_tx_state_t;

typedef struct {
    isotp_rx_state_t state;
    uint8_t  buf[ISOTP_FD_RX_BUF_SIZE];
    uint32_t msg_len;
    uint32_t offset;
    uint8_t  seq;
    uint8_t  bs;
    uint8_t  bs_count;
    uint32_t timer_ms;
} isotp_fd_rx_ctx_t;

typedef struct {
    isotp_tx_state_t state;
    uint8_t  buf[ISOTP_FD_TX_BUF_SIZE];
    uint32_t msg_len;
    uint32_t offset;
    uint8_t  seq;
    uint8_t  bs;
    uint8_t  bs_count;
    uint8_t  stmin;
    uint32_t timer_ms;
    uint8_t  wft_count;
} isotp_fd_tx_ctx_t;

/* 29-bit Extended Addressing Macros (ISO 15765-2 / SAE J1939, Issue #123) */
#define ISOTP_IS_EXT_ID(id)                 ((id) > 0x7FFU)
#define ISOTP_CAN_ID_MASK_STD               0x7FFU
#define ISOTP_CAN_ID_MASK_EXT               0x1FFFFFFFU

#define ISOTP_29BIT_PHYS_REQ(ecu, tester)   (0x18DA0000U | ((uint32_t)(ecu) << 8U) | (uint32_t)(tester))
#define ISOTP_29BIT_PHYS_RESP(tester, ecu)  (0x18DA0000U | ((uint32_t)(tester) << 8U) | (uint32_t)(ecu))
#define ISOTP_29BIT_FUNC_REQ(group, tester) (0x18DB0000U | ((uint32_t)(group) << 8U) | (uint32_t)(tester))

typedef struct {
    isotp_fd_rx_ctx_t rx;
    isotp_fd_tx_ctx_t tx;
    uint32_t rx_id;
    uint32_t tx_id;
    bool     rx_is_ext;     /* True if 29-bit extended RX identifier */
    bool     tx_is_ext;     /* True if 29-bit extended TX identifier */
    bool     use_fd;        /* True to enable 64-byte CAN-FD transmission */
    isotp_fd_tx_fn tx_cb;
    void    *tx_user;
} isotp_fd_channel_t;

typedef enum {
    ISOTP_OK,
    ISOTP_BUSY,
    ISOTP_ERROR
} isotp_result_t;

void isotp_fd_init(isotp_fd_channel_t *ch, uint32_t rx_id, uint32_t tx_id,
                   bool use_fd, isotp_fd_tx_fn tx_cb, void *tx_user);

void isotp_fd_init_ext(isotp_fd_channel_t *ch, uint32_t rx_id, uint32_t tx_id,
                       bool rx_is_ext, bool tx_is_ext,
                       bool use_fd, isotp_fd_tx_fn tx_cb, void *tx_user);

void isotp_fd_on_rx(isotp_fd_channel_t *ch, const can_frame_t *frame, uint32_t now_ms);

void isotp_fd_poll(isotp_fd_channel_t *ch, uint32_t now_ms);

isotp_result_t isotp_fd_send(isotp_fd_channel_t *ch, const uint8_t *data, uint32_t len, uint32_t now_ms);

bool isotp_fd_rx_ready(const isotp_fd_channel_t *ch);

const uint8_t *isotp_fd_rx_data(const isotp_fd_channel_t *ch, uint32_t *len);

void isotp_fd_rx_done(isotp_fd_channel_t *ch);

bool isotp_fd_tx_idle(const isotp_fd_channel_t *ch);

#endif /* ISOTP_FD_H */
