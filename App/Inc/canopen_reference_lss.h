/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Project policy hooks around the CANopenNode CiA 305 LSS slave.
 */
#ifndef CANOPEN_REFERENCE_LSS_H
#define CANOPEN_REFERENCE_LSS_H

#include <stdbool.h>
#include <stdint.h>

#include "canopen_reference_co.h"

/* CO_t is declared by CANopen.h. */

/** Project policy state applied around the CANopenNode LSS slave. */
typedef struct {
    /** Current configured node-ID. */
    uint8_t node_id;
    /** Current configured nominal bitrate in kilobits per second. */
    uint16_t bitrate_kbps;
    /** Delay before applying an accepted bitrate change. */
    uint16_t activation_delay_ms;
    /** True after a store request has been accepted by policy. */
    bool store_requested;
} CANopenReferenceLssState;

/** Return whether the project policy supports the requested bitrate. */
bool CANopenReferenceLss_BitrateSupported(uint16_t bitrate_kbps);
/** Apply the configured bitrate after the bounded activation delay. */
void CANopenReferenceLss_ActivateBitrate(uint16_t delay_ms);
/** Validate and record a persistent node-ID/bitrate request. */
bool CANopenReferenceLss_StoreConfiguration(uint8_t node_id, uint16_t bitrate_kbps);
/** Initialize the project LSS policy around the CANopenNode instance. */
void CANopenReferenceLss_Init(CO_t *co, CANopenReferenceLssState *state);

#endif /* CANOPEN_REFERENCE_LSS_H */
