/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Standard CANopen wire-contract constants used by host acceptance tests and
 * board integrations. This header does not implement a second CAN parser or
 * alter CANopenNode's protocol stack.
 */
#ifndef CANOPEN_REFERENCE_PROTOCOL_H
#define CANOPEN_REFERENCE_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define CANOPEN_REFERENCE_COBID_NMT                 0x000U
#define CANOPEN_REFERENCE_COBID_EMCY_BASE           0x080U
#define CANOPEN_REFERENCE_COBID_SDO_RESPONSE_BASE   0x580U
#define CANOPEN_REFERENCE_COBID_SDO_REQUEST_BASE    0x600U
#define CANOPEN_REFERENCE_COBID_HEARTBEAT_BASE      0x700U
#define CANOPEN_REFERENCE_COBID_TPDO1_BASE          0x180U
#define CANOPEN_REFERENCE_COBID_TPDO2_BASE          0x280U
#define CANOPEN_REFERENCE_COBID_TPDO3_BASE          0x380U
#define CANOPEN_REFERENCE_COBID_TPDO4_BASE          0x480U
#define CANOPEN_REFERENCE_COBID_TPDO5_BASE          0x190U
#define CANOPEN_REFERENCE_COBID_TPDO6_BASE          0x290U
#define CANOPEN_REFERENCE_COBID_LSS_MASTER          0x7E5U
#define CANOPEN_REFERENCE_COBID_LSS_SLAVE           0x7E4U

#define CANOPEN_REFERENCE_SDO_UPLOAD_REQUEST        0x40U
#define CANOPEN_REFERENCE_SDO_UPLOAD_RESPONSE_1    0x4FU
#define CANOPEN_REFERENCE_SDO_UPLOAD_RESPONSE_2    0x4BU
#define CANOPEN_REFERENCE_SDO_UPLOAD_RESPONSE_4    0x43U
#define CANOPEN_REFERENCE_SDO_DOWNLOAD_RESPONSE    0x60U

static inline bool
CanopenReference_NodeIdValid(uint8_t node_id) {
    return node_id >= 0x01U && node_id <= 0x7FU;
}

static inline uint16_t
CanopenReference_HeartbeatCobId(uint8_t node_id) {
    return (uint16_t)(CANOPEN_REFERENCE_COBID_HEARTBEAT_BASE + node_id);
}

static inline uint16_t
CanopenReference_EmcyCobId(uint8_t node_id) {
    return (uint16_t)(CANOPEN_REFERENCE_COBID_EMCY_BASE + node_id);
}

static inline uint16_t
CanopenReference_SdoRequestCobId(uint8_t node_id) {
    return (uint16_t)(CANOPEN_REFERENCE_COBID_SDO_REQUEST_BASE + node_id);
}

static inline uint16_t
CanopenReference_SdoResponseCobId(uint8_t node_id) {
    return (uint16_t)(CANOPEN_REFERENCE_COBID_SDO_RESPONSE_BASE + node_id);
}

static inline uint16_t
CanopenReference_TpdoCobId(uint8_t tpdo_number, uint8_t node_id) {
    static const uint16_t bases[] = {
        CANOPEN_REFERENCE_COBID_TPDO1_BASE,
        CANOPEN_REFERENCE_COBID_TPDO2_BASE,
        CANOPEN_REFERENCE_COBID_TPDO3_BASE,
        CANOPEN_REFERENCE_COBID_TPDO4_BASE,
        CANOPEN_REFERENCE_COBID_TPDO5_BASE,
        CANOPEN_REFERENCE_COBID_TPDO6_BASE
    };
    if (tpdo_number < 1U || tpdo_number > 6U) {
        return 0U;
    }
    return (uint16_t)(bases[tpdo_number - 1U] + node_id);
}

#endif
