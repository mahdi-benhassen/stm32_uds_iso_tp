#ifndef STM32_UDS_ISO_TP_C092_UDS_PLATFORM_FDCAN_H
#define STM32_UDS_ISO_TP_C092_UDS_PLATFORM_FDCAN_H

#include <stdint.h>

uint32_t uds_c092_platform_now_ms(void);
void uds_c092_platform_system_reset(uint8_t reset_type);

#endif
