#include "stm32f767_bxcan_adapter.h"

#include <stddef.h>

static bool bxcan_send(void *context, const IsoTpCanFrame *frame) {
    Stm32F767BxCanBinding *binding = (Stm32F767BxCanBinding *)context;
    if ((binding == NULL) || (binding->send_classic == NULL) || frame->is_fd)
        return false;
    return binding->send_classic(binding->context, frame->can_id, frame->data, frame->dlc);
}

static uint32_t bxcan_clock(void *context) {
    Stm32F767BxCanBinding *binding = (Stm32F767BxCanBinding *)context;
    return ((binding != NULL) && (binding->now_ms != NULL)) ? binding->now_ms(binding->context)
                                                            : 0U;
}

bool stm32f767_bxcan_endpoint_configure(UdsIsoTpEndpointConfig *config,
                                        Stm32F767BxCanBinding *binding,
                                        const UdsCallbacks *callbacks, void *uds_context) {
    if ((config == NULL) || (binding == NULL) || (callbacks == NULL))
        return false;
    isotp_config_classic_can(&config->isotp_config);
    config->send_frame = bxcan_send;
    config->clock_ms = bxcan_clock;
    config->context = binding;
    config->request_id = 0x7E0U;
    config->response_id = 0x7E8U;
    config->uds_callbacks = *callbacks;
    config->uds_context = uds_context;
    return true;
}
