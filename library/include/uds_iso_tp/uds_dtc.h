/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#ifndef STM32_UDS_ISO_TP_UDS_DTC_H
#define STM32_UDS_ISO_TP_UDS_DTC_H

#include "uds_iso_tp/uds.h"

#include <stdbool.h>
#include <stdint.h>

/* Capability bits correspond to the ReadDTCInformation subfunction byte. */
#define UDS_DTC_CAP_REPORT_NUMBER_BY_STATUS (1UL << 0U)           /* 0x01 */
#define UDS_DTC_CAP_REPORT_BY_STATUS_MASK (1UL << 1U)             /* 0x02 */
#define UDS_DTC_CAP_REPORT_SNAPSHOT_IDENTIFICATION (1UL << 2U)    /* 0x03 */
#define UDS_DTC_CAP_REPORT_SNAPSHOT_RECORDS (1UL << 3U)           /* 0x04 */
#define UDS_DTC_CAP_REPORT_MIRROR_MEMORY (1UL << 4U)              /* 0x05 */
#define UDS_DTC_CAP_REPORT_EXTENDED_DATA (1UL << 5U)              /* 0x06 */
#define UDS_DTC_CAP_REPORT_NUMBER_BY_SEVERITY (1UL << 6U)         /* 0x07 */
#define UDS_DTC_CAP_REPORT_BY_SEVERITY (1UL << 7U)                /* 0x08 */
#define UDS_DTC_CAP_REPORT_SUPPORTED_DTC (1UL << 8U)              /* 0x0A */
#define UDS_DTC_CAP_REPORT_FIRST_FAILED (1UL << 9U)               /* 0x0B */
#define UDS_DTC_CAP_REPORT_FIRST_CONFIRMED (1UL << 10U)           /* 0x0C */
#define UDS_DTC_CAP_REPORT_MOST_RECENT_FAILED (1UL << 11U)        /* 0x0D */
#define UDS_DTC_CAP_REPORT_MOST_RECENT_CONFIRMED (1UL << 12U)     /* 0x0E */
#define UDS_DTC_CAP_REPORT_MIRROR_EXTENDED_DATA (1UL << 13U)      /* 0x0F */
#define UDS_DTC_CAP_REPORT_NUMBER_BY_SEVERITY_MASK (1UL << 14U)   /* 0x11 */
#define UDS_DTC_CAP_REPORT_BY_SEVERITY_MASK (1UL << 15U)          /* 0x12 */
#define UDS_DTC_CAP_REPORT_USER_MEMORY (1UL << 16U)               /* 0x13 */
#define UDS_DTC_CAP_REPORT_USER_MEMORY_EXTENDED_DATA (1UL << 17U) /* 0x14 */
#define UDS_DTC_CAP_REPORT_PERMANENT_STATUS (1UL << 18U)          /* 0x15 */
#define UDS_DTC_CAP_REPORT_PERMANENT_STATUS_MASK (1UL << 19U)     /* 0x16 */
#define UDS_DTC_CAP_REPORT_WWHOBD_STATUS (1UL << 20U)             /* 0x17 */
#define UDS_DTC_CAP_REPORT_WWHOBD_STATUS_MASK (1UL << 21U)        /* 0x18 */
#define UDS_DTC_CAP_REPORT_BY_SEVERITY_RECORDS (1UL << 22U)       /* 0x19 */
#define UDS_DTC_CAP_CUSTOM_42 (1UL << 23U)
#define UDS_DTC_CAP_CUSTOM_55 (1UL << 24U)

typedef UdsCallbackResult (*UdsDtcReportFn)(void *context, uint8_t subfunction,
                                            const uint8_t *request, uint16_t request_length,
                                            uint8_t *response, uint16_t *response_length,
                                            uint16_t response_capacity);

typedef struct UdsDtcBackend {
    uint32_t capabilities;
    UdsDtcReportFn report;
} UdsDtcBackend;

bool uds_dtc_subfunction_supported(uint8_t subfunction);
bool uds_dtc_request_length_valid(uint8_t subfunction, uint16_t request_length);
uint32_t uds_dtc_capability_for_subfunction(uint8_t subfunction);
bool uds_dtc_backend_supports(const UdsDtcBackend *backend, uint8_t subfunction);

#endif
