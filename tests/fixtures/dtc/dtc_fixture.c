#include "dtc_fixture.h"

#include <stddef.h>

static bool append_byte(uint8_t *response, uint16_t *length, uint16_t capacity, uint8_t value) {
    if ((*length >= capacity) || (response == NULL))
        return false;
    response[(*length)++] = value;
    return true;
}

static bool append_dtc(uint8_t *response, uint16_t *length, uint16_t capacity,
                       const UdsDtcFixtureRecord *record) {
    return append_byte(response, length, capacity, (uint8_t)(record->number >> 16U)) &&
           append_byte(response, length, capacity, (uint8_t)(record->number >> 8U)) &&
           append_byte(response, length, capacity, (uint8_t)record->number) &&
           append_byte(response, length, capacity, record->status);
}

static bool request_selects_record(const UdsDtcFixtureRecord *record, const uint8_t *request,
                                   uint16_t request_length) {
    if (request_length < 5U)
        return true;
    uint32_t requested = ((uint32_t)request[2] << 16U) | ((uint32_t)request[3] << 8U) | request[4];
    return (requested == 0xFFFFFFUL) || (requested == record->number);
}

static bool append_record_data(uint8_t *response, uint16_t *length, uint16_t capacity,
                               const UdsDtcFixtureRecord *record, bool snapshot) {
    const uint8_t *data = snapshot ? record->snapshot : record->extended;
    uint8_t data_length = snapshot ? record->snapshot_length : record->extended_length;
    if (!append_dtc(response, length, capacity, record) ||
        !append_byte(response, length, capacity, snapshot ? 0x01U : 0x02U) ||
        !append_byte(response, length, capacity, data_length))
        return false;
    for (uint8_t index = 0U; index < data_length; ++index) {
        if (!append_byte(response, length, capacity, data[index]))
            return false;
    }
    return true;
}

static uint8_t count_matching(const UdsDtcFixture *fixture, uint8_t status_mask,
                              uint8_t severity_min, const uint8_t *request,
                              uint16_t request_length) {
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < UDS_DTC_FIXTURE_RECORD_COUNT; ++index) {
        const UdsDtcFixtureRecord *record = &fixture->records[index];
        if (record->active && ((record->status & status_mask) != 0U) &&
            (record->severity >= severity_min) &&
            request_selects_record(record, request, request_length))
            count++;
    }
    return count;
}

static UdsCallbackResult fixture_report(void *context, uint8_t subfunction, const uint8_t *request,
                                        uint16_t request_length, uint8_t *response,
                                        uint16_t *response_length, uint16_t capacity) {
    UdsDtcFixture *fixture = (UdsDtcFixture *)context;
    if ((fixture == NULL) || (request == NULL) || (response == NULL) || (response_length == NULL) ||
        (capacity < 2U))
        return UDS_RESULT_ERROR;
    uint8_t status_mask = (request_length >= 3U) ? request[2] : 0xFFU;
    uint8_t severity_min =
        ((subfunction == 0x07U) || (subfunction == 0x08U) || (subfunction == 0x11U) ||
         (subfunction == 0x12U) || (subfunction == 0x19U))
            ? ((request_length >= 4U) ? request[3] : 0U)
            : 0U;
    uint16_t length = 0U;
    if (!append_byte(response, &length, capacity, subfunction))
        return UDS_RESULT_ERROR;

    if ((subfunction == 0x03U) || (subfunction == 0x04U) || (subfunction == 0x06U)) {
        bool snapshot = subfunction != 0x06U;
        uint8_t appended = 0U;
        for (uint8_t index = 0U; index < UDS_DTC_FIXTURE_RECORD_COUNT; ++index) {
            const UdsDtcFixtureRecord *record = &fixture->records[index];
            if (!record->active || !request_selects_record(record, request, request_length))
                continue;
            if (!append_record_data(response, &length, capacity, record, snapshot))
                return UDS_RESULT_RESPONSE_TOO_LONG;
            appended++;
        }
        if (appended == 0U)
            return UDS_RESULT_OUT_OF_RANGE;
    } else {
        uint8_t count = count_matching(fixture, status_mask, severity_min, request, request_length);
        if (!append_byte(response, &length, capacity, count))
            return UDS_RESULT_RESPONSE_TOO_LONG;
        for (uint8_t index = 0U; index < UDS_DTC_FIXTURE_RECORD_COUNT; ++index) {
            const UdsDtcFixtureRecord *record = &fixture->records[index];
            if (!record->active || ((record->status & status_mask) == 0U) ||
                (record->severity < severity_min) ||
                !request_selects_record(record, request, request_length))
                continue;
            if (!append_dtc(response, &length, capacity, record))
                return UDS_RESULT_RESPONSE_TOO_LONG;
        }
    }
    *response_length = length;
    return UDS_RESULT_OK;
}

void uds_dtc_fixture_init(UdsDtcFixture *fixture) {
    if (fixture == NULL)
        return;
    fixture->records[0] = (UdsDtcFixtureRecord){
        0x010203UL, 0x01U, 0x10U, 0x01U, {0xA1U, 0xB2U, 0xC3U, 0xD4U}, 4U, {0x11U, 0x22U, 0x33U},
        3U,         true};
    fixture->records[1] = (UdsDtcFixtureRecord){
        0x0A0B0CUL, 0x08U, 0x40U, 0x02U, {0xE1U, 0xF2U}, 2U, {0x44U, 0x55U, 0x66U}, 3U, true};
    fixture->records[2] = (UdsDtcFixtureRecord){
        0xA0B0C0UL, 0x20U, 0x70U, 0x03U, {0x71U, 0x82U, 0x93U}, 3U, {0x77U}, 1U, true};
    fixture->backend.capabilities = 0x03FFFFFFUL;
    fixture->backend.report = fixture_report;
}

const UdsDtcBackend *uds_dtc_fixture_backend(const UdsDtcFixture *fixture) {
    return (fixture != NULL) ? &fixture->backend : NULL;
}

UdsCallbackResult uds_dtc_fixture_clear(void *context, uint32_t group_of_dtc) {
    UdsDtcFixture *fixture = (UdsDtcFixture *)context;
    if ((fixture == NULL) || (group_of_dtc != 0xFFFFFFUL))
        return UDS_RESULT_OUT_OF_RANGE;
    for (uint8_t index = 0U; index < UDS_DTC_FIXTURE_RECORD_COUNT; ++index)
        fixture->records[index].active = false;
    return UDS_RESULT_OK;
}
