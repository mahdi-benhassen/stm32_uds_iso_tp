#include "uds_platform.h"

uint32_t uds_platform_now_ms(void) {
    return HAL_GetTick();
}

void uds_platform_error(void) {
    Error_Handler();
}
