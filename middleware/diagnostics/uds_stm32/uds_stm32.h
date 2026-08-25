/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#ifndef STM32_CANOPEN_UDS_STM32_H
#define STM32_CANOPEN_UDS_STM32_H

#include <stdbool.h>
#include <stdint.h>

#include "isotp.h"
#include "stm32f7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UDS_STM32_RX_QUEUE_CAPACITY
#define UDS_STM32_RX_QUEUE_CAPACITY 8U
#endif

#ifndef UDS_STM32_TX_QUEUE_CAPACITY
#define UDS_STM32_TX_QUEUE_CAPACITY 8U
#endif

#ifndef UDS_STM32_RX_BUDGET_PER_CALL
#define UDS_STM32_RX_BUDGET_PER_CALL 4U
#endif

#ifndef UDS_STM32_TX_BUDGET_PER_CALL
#define UDS_STM32_TX_BUDGET_PER_CALL 4U
#endif

#ifndef UDS_RX_CAN_ID
#define UDS_RX_CAN_ID 0x7E0U
#endif

#ifndef UDS_TX_CAN_ID
#define UDS_TX_CAN_ID 0x7E8U
#endif

typedef struct {
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t rx_overflow;
    uint32_t tx_overflow;
    uint32_t tx_mailbox_exhausted;
    uint32_t rx_invalid;
    uint32_t rx_timeouts;
    uint32_t tx_timeouts;
    uint32_t bus_errors;
    uint32_t bus_off_events;
} UdsStm32CanStats;

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[ISOTP_MAX_FRAME_DATA];
} UdsStm32CanQueuedFrame;

typedef struct {
    CAN_HandleTypeDef *hcan;
    UdsStm32CanQueuedFrame rx_queue[UDS_STM32_RX_QUEUE_CAPACITY];
    UdsStm32CanQueuedFrame tx_queue[UDS_STM32_TX_QUEUE_CAPACITY];
    uint32_t rx_head;
    uint32_t rx_tail;
    uint32_t tx_head;
    uint32_t tx_tail;
    UdsStm32CanStats stats;
    uint32_t request_id;
    uint32_t response_id;
    bool bound;
} UdsStm32Can;

int uds_stm32_can_bind(UdsStm32Can *adapter, CAN_HandleTypeDef *hcan, uint32_t request_id,
                       uint32_t response_id);
/* Attach to the board-owned HAL RX callback registration. This does not start,
 * stop, configure, or reconfigure the CAN controller. */
int uds_stm32_can_attach(UdsStm32Can *adapter);
void uds_stm32_can_detach(UdsStm32Can *adapter);
void uds_stm32_can_reset(UdsStm32Can *adapter);

/* ISR-safe receive handoff. It only validates, copies, counts, and publishes. */
void uds_stm32_can_rx_from_isr(UdsStm32Can *adapter, uint32_t id, const uint8_t *data, uint8_t dlc);

/* Mainline-only queue consumers/producers. */
int uds_stm32_can_rx_pop(UdsStm32Can *adapter, IsoTpCanFrame *frame);
int uds_stm32_can_tx_queue(UdsStm32Can *adapter, const IsoTpCanFrame *frame);
int uds_stm32_can_process_tx(UdsStm32Can *adapter, uint32_t budget);

/* These are intended for the existing HAL/CANopen error and timeout paths. */
void uds_stm32_can_note_rx_timeout(UdsStm32Can *adapter);
void uds_stm32_can_note_tx_timeout(UdsStm32Can *adapter);
void uds_stm32_can_note_error(UdsStm32Can *adapter, uint32_t hal_error, bool bus_off);
void uds_stm32_can_get_stats(const UdsStm32Can *adapter, UdsStm32CanStats *stats);

/* The existing CANopen filter owner calls this while constructing its bounded
 * standard-ID list. These IDs are configurable and are not hard-coded in the
 * filter implementation. */
bool uds_stm32_can_filter_match(const UdsStm32Can *adapter, uint32_t id);
uint32_t uds_stm32_can_request_id(const UdsStm32Can *adapter);
uint32_t uds_stm32_can_response_id(const UdsStm32Can *adapter);

#ifdef __cplusplus
}
#endif

#endif
