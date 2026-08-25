#ifndef STM32_UDS_ISO_TP_UDS_PLATFORM_H
#define STM32_UDS_ISO_TP_UDS_PLATFORM_H

#include "main.h"

#include <stdint.h>

uint32_t uds_platform_now_ms(void);
void uds_platform_error(void);

#endif
