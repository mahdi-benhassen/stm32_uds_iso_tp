#include "uds_platform_fdcan.h"

#include "stm32c0xx_hal.h"

UdsC092ResetPending uds_c092_reset_pending = {0U, 0U};

UdsCallbackResult uds_c092_platform_reset_prepare(void *context, uint8_t subfunction) {
    (void)context;
    switch (subfunction) {
    case UDS_RESET_TYPE_HARD:
    case UDS_RESET_TYPE_SOFT:
        return UDS_RESULT_OK;
    case UDS_RESET_TYPE_KEY_OFF_ON:
    case UDS_RESET_TYPE_ENABLE_RAPID_POWER_SHUTDOWN:
    case UDS_RESET_TYPE_DISABLE_RAPID_POWER_SHUTDOWN:
        return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
    default:
        return UDS_RESULT_OUT_OF_RANGE;
    }
}

void uds_c092_platform_reset_execute(void *context, uint8_t subfunction) {
    (void)context;
    uds_c092_reset_pending.reset_type_requested = subfunction;
    uds_c092_reset_pending.reset_wait_started_ms = uds_c092_platform_now_ms();
}

void uds_c092_platform_reset_poll(void) {
    if ((uds_c092_reset_pending.reset_type_requested != 0U) &&
        ((uint32_t)(uds_c092_platform_now_ms() - uds_c092_reset_pending.reset_wait_started_ms) >=
         UDS_C092_RESET_TX_WAIT_MS)) {
        uint8_t reset_type = uds_c092_reset_pending.reset_type_requested;
        uds_c092_reset_pending.reset_type_requested = 0U;
        uds_c092_reset_pending.reset_wait_started_ms = 0U;
        uds_c092_platform_system_reset(reset_type);
    }
}

void uds_c092_platform_system_reset(uint8_t reset_type) {
    (void)reset_type;
    NVIC_SystemReset();
}

uint32_t uds_c092_platform_now_ms(void) {
    return HAL_GetTick();
}
