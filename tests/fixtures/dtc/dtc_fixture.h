#ifndef STM32_UDS_ISO_TP_DTC_FIXTURE_H
#define STM32_UDS_ISO_TP_DTC_FIXTURE_H

#include "uds_iso_tp/uds_dtc.h"

#include <stdbool.h>
#include <stdint.h>

#define UDS_DTC_FIXTURE_RECORD_COUNT 3U

typedef struct {
    uint32_t number;
    uint8_t status;
    uint8_t severity;
    uint8_t functional_unit;
    uint8_t snapshot[4];
    uint8_t snapshot_length;
    uint8_t extended[3];
    uint8_t extended_length;
    bool active;
} UdsDtcFixtureRecord;

typedef struct {
    UdsDtcBackend backend;
    UdsDtcFixtureRecord records[UDS_DTC_FIXTURE_RECORD_COUNT];
} UdsDtcFixture;

void uds_dtc_fixture_init(UdsDtcFixture *fixture);
const UdsDtcBackend *uds_dtc_fixture_backend(const UdsDtcFixture *fixture);
UdsCallbackResult uds_dtc_fixture_clear(void *context, uint32_t group_of_dtc);

#endif
