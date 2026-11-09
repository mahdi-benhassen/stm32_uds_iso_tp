#ifndef FOTA_CLIENT_H
#define FOTA_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_iso_tp/isotp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FOTA_STEP_IDLE = 0,
    FOTA_STEP_DIAG_SESSION_EXT,
    FOTA_STEP_DIAG_SESSION_PROG,
    FOTA_STEP_DISABLE_DTC,
    FOTA_STEP_COMM_CONTROL,
    FOTA_STEP_SECURITY_SEED,
    FOTA_STEP_SECURITY_KEY,
    FOTA_STEP_ERASE_MEMORY,
    FOTA_STEP_REQUEST_DOWNLOAD,
    FOTA_STEP_TRANSFER_DATA,
    FOTA_STEP_TRANSFER_EXIT,
    FOTA_STEP_VERIFY_DEPENDENCY,
    FOTA_STEP_ECU_RESET,
    FOTA_STEP_COMPLETED,
    FOTA_STEP_FAILED
} fota_step_t;

typedef struct {
    fota_step_t current_step;
    uint32_t flash_address;
    uint32_t flash_size;
    const uint8_t *firmware_image;
    uint32_t firmware_bytes_transferred;
    uint8_t block_counter;
    uint32_t seed_received;
    bool step_pending;
    uint32_t step_timeout_ms;
} fota_client_t;

void fota_client_init(fota_client_t *client, uint32_t target_address,
                      const uint8_t *image, uint32_t image_size);
fota_step_t fota_client_get_step(const fota_client_t *client);
bool fota_client_build_next_request(fota_client_t *client, uint8_t *buf,
                                    uint16_t *len, uint16_t capacity);
bool fota_client_handle_response(fota_client_t *client, const uint8_t *buf,
                                 uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* FOTA_CLIENT_H */
