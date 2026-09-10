#ifndef STM32_UDS_ISO_TP_C092_UDS_PLATFORM_FDCAN_H
#define STM32_UDS_ISO_TP_C092_UDS_PLATFORM_FDCAN_H

#include "uds_iso_tp/uds.h"

#include <stdint.h>

UdsCallbackResult uds_c092_platform_reset_prepare(void *context, uint8_t subfunction);
void uds_c092_platform_reset_execute(void *context, uint8_t subfunction);
uint32_t uds_c092_platform_now_ms(void);
void uds_c092_platform_system_reset(uint8_t reset_type);

#ifndef UDS_C092_RESET_TX_WAIT_MS
#define UDS_C092_RESET_TX_WAIT_MS 50U
#endif

typedef struct {
    uint8_t reset_type_requested;
    uint32_t reset_wait_started_ms;
} UdsC092ResetPending;

extern UdsC092ResetPending uds_c092_reset_pending;
void uds_c092_platform_reset_poll(void);

#endif
