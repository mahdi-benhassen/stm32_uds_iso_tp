/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Deliberate, documented fix-ups applied to the CubeMX-generated peripheral
 * configuration before the CANopen runtime starts. This module exists so the
 * generated code base remains byte-for-byte regenerable while protocol-level
 * requirements that CubeMX cannot express are still enforced.
 */
#ifndef CANOPEN_REFERENCE_PORT_FIXUP_H
#define CANOPEN_REFERENCE_PORT_FIXUP_H

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Apply CANopen-required bxCAN settings to the generated CAN1 handle.
 *
 * - Enables automatic retransmission (CiA 301 mandates it; the generated
 *   configuration disables it, which would silently drop frames on error).
 * - Moves the sample point from the generated 12/18 tq (66.7 %) to 15/18 tq
 *   (83.3 %) at the same 500 kbit/s, matching the reference timing table and
 *   typical CiA network expectations.
 * - Keeps AutoBusOff disabled on purpose: bus-off recovery is owned by the
 *   bounded software recovery state machine.
 *
 * Re-initializes CAN1 through HAL (MspDeInit/MspInit run again), so this must
 * be called before acceptance filters, notifications, or canopen_app_init().
 * Also demotes the TIM7 interrupt from the generated priority (0,0) — equal
 * to CAN RX — to (1,0), so CAN reception always preempts the 1 ms dispatch.
 */
void CanopenReferencePortFixup_Prepare(CAN_HandleTypeDef *hcan);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_REFERENCE_PORT_FIXUP_H */
