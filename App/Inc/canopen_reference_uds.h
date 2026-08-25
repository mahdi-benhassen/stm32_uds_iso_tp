/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#ifndef CANOPEN_REFERENCE_UDS_H
#define CANOPEN_REFERENCE_UDS_H

#include <stdbool.h>
#include <stdint.h>

#include "canopen_reference_config.h"
#include "uds_stm32.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (CANOPEN_REFERENCE_ENABLE_UDS != 0U)
int CANopenReference_UDS_Init(CAN_HandleTypeDef *hcan, uint32_t now_ms);

/* Called by the single board-owned CAN RX callback after HAL_CAN_GetRxMessage.
 * It is ISR-safe and does not invoke UDS or application callbacks. */
void CANopenReference_UDS_RxFromIsr(uint32_t id, const uint8_t *data, uint8_t dlc);

/* Called from the existing mainline loop, never from an interrupt. */
void CANopenReference_UDS_Process(uint32_t now_ms);

bool CANopenReference_UDS_ResetPending(void);
void CANopenReference_UDS_ClearReset(void);
void CANopenReference_UDS_GetStats(UdsStm32CanStats *stats);
#endif

#ifdef __cplusplus
}
#endif

#endif
