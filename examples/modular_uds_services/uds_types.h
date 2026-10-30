/**
 * @file uds_types.h
 * @brief Common UDS types and Negative Response Codes (ISO 14229-1)
 */

#ifndef UDS_TYPES_H
#define UDS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef ISOTP_TX_BUF_SIZE
#define ISOTP_TX_BUF_SIZE 4095U
#endif

#define UDS_SID_NEGATIVE_RESPONSE            0x7FU
#define UDS_POSITIVE_RESPONSE(sid)           ((uint8_t)((sid) + 0x40U))

/* Negative Response Codes (NRC) */
#define NRC_GENERAL_REJECT                   0x10U
#define NRC_SERVICE_NOT_SUPPORTED            0x11U
#define NRC_SUBFUNCTION_NOT_SUPPORTED        0x12U
#define NRC_INCORRECT_MSG_LEN_OR_FORMAT      0x13U
#define NRC_RESPONSE_TOO_LONG                0x14U
#define NRC_BUSY_REPEAT_REQUEST              0x21U
#define NRC_CONDITIONS_NOT_CORRECT           0x22U
#define NRC_REQUEST_SEQUENCE_ERROR           0x24U
#define NRC_REQUEST_OUT_OF_RANGE             0x31U
#define NRC_SECURITY_ACCESS_DENIED           0x33U
#define NRC_INVALID_KEY                      0x35U
#define NRC_EXCEED_NUMBER_OF_ATTEMPTS        0x36U
#define NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED  0x37U
#define NRC_WRONG_BLOCK_SEQUENCE_COUNTER     0x73U
#define NRC_SERVICE_NOT_SUPPORTED_IN_SESSION 0x7FU

/* Session Bitmasks */
#define SESSION_MASK_DEFAULT                 (1U << 0)
#define SESSION_MASK_PROGRAMMING             (1U << 1)
#define SESSION_MASK_EXTENDED                (1U << 2)
#define SESSION_MASK_ENGINEERING             (1U << 3)
#define SESSION_MASK_ALL                     0xFFU
#define SESSION_MASK_NON_DEFAULT             (SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING)

typedef struct {
    uint8_t sid;
    const uint8_t *data;
    uint16_t data_len;
    bool functional;
} uds_request_t;

typedef struct {
    uint8_t data[ISOTP_TX_BUF_SIZE];
    uint16_t len;
    bool suppress;
} uds_response_t;

typedef void (*uds_service_handler_t)(const uds_request_t *req, uds_response_t *resp);

#endif /* UDS_TYPES_H */
