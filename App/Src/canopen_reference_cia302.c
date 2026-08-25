/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Project-owned opt-in CiA 302 NMT-master adapter. The adapter deliberately
 * reuses CANopenNode's NMT master transmit path and heartbeat consumer instead
 * of taking ownership of the STM32 HAL callbacks.
 *
 * Scope boundary: this module provides bounded configured-peer supervision only.
 * It does not implement the standard CiA 302 Network List/Configuration Manager
 * objects (0x1F80-0x1F89), commissioning workflow, or a complete multi-node
 * production network manager.
 */
#include "canopen_reference_cia302.h"

#include "canopen_reference_co.h"
#include "CO_HBconsumer.h"
#include "canopen_reference_od.h"
#include "canopen_reference_config.h"
#include "cia302_nmt_master.h"

#include <string.h>

static CANopenReferenceCia302Snapshot s_snapshot;

#if CANOPEN_REFERENCE_ENABLE_CIA302_MASTER

#define CIA302_REFERENCE_HB_INDEX 0U

static cia302_nmt_master_t s_master;
static CO_t *s_co;

static bool
cia302_reference_send(void *context, const cia302_frame_t *frame) {
    CO_NMT_command_t command;
    CO_t *co = (CO_t *)context;

    if (co == NULL || co->NMT == NULL || frame == NULL || frame->can_id != CIA302_NMT_CAN_ID ||
        frame->dlc != 2U || frame->data[1] > CIA302_MAX_NODES) {
        return false;
    }

    switch (frame->data[0]) {
        case CIA302_NMT_START:
            command = CO_NMT_ENTER_OPERATIONAL;
            break;
        case CIA302_NMT_STOP:
            command = CO_NMT_ENTER_STOPPED;
            break;
        case CIA302_NMT_PREOP:
            command = CO_NMT_ENTER_PRE_OPERATIONAL;
            break;
        case CIA302_NMT_RESET_NODE:
            command = CO_NMT_RESET_NODE;
            break;
        case CIA302_NMT_RESET_COMMUNICATION:
            command = CO_NMT_RESET_COMMUNICATION;
            break;
        default:
            return false;
    }

    return CO_NMT_sendCommand(co->NMT, command, frame->data[1]) == CO_ERROR_NO;
}

static void
cia302_reference_event(void *context, const cia302_event_t *event) {
    (void)context;
    if (event == NULL) {
        return;
    }

    s_snapshot.last_event_type = (uint8_t)event->type;
    s_snapshot.last_event_node_id = event->node_id;
    s_snapshot.last_event_state = event->state;
    s_snapshot.last_event_timestamp_ms = event->timestamp_ms;
    if (event->node_id == CANOPEN_REFERENCE_CIA302_PEER_NODE_ID) {
        s_snapshot.monitored_node_state = event->state;
    }

    switch (event->type) {
        case CIA302_EVENT_BOOTUP:
            s_snapshot.event_count_bootup++;
            break;
        case CIA302_EVENT_HEARTBEAT:
            s_snapshot.event_count_heartbeat++;
            break;
        case CIA302_EVENT_HEARTBEAT_TIMEOUT:
            s_snapshot.event_count_heartbeat_timeout++;
            break;
        case CIA302_EVENT_BOOT_TIMEOUT:
            s_snapshot.event_count_boot_timeout++;
            break;
        case CIA302_EVENT_NETWORK_READY:
            s_snapshot.event_count_network_ready++;
            s_snapshot.network_ready = true;
            break;
        case CIA302_EVENT_INVALID_FRAME:
            s_snapshot.event_count_invalid_frame++;
            break;
        default:
            break;
    }
}

static void
cia302_reference_timeout(uint8_t node_id, uint8_t idx, void *object) {
    (void)node_id;
    (void)idx;
    (void)object;
    /* The transport-neutral master owns timeout timing and emits exactly one
     * timeout event per loss episode from its regular process call. */
}

void
CANopenReferenceCia302_PrepareOd(void) {
    OD_PERSIST_COMM.x1016_consumerHeartbeatTime[CIA302_REFERENCE_HB_INDEX] =
        ((uint32_t)CANOPEN_REFERENCE_CIA302_PEER_NODE_ID << 16U) | CANOPEN_REFERENCE_CIA302_HEARTBEAT_TIMEOUT_MS;
}

void
CANopenReferenceCia302_Init(void *canopen_stack, uint8_t master_node_id, uint32_t now_ms) {
    s_co = (CO_t *)canopen_stack;
    (void)memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_snapshot.enabled = true;
    s_snapshot.monitored_node_id = CANOPEN_REFERENCE_CIA302_PEER_NODE_ID;

    cia302_nmt_master_init(&s_master, master_node_id, cia302_reference_send, cia302_reference_event, s_co);
    s_master.boot_time_ms = CANOPEN_REFERENCE_CIA302_BOOT_TIMEOUT_MS;
    s_master.nmt_startup = 0x01U;
    (void)cia302_nmt_master_configure(&s_master, CANOPEN_REFERENCE_CIA302_PEER_NODE_ID, true,
                                      CANOPEN_REFERENCE_CIA302_AUTO_START != 0U,
                                      CANOPEN_REFERENCE_CIA302_HEARTBEAT_TIMEOUT_MS);
    (void)cia302_nmt_master_start(&s_master, now_ms);
    s_snapshot.running = s_master.running;

#if ((CO_CONFIG_HB_CONS)&CO_CONFIG_HB_CONS_ENABLE) != 0
    if (s_co != NULL && s_co->HBcons != NULL) {
        int8_t idx = CO_HBconsumer_getIdxByNodeId(s_co->HBcons, CANOPEN_REFERENCE_CIA302_PEER_NODE_ID);
        if (idx >= 0) {
            CO_HBconsumer_initCallbackTimeout(s_co->HBcons, (uint8_t)idx, NULL, cia302_reference_timeout);
        }
    }
#endif
}

void
CANopenReferenceCia302_Deinit(void) {
    s_co = NULL;
    (void)memset(&s_master, 0, sizeof(s_master));
    (void)memset(&s_snapshot, 0, sizeof(s_snapshot));
}

void
CANopenReferenceCia302_PreProcess(uint32_t now_ms) {
#if ((CO_CONFIG_HB_CONS)&CO_CONFIG_HB_CONS_ENABLE) != 0
    if (s_snapshot.enabled && s_co != NULL && s_co->HBcons != NULL) {
        int8_t idx = CO_HBconsumer_getIdxByNodeId(s_co->HBcons, CANOPEN_REFERENCE_CIA302_PEER_NODE_ID);
        if (idx >= 0) {
            CO_HBconsNode_t *node = &s_co->HBcons->monitoredNodes[(uint8_t)idx];
            if (CO_FLAG_READ(node->CANrxNew)) {
                uint8_t heartbeat = (uint8_t)node->NMTstate;
                cia302_nmt_master_receive(&s_master, (uint16_t)(CIA302_HEARTBEAT_BASE + node->nodeId),
                                          &heartbeat, 1U, now_ms);
            }
        }
    }
#else
    (void)now_ms;
#endif
}

void
CANopenReferenceCia302_Process(uint32_t now_ms) {
    if (!s_snapshot.enabled) {
        return;
    }
    cia302_nmt_master_process(&s_master, now_ms);
    s_snapshot.running = s_master.running;
    s_snapshot.network_ready = s_master.network_ready;
}

#else

void CANopenReferenceCia302_PrepareOd(void) {}
void CANopenReferenceCia302_Init(void *canopen_stack, uint8_t master_node_id, uint32_t now_ms) {
    (void)canopen_stack;
    (void)master_node_id;
    (void)now_ms;
}
void CANopenReferenceCia302_Deinit(void) {}
void CANopenReferenceCia302_PreProcess(uint32_t now_ms) { (void)now_ms; }
void CANopenReferenceCia302_Process(uint32_t now_ms) { (void)now_ms; }

#endif

void
CANopenReferenceCia302_GetSnapshot(CANopenReferenceCia302Snapshot *snapshot) {
    if (snapshot != NULL) {
        *snapshot = s_snapshot;
    }
}
