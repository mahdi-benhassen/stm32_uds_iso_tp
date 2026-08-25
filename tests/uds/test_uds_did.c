/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "uds_did.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    static const uint8_t software[] = "1.2.3";
    static const uint8_t serial[] = "SN-001";
    UdsProjectDidSource source = {
        .software_version = {software, (uint16_t)(sizeof(software) - 1U)},
        .serial_number = {serial, (uint16_t)(sizeof(serial) - 1U)},
    };
    UdsDidRegistry registry = {0};
    uds_did_registry_init(&registry, &source);

    uint8_t value[16] = {0};
    uint16_t length = 0U;
    assert(uds_did_registry_find(&registry, UDS_DID_SOFTWARE_VERSION) != NULL);
    assert(uds_did_registry_read(&registry, UDS_DID_SOFTWARE_VERSION, 0x01U, 0U, value, &length,
                                 sizeof(value)) == UDS_DID_OK);
    assert(length == 5U && memcmp(value, software, length) == 0);

    assert(uds_did_registry_read(&registry, 0x1234U, 0x01U, 0U, value, &length, sizeof(value)) ==
           UDS_DID_NOT_FOUND);
    assert(uds_did_registry_read(&registry, UDS_DID_SERIAL_NUMBER, 0x02U, 0U, value, &length,
                                 sizeof(value)) == UDS_DID_SECURITY_DENIED);
    assert(uds_did_registry_read(&registry, UDS_DID_SOFTWARE_VERSION, 0x99U, 0U, value, &length,
                                 sizeof(value)) == UDS_DID_SESSION_DENIED);
    assert(uds_did_registry_read(&registry, UDS_DID_SOFTWARE_VERSION, 0x01U, 0U, value, &length,
                                 2U) == UDS_DID_RESPONSE_TOO_LONG);
    assert(uds_did_registry_write(&registry, UDS_DID_SOFTWARE_VERSION, 0x01U, 0U, software, 5U) ==
           UDS_DID_NOT_WRITABLE);
    return 0;
}
