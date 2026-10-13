/**
 * @file svc_link_ctrl.h
 * @brief UDS Service 0x87 LinkControl
 */
#ifndef SVC_LINK_CTRL_H
#define SVC_LINK_CTRL_H

#include "uds_types.h"

void svc_link_control(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_LINK_CTRL_H */
