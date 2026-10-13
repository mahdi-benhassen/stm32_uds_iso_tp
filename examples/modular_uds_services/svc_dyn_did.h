/**
 * @file svc_dyn_did.h
 * @brief UDS Service 0x2C DynamicallyDefineDataIdentifier
 */
#ifndef SVC_DYN_DID_H
#define SVC_DYN_DID_H

#include "uds_types.h"

void svc_dyn_define_did(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_DYN_DID_H */
