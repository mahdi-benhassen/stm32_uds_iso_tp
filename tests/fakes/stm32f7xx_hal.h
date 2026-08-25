/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef TEST_FAKE_STM32F7XX_HAL_H
#define TEST_FAKE_STM32F7XX_HAL_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    struct {
        uint32_t Prescaler;
        uint32_t TimeSeg1;
        uint32_t TimeSeg2;
    } Init;
    uint32_t unused;
} CAN_HandleTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t TransmitGlobalTime;
} CAN_TxHeaderTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
} CAN_RxHeaderTypeDef;

typedef void (*HAL_CAN_RxCallbackTypeDef)(CAN_HandleTypeDef *hcan);

typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1
} HAL_StatusTypeDef;

#define CAN_IT_RX_FIFO0_MSG_PENDING (1U << 0)
#define CAN_IT_RX_FIFO1_MSG_PENDING (1U << 1)
#define CAN_IT_TX_MAILBOX_EMPTY (1U << 2)
#define CAN_IT_ERROR (1U << 3)
#define CAN_RX_FIFO0 0U
#define CAN_RX_FIFO1 1U
#define HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID 0U
#define HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID 1U
#define CAN_ID_STD 0U
#define CAN_RTR_DATA 0U
#define DISABLE 0U
#define CAN_BS1_12TQ 12U
#define CAN_BS1_13TQ 13U
#define CAN_BS1_14TQ 14U
#define CAN_BS1_16TQ 16U
#define CAN_BS2_3TQ 3U
#define CAN_BS2_8TQ 8U
static uint32_t s_fake_tick;
static inline uint32_t HAL_GetTick(void) { return s_fake_tick; }
static inline void HAL_Delay(uint32_t delay_ms) { s_fake_tick += delay_ms; }
static inline HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef *hcan) { (void)hcan; return HAL_OK; }

static inline HAL_StatusTypeDef
HAL_CAN_Start(CAN_HandleTypeDef *hcan) {
    (void)hcan;
    return HAL_OK;
}

static inline HAL_StatusTypeDef
HAL_CAN_Stop(CAN_HandleTypeDef *hcan) {
    (void)hcan;
    return HAL_OK;
}

static inline HAL_StatusTypeDef
HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t notifications) {
    (void)hcan;
    (void)notifications;
    return HAL_OK;
}

static inline HAL_StatusTypeDef
HAL_CAN_DeactivateNotification(CAN_HandleTypeDef *hcan, uint32_t notifications) {
    (void)hcan;
    (void)notifications;
    return HAL_OK;
}

static inline uint32_t
HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef *hcan, uint32_t fifo) {
    (void)hcan;
    (void)fifo;
    return 0U;
}

static inline HAL_StatusTypeDef
HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t fifo,
                    CAN_RxHeaderTypeDef *header, uint8_t *data) {
    (void)hcan;
    (void)fifo;
    (void)header;
    (void)data;
    return HAL_ERROR;
}

static inline HAL_StatusTypeDef
HAL_CAN_RegisterCallback(CAN_HandleTypeDef *hcan, uint32_t callback_id,
                         HAL_CAN_RxCallbackTypeDef callback) {
    (void)hcan;
    (void)callback_id;
    (void)callback;
    return HAL_OK;
}

static inline HAL_StatusTypeDef
HAL_CAN_UnRegisterCallback(CAN_HandleTypeDef *hcan, uint32_t callback_id) {
    (void)hcan;
    (void)callback_id;
    return HAL_OK;
}

static inline uint32_t
HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *hcan) {
    (void)hcan;
    return 1U;
}

static inline HAL_StatusTypeDef
HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan,
                     CAN_TxHeaderTypeDef *header,
                     uint8_t *data,
                     uint32_t *mailbox) {
    (void)hcan;
    (void)header;
    (void)data;
    if (mailbox != NULL) {
        *mailbox = 0U;
    }
    return HAL_OK;
}

#endif /* TEST_FAKE_STM32F7XX_HAL_H */
