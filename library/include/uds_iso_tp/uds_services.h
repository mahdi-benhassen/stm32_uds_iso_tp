/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#ifndef STM32_UDS_ISO_TP_UDS_SERVICES_H
#define STM32_UDS_ISO_TP_UDS_SERVICES_H

#include "uds_iso_tp/uds.h"

#include <stdint.h>

typedef UdsCallbackResult (*UdsServiceHandlerFn)(void *context, const uint8_t *request,
                                                 uint16_t request_length, uint8_t *response,
                                                 uint16_t *response_length,
                                                 uint16_t response_capacity);

typedef UdsCallbackResult (*UdsMemoryAccessFn)(void *context, uint8_t sid, const uint8_t *request,
                                               uint16_t request_length);

typedef struct {
    UdsMemoryAccessFn check_access;   /* secure region/permission preflight */
    UdsServiceHandlerFn read_memory;  /* 0x23 */
    UdsServiceHandlerFn write_memory; /* 0x3D */
} UdsMemoryServiceBackend;

typedef struct {
    UdsServiceHandlerFn read_scaling;       /* 0x24 */
    UdsServiceHandlerFn dynamically_define; /* 0x2C */
} UdsDidServiceBackend;

typedef struct {
    UdsServiceHandlerFn request_upload; /* 0x35 */
    UdsServiceHandlerFn request_file;   /* 0x38 */
} UdsTransferServiceBackend;

typedef struct {
    UdsServiceHandlerFn access_timing_parameters; /* 0x83 */
} UdsTimingServiceBackend;

typedef struct {
    uint16_t max_pending_items;         /* nonzero application queue bound */
    UdsServiceHandlerFn periodic_data;  /* 0x2A */
    UdsServiceHandlerFn event_response; /* 0x86 */
} UdsPeriodicEventServiceBackend;

typedef struct {
    UdsServiceHandlerFn link_control; /* 0x87 */
} UdsLinkControlServiceBackend;

typedef struct {
    UdsServiceHandlerFn authentication; /* 0x29 */
} UdsAuthenticationServiceBackend;

typedef struct {
    UdsServiceHandlerFn secured_data_transmission; /* 0x84 */
} UdsSecuredDataServiceBackend;

struct UdsServiceBackends {
    const UdsMemoryServiceBackend *memory;
    const UdsDidServiceBackend *did;
    const UdsTransferServiceBackend *transfer;
    const UdsTimingServiceBackend *timing;
    const UdsPeriodicEventServiceBackend *periodic_event;
    const UdsLinkControlServiceBackend *link_control;
    const UdsAuthenticationServiceBackend *authentication;
    const UdsSecuredDataServiceBackend *secured_data;
};

UdsCallbackResult uds_service_backends_preflight(const UdsServiceBackends *backends, void *context,
                                                 uint8_t sid, const uint8_t *request,
                                                 uint16_t request_length);
UdsServiceHandlerFn uds_service_backends_handler(const UdsServiceBackends *backends, uint8_t sid);

#endif
