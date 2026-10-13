/**
 * @file uds_session.h
 * @brief UDS Session Management API (ISO 14229-1)
 */

#ifndef UDS_SESSION_H
#define UDS_SESSION_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UDS_SESSION_DEFAULT     = 0x01,
    UDS_SESSION_PROGRAMMING = 0x02,
    UDS_SESSION_EXTENDED    = 0x03,
    UDS_SESSION_ENGINEERING = 0x04
} uds_session_t;

uds_session_t uds_session_get(void);
void uds_session_set(uds_session_t session);
void uds_session_reset(void);
void uds_session_refresh(void);
bool uds_session_check_timeout(void);
uint8_t uds_session_to_mask(uds_session_t session);
void uds_state_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SESSION_H */
