#include "stm32_fdcan_adapter.h"

#include <stddef.h>

static bool fdcan_send(void *context, const IsoTpCanFrame *frame) {
    Stm32FdCanBinding *binding = (Stm32FdCanBinding *)context;
    if ((binding == NULL) || (binding->send_fd == NULL) || !frame->is_fd)
        return false;
    return binding->send_fd(binding->context, frame->can_id, frame->data, frame->dlc,
                            frame->bit_rate_switch);
}

static uint32_t fdcan_clock(void *context) {
    Stm32FdCanBinding *binding = (Stm32FdCanBinding *)context;
    return ((binding != NULL) && (binding->now_ms != NULL)) ? binding->now_ms(binding->context)
                                                            : 0U;
}

bool stm32_fdcan_endpoint_configure(UdsIsoTpEndpointConfig *config, Stm32FdCanBinding *binding,
                                    const UdsCallbacks *callbacks, void *uds_context) {
    if ((config == NULL) || (binding == NULL) || (callbacks == NULL))
        return false;
    isotp_config_can_fd(&config->isotp_config, 64U, 64U);
    config->isotp_config.bit_rate_switch = true;
    config->send_frame = fdcan_send;
    config->clock_ms = fdcan_clock;
    config->context = binding;
    config->request_id = 0x7E0U;
    config->response_id = 0x7E8U;
    config->uds_callbacks = *callbacks;
    config->uds_context = uds_context;
    return true;
}
