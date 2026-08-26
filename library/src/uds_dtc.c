/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds_iso_tp/uds_dtc.h"

#include <stddef.h>

bool uds_dtc_subfunction_supported(uint8_t subfunction) {
    switch (subfunction) {
    case 0x01U:
    case 0x02U:
    case 0x03U:
    case 0x04U:
    case 0x05U:
    case 0x06U:
    case 0x07U:
    case 0x08U:
    case 0x09U:
    case 0x0AU:
    case 0x0BU:
    case 0x0CU:
    case 0x0DU:
    case 0x0EU:
    case 0x0FU:
    case 0x10U:
    case 0x11U:
    case 0x12U:
    case 0x13U:
    case 0x14U:
    case 0x15U:
    case 0x16U:
    case 0x17U:
    case 0x18U:
    case 0x19U:
    case 0x42U:
    case 0x55U:
        return true;
    default:
        return false;
    }
}

bool uds_dtc_request_length_valid(uint8_t subfunction, uint16_t request_length) {
    if (!uds_dtc_subfunction_supported(subfunction))
        return false;
    switch (subfunction) {
    case 0x01U:
    case 0x02U:
    case 0x05U:
    case 0x07U:
    case 0x08U:
    case 0x0AU:
    case 0x0FU:
    case 0x11U:
    case 0x12U:
    case 0x13U:
    case 0x15U:
    case 0x17U:
    case 0x42U:
    case 0x55U:
        return request_length == 3U;
    case 0x04U:
    case 0x06U:
    case 0x09U:
    case 0x10U:
    case 0x14U:
    case 0x19U:
        return request_length == 5U;
    case 0x16U:
    case 0x18U:
        return request_length == 4U;
    case 0x0BU:
    case 0x0CU:
    case 0x0DU:
    case 0x0EU:
    case 0x03U:
        return request_length == 2U;
    default:
        return false;
    }
}

uint32_t uds_dtc_capability_for_subfunction(uint8_t subfunction) {
    switch (subfunction) {
    case 0x01U:
        return UDS_DTC_CAP_REPORT_NUMBER_BY_STATUS;
    case 0x02U:
        return UDS_DTC_CAP_REPORT_BY_STATUS_MASK;
    case 0x03U:
        return UDS_DTC_CAP_REPORT_SNAPSHOT_IDENTIFICATION;
    case 0x04U:
        return UDS_DTC_CAP_REPORT_SNAPSHOT_RECORDS;
    case 0x05U:
        return UDS_DTC_CAP_REPORT_MIRROR_MEMORY;
    case 0x06U:
        return UDS_DTC_CAP_REPORT_EXTENDED_DATA;
    case 0x07U:
        return UDS_DTC_CAP_REPORT_NUMBER_BY_SEVERITY;
    case 0x08U:
        return UDS_DTC_CAP_REPORT_BY_SEVERITY;
    case 0x09U:
        return (1UL << 25U);
    case 0x0AU:
        return UDS_DTC_CAP_REPORT_SUPPORTED_DTC;
    case 0x0BU:
        return UDS_DTC_CAP_REPORT_FIRST_FAILED;
    case 0x0CU:
        return UDS_DTC_CAP_REPORT_FIRST_CONFIRMED;
    case 0x0DU:
        return UDS_DTC_CAP_REPORT_MOST_RECENT_FAILED;
    case 0x0EU:
        return UDS_DTC_CAP_REPORT_MOST_RECENT_CONFIRMED;
    case 0x0FU:
        return UDS_DTC_CAP_REPORT_MIRROR_EXTENDED_DATA;
    case 0x10U:
        return UDS_DTC_CAP_REPORT_MIRROR_EXTENDED_DATA;
    case 0x11U:
        return UDS_DTC_CAP_REPORT_NUMBER_BY_SEVERITY_MASK;
    case 0x12U:
        return UDS_DTC_CAP_REPORT_BY_SEVERITY_MASK;
    case 0x13U:
        return UDS_DTC_CAP_REPORT_USER_MEMORY;
    case 0x14U:
        return UDS_DTC_CAP_REPORT_USER_MEMORY_EXTENDED_DATA;
    case 0x15U:
        return UDS_DTC_CAP_REPORT_PERMANENT_STATUS;
    case 0x16U:
        return UDS_DTC_CAP_REPORT_PERMANENT_STATUS_MASK;
    case 0x17U:
        return UDS_DTC_CAP_REPORT_WWHOBD_STATUS;
    case 0x18U:
        return UDS_DTC_CAP_REPORT_WWHOBD_STATUS_MASK;
    case 0x19U:
        return UDS_DTC_CAP_REPORT_BY_SEVERITY_RECORDS;
    case 0x42U:
        return UDS_DTC_CAP_CUSTOM_42;
    case 0x55U:
        return UDS_DTC_CAP_CUSTOM_55;
    default:
        return 0U;
    }
}

bool uds_dtc_backend_supports(const UdsDtcBackend *backend, uint8_t subfunction) {
    uint32_t required = uds_dtc_capability_for_subfunction(subfunction);
    return (backend != NULL) && (backend->report != NULL) && (required != 0U) &&
           ((backend->capabilities & required) != 0U);
}
