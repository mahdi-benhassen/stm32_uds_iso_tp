/**
 * @file svc_authentication.h
 * @brief UDS Service 0x29 Authentication (ISO 14229-1:2020)
 * @details Implements APCE (Asymmetric/Symmetric Proof of Ownership) authentication flow with AES-CMAC-128.
 */

#ifndef SVC_AUTHENTICATION_H
#define SVC_AUTHENTICATION_H

#include "uds_types.h"
#include <stdint.h>
#include <stdbool.h>

#define UDS_AUTH_SUB_DEAUTHENTICATE                 0x01U
#define UDS_AUTH_SUB_VERIFY_CERT_UNIDIR             0x02U
#define UDS_AUTH_SUB_PROOF_OF_OWNERSHIP             0x04U
#define UDS_AUTH_SUB_TRANSMIT_CERT                  0x05U
#define UDS_AUTH_SUB_REQUEST_CHALLENGE              0x06U
#define UDS_AUTH_SUB_VERIFY_PROOF_UNIDIR            0x07U

#define UDS_AUTH_CHALLENGE_LEN                      16U
#define UDS_AUTH_PROOF_LEN                          16U

#ifdef __cplusplus
extern "C" {
#endif

void svc_authentication(const uds_request_t *req, uds_response_t *resp);
void uds_auth_init(void);
void uds_auth_reset(void);
bool uds_auth_is_authenticated(void);
void uds_auth_set_master_key(const uint8_t key[16]);

#ifdef __cplusplus
}
#endif

#endif /* SVC_AUTHENTICATION_H */
