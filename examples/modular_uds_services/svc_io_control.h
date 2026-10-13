/**
 * @file svc_io_control.h
 * @brief UDS Service 0x2F InputOutputControlByIdentifier
 */
#ifndef SVC_IO_CONTROL_H
#define SVC_IO_CONTROL_H

#include "uds_types.h"

void svc_io_control(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_IO_CONTROL_H */
