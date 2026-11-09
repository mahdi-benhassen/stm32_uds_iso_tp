#ifndef BCM_UDS_ADAPTER_H
#define BCM_UDS_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_iso_tp/uds.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BCM_DID_STATE_SNAPSHOT          0x0100U
#define BCM_DID_POWER_ENVIRONMENT       0x0101U
#define BCM_DID_IO_HEADLIGHT            0x0102U
#define BCM_DID_IO_HORN                 0x0103U
#define BCM_DID_IO_HAZARD               0x0104U
#define BCM_DID_VIN                     0xF190U
#define BCM_DID_PART_NUMBER             0xF187U

#define BCM_ROUTINE_WIPER_SWEEP_TEST    0x0301U

#define BCM_DTC_LIGHT_CIRCUIT_FAULT     0xB10001U
#define BCM_DTC_DOOR_ACTUATOR_STALL     0xB10002U
#define BCM_DTC_BATTERY_VOLTAGE_LOW     0x900116U

void bcm_uds_adapter_init(UdsServer *server, uint32_t now_ms);
void bcm_uds_adapter_poll(UdsServer *server, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* BCM_UDS_ADAPTER_H */
