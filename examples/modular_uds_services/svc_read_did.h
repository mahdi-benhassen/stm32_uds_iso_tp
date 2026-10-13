/**
 * @file svc_read_did.h
 * @brief UDS Service 0x22 ReadDataByIdentifier
 */
#ifndef SVC_READ_DID_H
#define SVC_READ_DID_H

#include "uds_types.h"

void svc_read_data_by_id(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_READ_DID_H */
