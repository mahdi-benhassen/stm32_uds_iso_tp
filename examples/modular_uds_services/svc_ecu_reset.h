/**
 * @file svc_ecu_reset.h
 * @brief UDS Service 0x11 ECUReset
 */
#ifndef SVC_ECU_RESET_H
#define SVC_ECU_RESET_H

#include "uds_types.h"

void svc_ecu_reset(const uds_request_t *req, uds_response_t *resp);
void uds_reset_poll(void);

#endif /* SVC_ECU_RESET_H */
