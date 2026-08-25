/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "canopen_reference_lss.h"

#include "canopen_reference_co.h"

__attribute__((weak)) bool
CANopenReferenceLss_BoardStore(uint8_t node_id, uint16_t bitrate_kbps) {
    (void)node_id;
    (void)bitrate_kbps;
    return false;
}

__attribute__((weak)) void
CANopenReferenceLss_BoardActivate(uint16_t delay_ms) {
    (void)delay_ms;
}

bool
CANopenReferenceLss_BitrateSupported(uint16_t bitrate_kbps) {
    switch (bitrate_kbps) {
        case 10U:
        case 20U:
        case 50U:
        case 125U:
        case 250U:
        case 500U:
        case 800U:
        case 1000U:
            return true;
        default:
            return false;
    }
}

void
CANopenReferenceLss_ActivateBitrate(uint16_t delay_ms) {
    CANopenReferenceLss_BoardActivate(delay_ms);
}

bool
CANopenReferenceLss_StoreConfiguration(uint8_t node_id, uint16_t bitrate_kbps) {
    if (node_id == 0U || node_id > 127U || !CANopenReferenceLss_BitrateSupported(bitrate_kbps)) {
        return false;
    }
    return CANopenReferenceLss_BoardStore(node_id, bitrate_kbps);
}

#if ((CO_CONFIG_LSS) & CO_CONFIG_LSS_SLAVE)
static bool_t
check_bitrate(void *object, uint16_t bitrate) {
    (void)object;
    return CANopenReferenceLss_BitrateSupported(bitrate) ? true : false;
}

static void
activate_bitrate(void *object, uint16_t delay) {
    CANopenReferenceLssState *state = object;
    if (state != NULL) {
        state->activation_delay_ms = delay;
    }
    CANopenReferenceLss_ActivateBitrate(delay);
}

static bool_t
store_configuration(void *object, uint8_t node_id, uint16_t bitrate) {
    CANopenReferenceLssState *state = object;
    bool stored = CANopenReferenceLss_StoreConfiguration(node_id, bitrate);
    if (state != NULL) {
        state->node_id = node_id;
        state->bitrate_kbps = bitrate;
        state->store_requested = true;
    }
    return stored ? true : false;
}
#endif

void
CANopenReferenceLss_Init(CO_t *co, CANopenReferenceLssState *state) {
#if ((CO_CONFIG_LSS) & CO_CONFIG_LSS_SLAVE)
    if (co != NULL && co->LSSslave != NULL) {
        CO_LSSslave_initCkBitRateCall(co->LSSslave, state, check_bitrate);
        CO_LSSslave_initActBitRateCall(co->LSSslave, state, activate_bitrate);
        CO_LSSslave_initCfgStoreCall(co->LSSslave, state, store_configuration);
    }
#else
    (void)co;
    (void)state;
#endif
}
