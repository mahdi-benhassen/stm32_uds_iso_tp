/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Project-owned opt-in CiA 302 NMT-master adapter.
 */
#ifndef CANOPEN_REFERENCE_CIA302_H
#define CANOPEN_REFERENCE_CIA302_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Stable, bounded diagnostic view of the opt-in CiA 302 adapter. */
typedef struct {
    /** True when the compile-time CiA 302 personality is enabled. */
    bool enabled;
    /** True after the adapter has entered active supervision. */
    bool running;
    /** True after all configured mandatory nodes are ready. */
    bool network_ready;
    /** Node-ID currently selected for the primary diagnostic view. */
    uint8_t monitored_node_id;
    /** Last validated heartbeat state for the monitored node. */
    uint8_t monitored_node_state;
    /** Number of accepted boot-up events. */
    uint32_t event_count_bootup;
    /** Number of accepted heartbeat events. */
    uint32_t event_count_heartbeat;
    /** Number of heartbeat-timeout events. */
    uint32_t event_count_heartbeat_timeout;
    /** Number of mandatory-node boot-timeout events. */
    uint32_t event_count_boot_timeout;
    /** Number of network-ready events. */
    uint32_t event_count_network_ready;
    /** Number of rejected or malformed heartbeat events. */
    uint32_t event_count_invalid_frame;
    /** Timestamp of the most recent event. */
    uint32_t last_event_timestamp_ms;
    /** Type of the most recent event. */
    uint8_t last_event_type;
    /** Node-ID associated with the most recent event. */
    uint8_t last_event_node_id;
    /** Raw state byte associated with the most recent event. */
    uint8_t last_event_state;
} CANopenReferenceCia302Snapshot;

/** Prepare the optional heartbeat-consumer OD entries before CANopen init. */
void CANopenReferenceCia302_PrepareOd(void);

/** Initialize the opt-in CiA 302 master adapter after CANopen init. */
void CANopenReferenceCia302_Init(void *canopen_stack, uint8_t master_node_id, uint32_t now_ms);

/** Stop and clear the opt-in adapter during communication reset. */
void CANopenReferenceCia302_Deinit(void);

/** Feed each received heartbeat to the master before CANopenNode clears its RX flag. */
void CANopenReferenceCia302_PreProcess(uint32_t now_ms);

/** Process the non-blocking master state machine from mainline context. */
void CANopenReferenceCia302_Process(uint32_t now_ms);

/** Return a stable copy of the bounded diagnostic state. */
void CANopenReferenceCia302_GetSnapshot(CANopenReferenceCia302Snapshot *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_REFERENCE_CIA302_H */
