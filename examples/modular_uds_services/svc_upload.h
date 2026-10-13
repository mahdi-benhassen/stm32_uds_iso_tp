/**
 * @file svc_upload.h
 * @brief UDS Service 0x35 RequestUpload
 */
#ifndef SVC_UPLOAD_H
#define SVC_UPLOAD_H

#include "uds_types.h"

void svc_request_upload(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_UPLOAD_H */
