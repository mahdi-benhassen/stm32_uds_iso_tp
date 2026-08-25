/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Bounded CiA 302 NMT-master foundation for the reference project.
 * This module is transport-neutral: the application supplies frame/event
 * callbacks and remains the owner of the production bxCAN driver.
 */
#ifndef CIA302_NMT_MASTER_H
#define CIA302_NMT_MASTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Maximum valid CANopen node-ID supported by the bounded master. */
#define CIA302_MAX_NODES 127U
/** CAN-ID used for NMT command frames. */
#define CIA302_NMT_CAN_ID 0x000U
/** Base CAN-ID for heartbeat frames; add the node-ID. */
#define CIA302_HEARTBEAT_BASE 0x700U

#define CIA302_NMT_START 0x01U
#define CIA302_NMT_STOP 0x02U
#define CIA302_NMT_PREOP 0x80U
#define CIA302_NMT_RESET_NODE 0x81U
#define CIA302_NMT_RESET_COMMUNICATION 0x82U

#define CIA302_HEARTBEAT_BOOTUP 0x00U
#define CIA302_HEARTBEAT_STOPPED 0x04U
#define CIA302_HEARTBEAT_OPERATIONAL 0x05U
#define CIA302_HEARTBEAT_PREOP 0x7FU

/** Tracked state of an assigned remote node. */
typedef enum {
    CIA302_NODE_UNASSIGNED = 0,
    CIA302_NODE_WAITING_BOOTUP,
    CIA302_NODE_PREOP,
    CIA302_NODE_OPERATIONAL,
    CIA302_NODE_STOPPED,
    CIA302_NODE_TIMED_OUT,
} cia302_node_state_t;

/** Event types emitted by the bounded NMT master. */
typedef enum {
    CIA302_EVENT_BOOTUP = 1,
    CIA302_EVENT_HEARTBEAT,
    CIA302_EVENT_HEARTBEAT_TIMEOUT,
    CIA302_EVENT_BOOT_TIMEOUT,
    CIA302_EVENT_NETWORK_READY,
    CIA302_EVENT_INVALID_FRAME,
} cia302_event_type_t;

/** Transport-neutral classic-CAN frame passed to the send callback. */
typedef struct {
    /** Standard 11-bit CAN identifier. */
    uint16_t can_id;
    /** Number of valid bytes in data, from 0 through 8. */
    uint8_t dlc;
    /** Classic-CAN payload; only the first dlc bytes are valid. */
    uint8_t data[8];
} cia302_frame_t;

/** Bounded event emitted from mainline processing. */
typedef struct {
    /** Event classification. */
    cia302_event_type_t type;
    /** Remote node associated with the event, or zero for network events. */
    uint8_t node_id;
    /** Raw heartbeat state byte when the event represents a received frame. */
    uint8_t state;
    /** Monotonic mainline timestamp at event creation. */
    uint32_t timestamp_ms;
} cia302_event_t;

/** Send one NMT frame; return false when the transport cannot queue it. */
typedef bool (*cia302_send_fn)(void *context, const cia302_frame_t *frame);
/** Receive one bounded event; the callback must not block. */
typedef void (*cia302_event_fn)(void *context, const cia302_event_t *event);

/** Assignment and supervision state for one remote CANopen node. */
typedef struct {
    /** True when this node-ID has an active master assignment. */
    bool assigned;
    /** True when boot and heartbeat supervision is required for readiness. */
    bool mandatory;
    /** True when the master requests operational state after readiness. */
    bool auto_start;
    /** Consumer timeout applied to this node’s heartbeat. */
    uint16_t heartbeat_timeout_ms;
    /** Timestamp of the most recently accepted heartbeat. */
    uint32_t last_heartbeat_ms;
    /** Last validated state tracked for this node. */
    cia302_node_state_t state;
} cia302_node_t;

/** Complete bounded runtime state of the CiA 302 NMT master. */
typedef struct {
    /** Startup policy bits corresponding to the product’s NMT startup object. */
    uint32_t nmt_startup;
    /** Maximum time allowed for mandatory nodes to boot. */
    uint32_t boot_time_ms;
    /** Local master node-ID used by the application policy. */
    uint8_t master_node_id;
    /** Current monotonic processing timestamp. */
    uint32_t now_ms;
    /** Timestamp at which supervision started. */
    uint32_t started_at_ms;
    /** True after cia302_nmt_master_start succeeds. */
    bool running;
    /** True after all mandatory assignments have reached readiness. */
    bool network_ready;
    /** Prevents repeated boot-timeout events during one startup attempt. */
    bool boot_timeout_reported;
    /** Node assignments indexed by CANopen node-ID. */
    cia302_node_t nodes[CIA302_MAX_NODES + 1U];
    /** Transport callback used for NMT command transmission. */
    cia302_send_fn send;
    /** Non-blocking event callback used for diagnostic observation. */
    cia302_event_fn event;
    /** Opaque context passed to send and event callbacks. */
    void *callback_context;
} cia302_nmt_master_t;

/** Initialize a zeroed master and bind non-blocking transport callbacks. */
void cia302_nmt_master_init(cia302_nmt_master_t *master, uint8_t master_node_id,
                            cia302_send_fn send, cia302_event_fn event, void *callback_context);

/** Assign a node and configure mandatory, auto-start, and heartbeat policy. */
bool cia302_nmt_master_configure(cia302_nmt_master_t *master, uint8_t node_id, bool mandatory,
                                 bool auto_start, uint16_t heartbeat_timeout_ms);

/** Queue a targeted or broadcast NMT command; returns false for invalid input. */
bool cia302_nmt_master_request(cia302_nmt_master_t *master, uint8_t command, uint8_t node_id);

/** Start supervision at the supplied monotonic millisecond timestamp. */
bool cia302_nmt_master_start(cia302_nmt_master_t *master, uint32_t now_ms);
/** Validate and consume one received heartbeat in mainline context. */
void cia302_nmt_master_receive(cia302_nmt_master_t *master, uint16_t can_id, const uint8_t *data,
                               uint8_t dlc, uint32_t now_ms);
/** Advance boot, heartbeat, readiness, and auto-start supervision. */
void cia302_nmt_master_process(cia302_nmt_master_t *master, uint32_t now_ms);

#endif /* CIA302_NMT_MASTER_H */
