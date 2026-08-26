#include "uds_iso_tp/uds.h"
#include "uds_iso_tp/uds_dtc.h"
#include "uds_iso_tp/uds_services.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static UdsCallbackResult clear_dtc(void *context, uint32_t group_of_dtc) {
    (void)context;
    return (group_of_dtc == 0xFFFFFFU) ? UDS_RESULT_OK : UDS_RESULT_OUT_OF_RANGE;
}

static UdsCallbackResult dtc_report(void *context, uint8_t subfunction, const uint8_t *request,
                                    uint16_t request_length, uint8_t *response,
                                    uint16_t *response_length, uint16_t capacity) {
    (void)context;
    (void)request;
    if ((subfunction != 0x01U) || (request_length != 3U) || (capacity < 2U))
        return UDS_RESULT_OUT_OF_RANGE;
    response[0] = 0x01U;
    response[1] = 0x02U;
    *response_length = 2U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult timing_service(void *context, const uint8_t *request, uint16_t request_len,
                                        uint8_t *response, uint16_t *response_len,
                                        uint16_t capacity) {
    (void)context;
    if ((request[0] != 0x83U) || (request_len != 2U) || (capacity < 2U))
        return UDS_RESULT_OUT_OF_RANGE;
    response[0] = 0xC3U;
    response[1] = request[1];
    *response_len = 2U;
    return UDS_RESULT_OK;
}

int main(void) {
    static const uint8_t supported[] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
                                        0x08U, 0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU,
                                        0x0FU, 0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U,
                                        0x16U, 0x17U, 0x18U, 0x19U, 0x42U, 0x55U};
    for (uint8_t index = 0U; index < (uint8_t)(sizeof(supported) / sizeof(supported[0])); ++index)
        assert(uds_dtc_subfunction_supported(supported[index]));
    assert(!uds_dtc_subfunction_supported(0x20U));
    assert(!uds_dtc_subfunction_supported(0x00U));

    assert(uds_dtc_request_length_valid(0x01U, 3U));
    assert(uds_dtc_request_length_valid(0x04U, 5U));
    assert(uds_dtc_request_length_valid(0x16U, 4U));
    assert(uds_dtc_request_length_valid(0x0BU, 2U));
    assert(!uds_dtc_request_length_valid(0x01U, 2U));
    assert(!uds_dtc_request_length_valid(0x04U, 4U));
    assert(!uds_dtc_request_length_valid(0x20U, 3U));

    const UdsDtcBackend backend = {
        .capabilities = UDS_DTC_CAP_REPORT_NUMBER_BY_STATUS,
        .report = dtc_report,
    };
    const UdsTimingServiceBackend timing_backend = {
        .access_timing_parameters = timing_service,
    };
    const UdsServiceBackends service_backends = {
        .timing = &timing_backend,
    };
    UdsCallbacks callbacks = {
        .dtc_backend = &backend,
        .clear_dtc = clear_dtc,
        .service_backends = &service_backends,
    };
    UdsServer server;
    uds_server_init(&server, &callbacks, NULL, 0U);
    uint8_t response[16] = {0U};
    uint16_t response_length = 0U;
    const uint8_t clear_request[] = {0x14U, 0xFFU, 0xFFU, 0xFFU};
    assert(uds_server_handle(&server, clear_request, sizeof(clear_request), response,
                             &response_length, sizeof(response), 0U) == UDS_RESULT_OK);
    assert(response_length == 1U && response[0] == 0x54U);
    const uint8_t supported_dtc_request[] = {0x19U, 0x01U, 0xFFU};
    assert(uds_server_handle(&server, supported_dtc_request, sizeof(supported_dtc_request),
                             response, &response_length, sizeof(response), 1U) == UDS_RESULT_OK);
    assert(response_length == 3U && response[0] == 0x59U && response[1] == 0x01U &&
           response[2] == 0x02U);
    const uint8_t unsupported_dtc_request[] = {0x19U, 0x02U, 0xFFU};
    assert(uds_server_handle(&server, unsupported_dtc_request, sizeof(unsupported_dtc_request),
                             response, &response_length, sizeof(response), 2U) == UDS_RESULT_OK);
    assert(response_length == 3U && response[0] == 0x7FU && response[1] == 0x19U &&
           response[2] == UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
    const uint8_t extended_request[] = {0x83U, 0x01U};
    assert(uds_server_handle(&server, extended_request, sizeof(extended_request), response,
                             &response_length, sizeof(response), 1U) == UDS_RESULT_OK);
    assert(response_length == 2U && response[0] == 0xC3U && response[1] == 0x01U);
    const uint8_t bad_dtc_subfunction[] = {0x19U, 0x20U};
    assert(uds_server_handle(&server, bad_dtc_subfunction, sizeof(bad_dtc_subfunction), response,
                             &response_length, sizeof(response), 2U) == UDS_RESULT_OK);
    assert(response_length == 3U && response[0] == 0x7FU && response[1] == 0x19U &&
           response[2] == UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
    const UdsCallbacks legacy_callbacks = {0};
    UdsServer legacy_server;
    uds_server_init(&legacy_server, &legacy_callbacks, NULL, 0U);
    const uint8_t unavailable_dtc[] = {0x19U, 0x01U, 0xFFU};
    assert(uds_server_handle(&legacy_server, unavailable_dtc, sizeof(unavailable_dtc), response,
                             &response_length, sizeof(response), 3U) == UDS_RESULT_OK);
    assert(response_length == 3U && response[0] == 0x7FU && response[1] == 0x19U &&
           response[2] == UDS_NRC_SERVICE_NOT_SUPPORTED);
    return 0;
}
