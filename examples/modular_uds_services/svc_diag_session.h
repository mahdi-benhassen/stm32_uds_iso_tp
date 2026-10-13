/**
 * @file svc_diag_session.h
 * @brief UDS Service 0x10 DiagnosticSessionControl
 */
#ifndef SVC_DIAG_SESSION_H
#define SVC_DIAG_SESSION_H

#include "uds_types.h"

void svc_diag_session_control(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_DIAG_SESSION_H */
