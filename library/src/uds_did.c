/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds_did.h"

#include <stddef.h>
#include <string.h>

static UdsDidResult read_value(void *context, uint16_t did, uint8_t *data, uint16_t *length,
                               uint16_t capacity) {
    const UdsDidValue *value = (const UdsDidValue *)context;
    (void)did;
    if ((value == NULL) || (data == NULL) || (length == NULL)) {
        return UDS_DID_ARGUMENT_ERROR;
    }
    if (value->length > capacity) {
        return UDS_DID_RESPONSE_TOO_LONG;
    }
    if ((value->length != 0U) && (value->data == NULL)) {
        return UDS_DID_ARGUMENT_ERROR;
    }
    if (value->length != 0U) {
        (void)memcpy(data, value->data, value->length);
    }
    *length = value->length;
    return UDS_DID_OK;
}

static void set_entry(UdsDidEntry *entry, uint16_t did, const UdsDidValue *value,
                      uint16_t maximum_length, uint8_t session_mask,
                      uint8_t minimum_security_level) {
    entry->did = did;
    entry->read_allowed = true;
    entry->write_allowed = false;
    entry->maximum_length = maximum_length;
    entry->session_mask = session_mask;
    entry->minimum_security_level = minimum_security_level;
    entry->read = read_value;
    entry->write = NULL;
    entry->context = (void *)value;
}

void uds_did_registry_init(UdsDidRegistry *registry, const UdsProjectDidSource *source) {
    if ((registry == NULL) || (source == NULL)) {
        return;
    }
    set_entry(&registry->entries[0], UDS_DID_SOFTWARE_VERSION, &source->software_version, 64U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[1], UDS_DID_HARDWARE_VERSION, &source->hardware_version, 64U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[2], UDS_DID_BOOTLOADER_VERSION, &source->bootloader_version, 64U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[3], UDS_DID_SERIAL_NUMBER, &source->serial_number, 32U,
              UDS_DID_SESSION_ALL_MASK, 1U);
    set_entry(&registry->entries[4], UDS_DID_DEVICE_IDENTITY, &source->device_identity, 64U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[5], UDS_DID_CONTROLLER_ID, &source->controller_id, 1U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[6], UDS_DID_CAN_BITRATE, &source->can_bitrate, 4U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[7], UDS_DID_DIAGNOSTIC_STATUS, &source->diagnostic_status, 64U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[8], UDS_DID_FIRMWARE_STATUS, &source->firmware_status, 64U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[9], UDS_DID_RESET_CAUSE, &source->reset_cause, 4U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[10], UDS_DID_WATCHDOG_STATUS, &source->watchdog_status, 64U,
              UDS_DID_SESSION_ALL_MASK, 1U);
    set_entry(&registry->entries[11], UDS_DID_CAN_ERROR_COUNTERS, &source->can_error_counters, 32U,
              UDS_DID_SESSION_ALL_MASK, 0U);
    set_entry(&registry->entries[12], UDS_DID_NETWORK_ERROR_STATE, &source->network_error_state, 4U,
              UDS_DID_SESSION_ALL_MASK, 0U);
}

const UdsDidEntry *uds_did_registry_find(const UdsDidRegistry *registry, uint16_t did) {
    if (registry == NULL) {
        return NULL;
    }
    for (uint32_t index = 0U; index < UDS_PROJECT_DID_COUNT; ++index) {
        if (registry->entries[index].did == did) {
            return &registry->entries[index];
        }
    }
    return NULL;
}

static bool session_allowed(const UdsDidEntry *entry, uint8_t session) {
    uint8_t mask = 0U;
    if (session == 0x01U) {
        mask = UDS_DID_SESSION_DEFAULT_MASK;
    } else if (session == 0x02U) {
        mask = UDS_DID_SESSION_PROGRAMMING_MASK;
    } else if (session == 0x03U) {
        mask = UDS_DID_SESSION_EXTENDED_MASK;
    }
    return (mask != 0U) && ((entry->session_mask & mask) != 0U);
}

UdsDidResult uds_did_registry_read(const UdsDidRegistry *registry, uint16_t did, uint8_t session,
                                   uint8_t security_level, uint8_t *data, uint16_t *length,
                                   uint16_t capacity) {
    const UdsDidEntry *entry = uds_did_registry_find(registry, did);
    if (entry == NULL) {
        return UDS_DID_NOT_FOUND;
    }
    if (!entry->read_allowed || (entry->read == NULL)) {
        return UDS_DID_NOT_READABLE;
    }
    if (!session_allowed(entry, session)) {
        return UDS_DID_SESSION_DENIED;
    }
    if (security_level < entry->minimum_security_level) {
        return UDS_DID_SECURITY_DENIED;
    }
    UdsDidResult result = entry->read(entry->context, did, data, length, capacity);
    if ((result == UDS_DID_OK) && (*length > entry->maximum_length)) {
        return UDS_DID_RESPONSE_TOO_LONG;
    }
    return result;
}

UdsDidResult uds_did_registry_write(const UdsDidRegistry *registry, uint16_t did, uint8_t session,
                                    uint8_t security_level, const uint8_t *data, uint16_t length) {
    const UdsDidEntry *entry = uds_did_registry_find(registry, did);
    if (entry == NULL) {
        return UDS_DID_NOT_FOUND;
    }
    if (!entry->write_allowed || (entry->write == NULL)) {
        return UDS_DID_NOT_WRITABLE;
    }
    if (!session_allowed(entry, session)) {
        return UDS_DID_SESSION_DENIED;
    }
    if (security_level < entry->minimum_security_level) {
        return UDS_DID_SECURITY_DENIED;
    }
    if (length > entry->maximum_length) {
        return UDS_DID_INVALID_WRITE;
    }
    return entry->write(entry->context, did, data, length);
}
