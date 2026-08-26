#include "uds_platform_fdcan.h"

#include "stm32c0xx_hal.h"

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
    if ((subfunction == UDS_RESET_TYPE_HARD) || (subfunction == UDS_RESET_TYPE_SOFT))
        uds_c092_platform_system_reset(subfunction);
}

void uds_c092_platform_system_reset(uint8_t reset_type) {
    (void)reset_type;
    NVIC_SystemReset();
}

uint32_t uds_c092_platform_now_ms(void) {
    return HAL_GetTick();
}
