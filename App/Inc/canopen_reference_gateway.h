/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_REFERENCE_GATEWAY_H
#define CANOPEN_REFERENCE_GATEWAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canopen_reference_co.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Bind the ASCII gateway response stream after CO_CANopenInit() succeeds. */
void CANopenReferenceGateway_Init(CO_t *co);

/** Write ASCII CiA 309-3 command bytes from a non-interrupt UART/USB worker. */
size_t CANopenReferenceGateway_WriteCommand(const uint8_t *bytes, size_t length);

/** Board policy hook: diagnostic gateway access is denied by default. */
bool CANopenReferenceGateway_Authorized(void);

/** Board transport hook for bounded ASCII response output from mainline. */
size_t CANopenReferenceGateway_WriteResponse(const uint8_t *bytes, size_t length, bool *connection_ok);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_REFERENCE_GATEWAY_H */
