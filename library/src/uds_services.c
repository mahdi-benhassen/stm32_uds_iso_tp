/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds_iso_tp/uds_services.h"

#include <stddef.h>

UdsServiceHandlerFn uds_service_backends_handler(const UdsServiceBackends *backends, uint8_t sid) {
    if (backends == NULL)
        return NULL;
    switch (sid) {
    case 0x23U:
        return (backends->memory != NULL) ? backends->memory->read_memory : NULL;
    case 0x3DU:
        return (backends->memory != NULL) ? backends->memory->write_memory : NULL;
    case 0x24U:
        return (backends->did != NULL) ? backends->did->read_scaling : NULL;
    case 0x2CU:
        return (backends->did != NULL) ? backends->did->dynamically_define : NULL;
    case 0x35U:
        return (backends->transfer != NULL) ? backends->transfer->request_upload : NULL;
    case 0x38U:
        return (backends->transfer != NULL) ? backends->transfer->request_file : NULL;
    case 0x83U:
        return (backends->timing != NULL) ? backends->timing->access_timing_parameters : NULL;
    case 0x2AU:
        return (backends->periodic_event != NULL) ? backends->periodic_event->periodic_data : NULL;
    case 0x86U:
        return (backends->periodic_event != NULL) ? backends->periodic_event->event_response : NULL;
    case 0x87U:
        return (backends->link_control != NULL) ? backends->link_control->link_control : NULL;
    case 0x29U:
        return (backends->authentication != NULL) ? backends->authentication->authentication : NULL;
    case 0x84U:
        return (backends->secured_data != NULL) ? backends->secured_data->secured_data_transmission
                                                : NULL;
    default:
        return NULL;
    }
}
