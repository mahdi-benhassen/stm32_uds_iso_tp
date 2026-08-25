#include "uds_platform_fdcan.h"

#include "stm32c0xx_hal.h"

void uds_c092_platform_system_reset(uint8_t reset_type) {
    (void)reset_type;
    NVIC_SystemReset();
}

uint32_t uds_c092_platform_now_ms(void) {
    return HAL_GetTick();
}
