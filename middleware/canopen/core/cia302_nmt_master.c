/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "cia302_nmt_master.h"

#include <string.h>

static bool node_id_valid(uint8_t node_id) {
    return node_id >= 1U && node_id <= CIA302_MAX_NODES;
}

static bool command_valid(uint8_t command) {
    return command == CIA302_NMT_START || command == CIA302_NMT_STOP ||
           command == CIA302_NMT_PREOP || command == CIA302_NMT_RESET_NODE ||
           command == CIA302_NMT_RESET_COMMUNICATION;
}

static void emit_event(cia302_nmt_master_t *master, cia302_event_type_t type, uint8_t node_id,
                       uint8_t state) {
    if (master->event != NULL) {
        cia302_event_t event = {
            .type = type,
            .node_id = node_id,
            .state = state,
            .timestamp_ms = master->now_ms,
        };
        master->event(master->callback_context, &event);
    }
}

void cia302_nmt_master_init(cia302_nmt_master_t *master, uint8_t master_node_id,
                            cia302_send_fn send, cia302_event_fn event, void *callback_context) {
    (void)memset(master, 0, sizeof(*master));
    master->master_node_id = master_node_id;
    master->send = send;
    master->event = event;
    master->callback_context = callback_context;
    master->boot_time_ms = 10000U;
}

bool cia302_nmt_master_configure(cia302_nmt_master_t *master, uint8_t node_id, bool mandatory,
                                 bool auto_start, uint16_t heartbeat_timeout_ms) {
    if (master == NULL || !node_id_valid(node_id) || node_id == master->master_node_id) {
        return false;
    }
    master->nodes[node_id].assigned = true;
    master->nodes[node_id].mandatory = mandatory;
    master->nodes[node_id].auto_start = auto_start;
    master->nodes[node_id].heartbeat_timeout_ms = heartbeat_timeout_ms;
    master->nodes[node_id].state = CIA302_NODE_WAITING_BOOTUP;
    return true;
}

bool cia302_nmt_master_request(cia302_nmt_master_t *master, uint8_t command, uint8_t node_id) {
    if (master == NULL || master->send == NULL || !command_valid(command) ||
        node_id > CIA302_MAX_NODES) {
        return false;
    }
    cia302_frame_t frame = {
        .can_id = CIA302_NMT_CAN_ID,
        .dlc = 2U,
        .data = {command, node_id, 0U, 0U, 0U, 0U, 0U, 0U},
    };
    return master->send(master->callback_context, &frame);
}

bool cia302_nmt_master_start(cia302_nmt_master_t *master, uint32_t now_ms) {
    if (master == NULL || master->send == NULL || (master->nmt_startup & 0x01U) == 0U) {
        return false;
    }
    master->now_ms = now_ms;
    master->started_at_ms = now_ms;
    master->running = true;
    master->network_ready = false;
    master->boot_timeout_reported = false;
    for (uint8_t node_id = 1U; node_id <= CIA302_MAX_NODES; ++node_id) {
        if (master->nodes[node_id].assigned) {
            master->nodes[node_id].state = CIA302_NODE_WAITING_BOOTUP;
            master->nodes[node_id].last_heartbeat_ms = now_ms;
        }
    }
    return true;
}

void cia302_nmt_master_receive(cia302_nmt_master_t *master, uint16_t can_id, const uint8_t *data,
                               uint8_t dlc, uint32_t now_ms) {
    if (master == NULL || data == NULL || !master->running) {
        return;
    }
    master->now_ms = now_ms;
    if (dlc != 1U || can_id < CIA302_HEARTBEAT_BASE ||
        can_id > CIA302_HEARTBEAT_BASE + CIA302_MAX_NODES) {
        emit_event(master, CIA302_EVENT_INVALID_FRAME, 0U, 0U);
        return;
    }
    uint8_t node_id = (uint8_t)(can_id - CIA302_HEARTBEAT_BASE);
    if (!master->nodes[node_id].assigned) {
        return;
    }
    uint8_t state = data[0];
    master->nodes[node_id].last_heartbeat_ms = now_ms;
    if (state == CIA302_HEARTBEAT_BOOTUP) {
        master->nodes[node_id].state = CIA302_NODE_PREOP;
        emit_event(master, CIA302_EVENT_BOOTUP, node_id, state);
    } else if (state == CIA302_HEARTBEAT_PREOP) {
        master->nodes[node_id].state = CIA302_NODE_PREOP;
        emit_event(master, CIA302_EVENT_HEARTBEAT, node_id, state);
    } else if (state == CIA302_HEARTBEAT_OPERATIONAL) {
        master->nodes[node_id].state = CIA302_NODE_OPERATIONAL;
        emit_event(master, CIA302_EVENT_HEARTBEAT, node_id, state);
    } else if (state == CIA302_HEARTBEAT_STOPPED) {
        master->nodes[node_id].state = CIA302_NODE_STOPPED;
        emit_event(master, CIA302_EVENT_HEARTBEAT, node_id, state);
    } else {
        emit_event(master, CIA302_EVENT_INVALID_FRAME, node_id, state);
    }
}

static bool mandatory_nodes_ready(const cia302_nmt_master_t *master) {
    for (uint8_t node_id = 1U; node_id <= CIA302_MAX_NODES; ++node_id) {
        const cia302_node_t *node = &master->nodes[node_id];
        if (node->assigned && node->mandatory &&
            (node->state == CIA302_NODE_WAITING_BOOTUP || node->state == CIA302_NODE_TIMED_OUT)) {
            return false;
        }
    }
    return true;
}

static bool any_assigned(const cia302_nmt_master_t *master) {
    for (uint8_t node_id = 1U; node_id <= CIA302_MAX_NODES; ++node_id) {
        if (master->nodes[node_id].assigned) {
            return true;
        }
    }
    return false;
}

void cia302_nmt_master_process(cia302_nmt_master_t *master, uint32_t now_ms) {
    if (master == NULL || !master->running) {
        return;
    }
    master->now_ms = now_ms;
    for (uint8_t node_id = 1U; node_id <= CIA302_MAX_NODES; ++node_id) {
        cia302_node_t *node = &master->nodes[node_id];
        if (!node->assigned) {
            continue;
        }
        if (node->heartbeat_timeout_ms != 0U &&
            (uint32_t)(now_ms - node->last_heartbeat_ms) > node->heartbeat_timeout_ms &&
            node->state != CIA302_NODE_TIMED_OUT) {
            node->state = CIA302_NODE_TIMED_OUT;
            emit_event(master, CIA302_EVENT_HEARTBEAT_TIMEOUT, node_id, 0U);
        }
    }

    bool ready = mandatory_nodes_ready(master);
    if (!ready && master->boot_time_ms != 0U && !master->boot_timeout_reported &&
        (uint32_t)(now_ms - master->started_at_ms) > master->boot_time_ms) {
        for (uint8_t node_id = 1U; node_id <= CIA302_MAX_NODES; ++node_id) {
            const cia302_node_t *node = &master->nodes[node_id];
            if (node->assigned && node->mandatory &&
                (node->state == CIA302_NODE_WAITING_BOOTUP ||
                 node->state == CIA302_NODE_TIMED_OUT)) {
                master->boot_timeout_reported = true;
                emit_event(master, CIA302_EVENT_BOOT_TIMEOUT, node_id, 0U);
                break;
            }
        }
        return;
    }
    if (!master->network_ready && any_assigned(master) && ready) {
        master->network_ready = true;
        emit_event(master, CIA302_EVENT_NETWORK_READY, 0U, 0U);
        if ((master->nmt_startup & (1UL << 3U)) == 0U) {
            for (uint8_t node_id = 1U; node_id <= CIA302_MAX_NODES; ++node_id) {
                if (master->nodes[node_id].assigned && master->nodes[node_id].auto_start) {
                    (void)cia302_nmt_master_request(master, CIA302_NMT_START, node_id);
                }
            }
        }
    }
}
