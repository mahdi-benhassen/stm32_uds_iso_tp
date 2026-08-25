/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "can_port.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t received_id;
static uint8_t received_data[CAN_PORT_MAX_DLC];
static uint8_t received_len;

static void
on_frame(uint32_t id, uint8_t *data, uint8_t len) {
    received_id = id;
    received_len = len;
    (void)memcpy(received_data, data, len);
}

int
main(void) {
    uint8_t payload[] = {0xCA, 0xFE, 0x0A};
    int result;

    result = can_port_init(500000U);
    if (result != 0) {
        (void)fprintf(stderr, "can_port_init failed: %d\n", result);
        return 1;
    }
    can_port_register_rx(on_frame);
    result = can_port_send(0x555U, payload, (uint8_t)sizeof(payload));
    if (result != 0) {
        (void)fprintf(stderr, "can_port_send failed: %d\n", result);
        can_port_deinit();
        return 2;
    }
    result = can_port_poll(1000U);
    can_port_deinit();
    if (result != 1 || received_id != 0x555U || received_len != sizeof(payload)
        || memcmp(received_data, payload, sizeof(payload)) != 0) {
        (void)fprintf(stderr, "can_port loopback validation failed\n");
        return 3;
    }
    (void)puts("SocketCAN can_port loopback passed.");
    return 0;
}
