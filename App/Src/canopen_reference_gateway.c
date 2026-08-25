/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "canopen_reference_gateway.h"

#include "canopen_reference_config.h"

#if CANOPEN_REFERENCE_ENABLE_GATEWAY
#include "CO_gateway_ascii.h"

static CO_t *s_co;

__attribute__((weak)) bool
CANopenReferenceGateway_Authorized(void) {
    return false;
}

__attribute__((weak)) size_t
CANopenReferenceGateway_WriteResponse(const uint8_t *bytes, size_t length, bool *connection_ok) {
    (void)bytes;
    (void)length;
    if (connection_ok != NULL) {
        *connection_ok = false;
    }
    return 0U;
}

static size_t
CANopenReferenceGateway_ReadCallback(void *object, const char *buffer, size_t count, uint8_t *connection_ok) {
    bool connected = false;
    size_t written;
    (void)object;

    if (!CANopenReferenceGateway_Authorized()) {
        if (connection_ok != NULL) {
            *connection_ok = 0U;
        }
        return 0U;
    }
    written = CANopenReferenceGateway_WriteResponse((const uint8_t *)buffer, count, &connected);
    if (connection_ok != NULL) {
        *connection_ok = connected ? 1U : 0U;
    }
    return written;
}

void
CANopenReferenceGateway_Init(CO_t *co) {
    s_co = co;
    if (s_co != NULL && s_co->gtwa != NULL) {
        CO_GTWA_initRead(s_co->gtwa, CANopenReferenceGateway_ReadCallback, NULL);
    }
}

size_t
CANopenReferenceGateway_WriteCommand(const uint8_t *bytes, size_t length) {
    if (s_co == NULL || s_co->gtwa == NULL || bytes == NULL || length == 0U || !CANopenReferenceGateway_Authorized()) {
        return 0U;
    }
    return CO_GTWA_write(s_co->gtwa, (const char *)bytes, length);
}

#else

bool
CANopenReferenceGateway_Authorized(void) {
    return false;
}

size_t
CANopenReferenceGateway_WriteResponse(const uint8_t *bytes, size_t length, bool *connection_ok) {
    (void)bytes;
    (void)length;
    if (connection_ok != NULL) {
        *connection_ok = false;
    }
    return 0U;
}

void
CANopenReferenceGateway_Init(CO_t *co) {
    (void)co;
}

size_t
CANopenReferenceGateway_WriteCommand(const uint8_t *bytes, size_t length) {
    (void)bytes;
    (void)length;
    return 0U;
}

#endif
