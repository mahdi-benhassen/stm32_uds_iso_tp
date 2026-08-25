#include "uds_platform.h"

uint32_t uds_platform_now_ms(void) {
    return HAL_GetTick();
}

void uds_platform_system_reset(uint8_t reset_type) {
    (void)reset_type;
    NVIC_SystemReset();
}

void uds_platform_error(void) {
    Error_Handler();
}
