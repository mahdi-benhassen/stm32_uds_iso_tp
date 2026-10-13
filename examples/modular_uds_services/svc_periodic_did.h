/**
 * @file svc_periodic_did.h
 * @brief UDS Service 0x2A ReadDataByPeriodicIdentifier
 */
#ifndef SVC_PERIODIC_DID_H
#define SVC_PERIODIC_DID_H

#include "uds_types.h"

void svc_read_periodic_id(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_PERIODIC_DID_H */
