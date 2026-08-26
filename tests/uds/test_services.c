#include "uds_iso_tp/uds_services.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static UdsCallbackResult handler(void *context, const uint8_t *request, uint16_t request_length,
                                 uint8_t *response, uint16_t *response_length,
                                 uint16_t response_capacity) {
    (void)context;
    (void)request;
    (void)request_length;
    (void)response;
    (void)response_length;
    (void)response_capacity;
    return UDS_RESULT_OK;
}

int main(void) {
    const UdsMemoryServiceBackend memory = {.read_memory = handler, .write_memory = handler};
    const UdsDidServiceBackend did = {.read_scaling = handler, .dynamically_define = handler};
    const UdsTransferServiceBackend transfer = {.request_upload = handler, .request_file = handler};
    const UdsTimingServiceBackend timing = {.access_timing_parameters = handler};
    const UdsPeriodicEventServiceBackend periodic_event = {
        .periodic_data = handler,
        .event_response = handler,
    };
    const UdsLinkControlServiceBackend link_control = {.link_control = handler};
    const UdsAuthenticationServiceBackend authentication = {.authentication = handler};
    const UdsSecuredDataServiceBackend secured_data = {
        .secured_data_transmission = handler,
    };
    const UdsServiceBackends backends = {
        .memory = &memory,
        .did = &did,
        .transfer = &transfer,
        .timing = &timing,
        .periodic_event = &periodic_event,
        .link_control = &link_control,
        .authentication = &authentication,
        .secured_data = &secured_data,
    };
    const uint8_t service_ids[] = {0x23U, 0x3DU, 0x24U, 0x2CU, 0x35U, 0x38U,
                                   0x83U, 0x2AU, 0x86U, 0x87U, 0x29U, 0x84U};
    for (size_t index = 0U; index < sizeof(service_ids); ++index)
        assert(uds_service_backends_handler(&backends, service_ids[index]) == handler);
    assert(uds_service_backends_handler(&backends, 0x99U) == NULL);
    assert(uds_service_backends_handler(NULL, 0x23U) == NULL);
    return 0;
}
