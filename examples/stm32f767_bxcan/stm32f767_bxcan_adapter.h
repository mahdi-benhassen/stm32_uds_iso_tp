#ifndef STM32_UDS_ISO_TP_STM32F767_BXCAN_ADAPTER_H
#define STM32_UDS_ISO_TP_STM32F767_BXCAN_ADAPTER_H

#include "uds_iso_tp/endpoint.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool (*send_classic)(void *context, uint32_t can_id, const uint8_t *data, uint8_t dlc);
    uint32_t (*now_ms)(void *context);
    void *context;
} Stm32F767BxCanBinding;

bool stm32f767_bxcan_endpoint_configure(UdsIsoTpEndpointConfig *config,
                                        Stm32F767BxCanBinding *binding,
                                        const UdsCallbacks *callbacks, void *uds_context);

#endif
