/**
 * @file svc_tester_present.h
 * @brief UDS Service 0x3E TesterPresent
 */
#ifndef SVC_TESTER_PRESENT_H
#define SVC_TESTER_PRESENT_H

#include "uds_types.h"

void svc_tester_present(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_TESTER_PRESENT_H */
