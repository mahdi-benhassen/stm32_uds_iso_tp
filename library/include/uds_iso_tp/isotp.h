#ifndef STM32_UDS_ISO_TP_ISOTP_H
#define STM32_UDS_ISO_TP_ISOTP_H

#include <stdbool.h>
#include <stdint.h>

#ifndef ISOTP_MAX_PAYLOAD
#define ISOTP_MAX_PAYLOAD 16384U
#endif
#ifndef ISOTP_MAX_FRAME_DATA
#define ISOTP_MAX_FRAME_DATA 64U
#endif
#ifndef ISOTP_DEFAULT_RX_TIMEOUT_MS
#define ISOTP_DEFAULT_RX_TIMEOUT_MS 1000U
#endif
#ifndef ISOTP_DEFAULT_TX_TIMEOUT_MS
#define ISOTP_DEFAULT_TX_TIMEOUT_MS 1000U
#endif
#ifndef ISOTP_DEFAULT_MAX_WAIT_FRAMES
#define ISOTP_DEFAULT_MAX_WAIT_FRAMES 3U
#endif

typedef struct {
    uint32_t can_id;
    uint8_t dlc;
    bool is_fd;
    bool bit_rate_switch;
    uint8_t data[ISOTP_MAX_FRAME_DATA];
} IsoTpCanFrame;

typedef enum {
    ISOTP_OK = 0,
    ISOTP_COMPLETE,
    ISOTP_NEED_FLOW_CONTROL,
    ISOTP_TX_FRAME_READY,
    ISOTP_ERR_ARGUMENT,
    ISOTP_ERR_FORMAT,
    ISOTP_ERR_SEQUENCE,
    ISOTP_ERR_OVERFLOW,
    ISOTP_ERR_TIMEOUT,
    ISOTP_ERR_STATE,
    ISOTP_ERR_FLOW_CONTROL
} IsoTpStatus;

typedef enum {
    ISOTP_FC_CTS = 0x00U,
    ISOTP_FC_WAIT = 0x01U,
    ISOTP_FC_OVERFLOW = 0x02U
} IsoTpFlowStatus;

typedef struct {
    uint8_t block_size;
    uint8_t st_min;
    uint32_t rx_timeout_ms;
    uint32_t tx_timeout_ms;
    uint8_t max_wait_frames;
    uint8_t tx_dl;
    uint8_t rx_dl;
    bool can_fd;
    bool bit_rate_switch;
} IsoTpConfig;

typedef struct {
    IsoTpConfig config;
    uint32_t request_id;
    uint32_t response_id;
    uint8_t buffer[ISOTP_MAX_PAYLOAD];
    uint32_t expected_len;
    uint32_t received_len;
    uint8_t next_sequence;
    uint8_t block_count;
    uint32_t deadline_ms;
    bool active;
} IsoTpRx;

typedef struct {
    IsoTpConfig config;
    uint32_t request_id;
    uint32_t response_id;
    uint8_t buffer[ISOTP_MAX_PAYLOAD];
    uint32_t payload_len;
    uint32_t offset;
    uint8_t next_sequence;
    uint8_t block_count;
    uint8_t remote_block_size;
    uint8_t remote_st_min;
    uint32_t deadline_ms;
    uint32_t next_frame_ms;
    bool active;
    bool waiting_flow_control;
    uint8_t wait_frames;
} IsoTpTx;

typedef struct {
    const uint8_t *payload;
    uint32_t length;
    bool has_flow_control;
    IsoTpCanFrame flow_control;
} IsoTpRxEvent;

void isotp_config_default(IsoTpConfig *config);
void isotp_config_classic_can(IsoTpConfig *config);
void isotp_config_can_fd(IsoTpConfig *config, uint8_t tx_dl, uint8_t rx_dl);
void isotp_rx_init(IsoTpRx *rx, const IsoTpConfig *config, uint32_t request_id,
                   uint32_t response_id);
IsoTpStatus isotp_rx_feed(IsoTpRx *rx, const IsoTpCanFrame *frame, uint32_t now_ms,
                          IsoTpRxEvent *event);
IsoTpStatus isotp_rx_tick(IsoTpRx *rx, uint32_t now_ms);
void isotp_rx_reset(IsoTpRx *rx);
void isotp_tx_init(IsoTpTx *tx, const IsoTpConfig *config, uint32_t request_id,
                   uint32_t response_id);
IsoTpStatus isotp_tx_start(IsoTpTx *tx, const uint8_t *payload, uint32_t length, uint32_t now_ms,
                           IsoTpCanFrame *frame);
IsoTpStatus isotp_tx_feed_flow_control(IsoTpTx *tx, const IsoTpCanFrame *frame, uint32_t now_ms);
IsoTpStatus isotp_tx_next(IsoTpTx *tx, uint32_t now_ms, IsoTpCanFrame *frame);
IsoTpStatus isotp_tx_tick(IsoTpTx *tx, uint32_t now_ms);
void isotp_tx_reset(IsoTpTx *tx);

#endif
