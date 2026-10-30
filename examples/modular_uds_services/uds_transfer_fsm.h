/**
 * @file uds_transfer_fsm.h
 * @brief Common Transfer State Machine for UDS Download (0x34) and Upload (0x35)
 */

#ifndef UDS_TRANSFER_FSM_H
#define UDS_TRANSFER_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"

typedef enum {
    TRANSFER_IDLE = 0,
    TRANSFER_DOWNLOAD_ACTIVE,
    TRANSFER_UPLOAD_ACTIVE
} uds_transfer_state_t;

#define UDS_MAX_BLOCK_LENGTH 1026U

typedef struct {
    uds_transfer_state_t state;
    uint32_t             memory_address;
    uint32_t             memory_size;
    uint32_t             transferred_bytes;
    uint8_t              block_sequence_counter;
} uds_transfer_ctx_t;

#ifdef __cplusplus
extern "C" {
#endif

uds_transfer_ctx_t *uds_transfer_get_context(void);
void uds_transfer_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* UDS_TRANSFER_FSM_H */
