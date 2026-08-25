/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#ifndef STM32_UDS_ISO_TP_DID_H
#define STM32_UDS_ISO_TP_DID_H

#include <stdbool.h>
#include <stdint.h>

#define UDS_DID_SESSION_DEFAULT_MASK (1U << 0U)
#define UDS_DID_SESSION_PROGRAMMING_MASK (1U << 1U)
#define UDS_DID_SESSION_EXTENDED_MASK (1U << 2U)
#define UDS_DID_SESSION_ALL_MASK                                                                   \
    (UDS_DID_SESSION_DEFAULT_MASK | UDS_DID_SESSION_PROGRAMMING_MASK |                             \
     UDS_DID_SESSION_EXTENDED_MASK)

#define UDS_DID_SOFTWARE_VERSION 0xF180U
#define UDS_DID_BOOTLOADER_VERSION 0xF181U
#define UDS_DID_DEVICE_IDENTITY 0xF18AU
#define UDS_DID_SERIAL_NUMBER 0xF18CU
#define UDS_DID_HARDWARE_VERSION 0xF191U
#define UDS_DID_CONTROLLER_ID 0xF1A0U
#define UDS_DID_CAN_BITRATE 0xF1A1U
#define UDS_DID_DIAGNOSTIC_STATUS 0xF1B0U
#define UDS_DID_FIRMWARE_STATUS 0xF1B1U
#define UDS_DID_RESET_CAUSE 0xF1B2U
#define UDS_DID_WATCHDOG_STATUS 0xF1B3U
#define UDS_DID_CAN_ERROR_COUNTERS 0xF1B4U
#define UDS_DID_NETWORK_ERROR_STATE 0xF1B5U

#define UDS_PROJECT_DID_COUNT 13U

typedef enum {
    UDS_DID_OK = 0,
    UDS_DID_NOT_FOUND,
    UDS_DID_NOT_READABLE,
    UDS_DID_NOT_WRITABLE,
    UDS_DID_SESSION_DENIED,
    UDS_DID_SECURITY_DENIED,
    UDS_DID_RESPONSE_TOO_LONG,
    UDS_DID_INVALID_WRITE,
    UDS_DID_ARGUMENT_ERROR
} UdsDidResult;

typedef struct {
    const uint8_t *data;
    uint16_t length;
} UdsDidValue;

typedef UdsDidResult (*UdsDidReadFn)(void *context, uint16_t did, uint8_t *data, uint16_t *length,
                                     uint16_t capacity);
typedef UdsDidResult (*UdsDidWriteFn)(void *context, uint16_t did, const uint8_t *data,
                                      uint16_t length);

typedef struct {
    uint16_t did;
    bool read_allowed;
    bool write_allowed;
    uint16_t maximum_length;
    uint8_t session_mask;
    uint8_t minimum_security_level;
    UdsDidReadFn read;
    UdsDidWriteFn write;
    void *context;
} UdsDidEntry;

/* These fields are views into authoritative application state. The registry
 * copies only the descriptors, never the pointed-to value bytes. */
typedef struct {
    UdsDidValue software_version;
    UdsDidValue hardware_version;
    UdsDidValue bootloader_version;
    UdsDidValue serial_number;
    UdsDidValue device_identity;
    UdsDidValue controller_id;
    UdsDidValue can_bitrate;
    UdsDidValue diagnostic_status;
    UdsDidValue firmware_status;
    UdsDidValue reset_cause;
    UdsDidValue watchdog_status;
    UdsDidValue can_error_counters;
    UdsDidValue network_error_state;
} UdsProjectDidSource;

typedef struct {
    UdsDidEntry entries[UDS_PROJECT_DID_COUNT];
} UdsDidRegistry;

void uds_did_registry_init(UdsDidRegistry *registry, const UdsProjectDidSource *source);
const UdsDidEntry *uds_did_registry_find(const UdsDidRegistry *registry, uint16_t did);
UdsDidResult uds_did_registry_read(const UdsDidRegistry *registry, uint16_t did, uint8_t session,
                                   uint8_t security_level, uint8_t *data, uint16_t *length,
                                   uint16_t capacity);
UdsDidResult uds_did_registry_write(const UdsDidRegistry *registry, uint16_t did, uint8_t session,
                                    uint8_t security_level, const uint8_t *data, uint16_t length);

#endif
