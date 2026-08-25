/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "CO_gateway_ascii.h"
#include "canopen_reference_gateway.h"

void
CO_GTWA_initRead(CO_GTWA_t *gtwa, CO_GTWA_read_callback_t callback, void *object) {
    gtwa->read_callback = callback;
    gtwa->read_object = object;
}

size_t
CO_GTWA_write(CO_GTWA_t *gtwa, const char *buffer, size_t length) {
    (void)buffer;
    ++gtwa->write_calls;
    return length;
}

int
main(void) {
    CO_GTWA_t gateway = {0};
    CO_t canopen = {.gtwa = &gateway};
    const uint8_t command[] = {'r', ' ', '1', '0', '0', '0'};
    const char response[] = "1000=0\r\n";
    uint8_t connection_ok = 1U;
    bool transport_ok = true;

    assert(!CANopenReferenceGateway_Authorized());
    CANopenReferenceGateway_Init(&canopen);
    assert(gateway.read_callback != NULL);
    assert(CANopenReferenceGateway_WriteCommand(command, sizeof(command)) == 0U);
    assert(gateway.write_calls == 0U);

    assert(gateway.read_callback(gateway.read_object, response, sizeof(response) - 1U, &connection_ok) == 0U);
    assert(connection_ok == 0U);
    assert(gateway.write_calls == 0U);

    assert(CANopenReferenceGateway_WriteResponse(command, sizeof(command), &transport_ok) == 0U);
    assert(!transport_ok);
    return 0;
}
