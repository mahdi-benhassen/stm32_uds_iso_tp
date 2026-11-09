#ifndef BMS_UDS_ADAPTER_H
#define BMS_UDS_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_iso_tp/uds.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BMS_DID_PACK_SUMMARY            0x0200U
#define BMS_DID_CELL_EXTREMES           0x0201U
#define BMS_DID_CONTACTOR_STATUS        0x0202U
#define BMS_DID_ISOLATION_STATUS        0x0203U

#define BMS_ROUTINE_CONTACTOR_CYCLE     0x0201U
#define BMS_ROUTINE_CALIBRATE_SOC       0x0202U

#define BMS_DTC_CELL_OVERVOLTAGE        0xA00117U
#define BMS_DTC_CELL_UNDERVOLTAGE       0xA00116U
#define BMS_DTC_ISOLATION_LOSS          0xA00219U
#define BMS_DTC_CONTACTOR_WELDED        0xA0034BU

void bms_uds_adapter_init(UdsServer *server, uint32_t now_ms);
void bms_uds_adapter_poll(UdsServer *server, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* BMS_UDS_ADAPTER_H */
