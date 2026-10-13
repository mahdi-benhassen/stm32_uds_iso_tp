/**
 * @file svc_write_did.h
 * @brief UDS Service 0x2E WriteDataByIdentifier
 */
#ifndef SVC_WRITE_DID_H
#define SVC_WRITE_DID_H

#include "uds_types.h"

void svc_write_data_by_id(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_WRITE_DID_H */
