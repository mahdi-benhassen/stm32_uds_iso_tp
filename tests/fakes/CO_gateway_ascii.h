/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef TEST_FAKE_CO_GATEWAY_ASCII_H
#define TEST_FAKE_CO_GATEWAY_ASCII_H

#include <stddef.h>
#include <stdint.h>

typedef size_t (*CO_GTWA_read_callback_t)(void *object,
                                          const char *buffer,
                                          size_t count,
                                          uint8_t *connection_ok);

typedef struct CO_GTWA_t {
    CO_GTWA_read_callback_t read_callback;
    void *read_object;
    size_t write_calls;
} CO_GTWA_t;

struct CO_t {
    CO_GTWA_t *gtwa;
};

void CO_GTWA_initRead(CO_GTWA_t *gtwa, CO_GTWA_read_callback_t callback, void *object);
size_t CO_GTWA_write(CO_GTWA_t *gtwa, const char *buffer, size_t length);

#endif /* TEST_FAKE_CO_GATEWAY_ASCII_H */
