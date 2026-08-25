#ifndef STM32_UDS_ISO_TP_STM32_FDCAN_ADAPTER_H
#define STM32_UDS_ISO_TP_STM32_FDCAN_ADAPTER_H

#include "uds_iso_tp/endpoint.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool (*send_fd)(void *context, uint32_t can_id, const uint8_t *data, uint8_t dlc,
                    bool bit_rate_switch);
    uint32_t (*now_ms)(void *context);
    void *context;
} Stm32FdCanBinding;

bool stm32_fdcan_endpoint_configure(UdsIsoTpEndpointConfig *config, Stm32FdCanBinding *binding,
                                    const UdsCallbacks *callbacks, void *uds_context);

#endif
