#include "fota_client.h"
#include <string.h>

void fota_client_init(fota_client_t *client, uint32_t target_address,
                      const uint8_t *image, uint32_t image_size)
{
    if (client == NULL) {
        return;
    }
    memset(client, 0, sizeof(*client));
    client->current_step = FOTA_STEP_DIAG_SESSION_EXT;
    client->flash_address = target_address;
    client->firmware_image = image;
    client->flash_size = image_size;
    client->block_counter = 1U;
}

fota_step_t fota_client_get_step(const fota_client_t *client)
{
    return client ? client->current_step : FOTA_STEP_FAILED;
}

bool fota_client_build_next_request(fota_client_t *client, uint8_t *buf,
                                    uint16_t *len, uint16_t capacity)
{
    if (client == NULL || buf == NULL || len == NULL) {
        return false;
    }

    switch (client->current_step) {
    case FOTA_STEP_DIAG_SESSION_EXT:
        /* 0x10 0x03 - Extended Diagnostic Session (required precursor to Programming) */
        if (capacity < 2U) return false;
        buf[0] = 0x10U;
        buf[1] = 0x03U;
        *len = 2U;
        return true;

    case FOTA_STEP_DIAG_SESSION_PROG:
        /* 0x10 0x02 - Programming Session */
        if (capacity < 2U) return false;
        buf[0] = 0x10U;
        buf[1] = 0x02U;
        *len = 2U;
        return true;

    case FOTA_STEP_DISABLE_DTC:
        /* 0x85 0x02 - ControlDTCSetting (OFF) */
        if (capacity < 2U) return false;
        buf[0] = 0x85U;
        buf[1] = 0x02U;
        *len = 2U;
        return true;

    case FOTA_STEP_COMM_CONTROL:
        /* 0x28 0x03 0x01 - CommunicationControl: disableRxAndTx, normalCommunicationMessages */
        if (capacity < 3U) return false;
        buf[0] = 0x28U;
        buf[1] = 0x03U;
        buf[2] = 0x01U;
        *len = 3U;
        return true;

    case FOTA_STEP_SECURITY_SEED:
        /* 0x27 0x01 - SecurityAccess requestSeed Level 1 */
        if (capacity < 2U) return false;
        buf[0] = 0x27U;
        buf[1] = 0x01U;
        *len = 2U;
        return true;

    case FOTA_STEP_SECURITY_KEY: {
        /* 0x27 0x02 - sendKey Level 1 (key = seed ^ 0xDEAD1234) */
        if (capacity < 6U) return false;
        uint32_t key = client->seed_received ^ 0xDEAD1234U;
        buf[0] = 0x27U;
        buf[1] = 0x02U;
        buf[2] = (uint8_t)((key >> 24) & 0xFFU);
        buf[3] = (uint8_t)((key >> 16) & 0xFFU);
        buf[4] = (uint8_t)((key >> 8) & 0xFFU);
        buf[5] = (uint8_t)(key & 0xFFU);
        *len = 6U;
        return true;
    }

    case FOTA_STEP_ERASE_MEMORY:
        /* 0x31 0x01 0xFF 0x00 - RoutineControl EraseMemory (Address + Size) */
        if (capacity < 12U) return false;
        buf[0] = 0x31U;
        buf[1] = 0x01U;
        buf[2] = 0xFFU;
        buf[3] = 0x00U;
        /* 4-byte address */
        buf[4] = (uint8_t)((client->flash_address >> 24) & 0xFFU);
        buf[5] = (uint8_t)((client->flash_address >> 16) & 0xFFU);
        buf[6] = (uint8_t)((client->flash_address >> 8) & 0xFFU);
        buf[7] = (uint8_t)(client->flash_address & 0xFFU);
        /* 4-byte size */
        buf[8] = (uint8_t)((client->flash_size >> 24) & 0xFFU);
        buf[9] = (uint8_t)((client->flash_size >> 16) & 0xFFU);
        buf[10] = (uint8_t)((client->flash_size >> 8) & 0xFFU);
        buf[11] = (uint8_t)(client->flash_size & 0xFFU);
        *len = 12U;
        return true;

    case FOTA_STEP_REQUEST_DOWNLOAD:
        /* 0x34 - RequestDownload: dataFormatIdentifier=0x00, addressAndLengthFormatIdentifier=0x44 */
        if (capacity < 11U) return false;
        buf[0] = 0x34U;
        buf[1] = 0x00U;
        buf[2] = 0x44U;
        buf[3] = (uint8_t)((client->flash_address >> 24) & 0xFFU);
        buf[4] = (uint8_t)((client->flash_address >> 16) & 0xFFU);
        buf[5] = (uint8_t)((client->flash_address >> 8) & 0xFFU);
        buf[6] = (uint8_t)(client->flash_address & 0xFFU);
        buf[7] = (uint8_t)((client->flash_size >> 24) & 0xFFU);
        buf[8] = (uint8_t)((client->flash_size >> 16) & 0xFFU);
        buf[9] = (uint8_t)((client->flash_size >> 8) & 0xFFU);
        buf[10] = (uint8_t)(client->flash_size & 0xFFU);
        *len = 11U;
        return true;

    case FOTA_STEP_TRANSFER_DATA: {
        /* 0x36 - TransferData: blockSequenceCounter + up to 128 bytes payload chunk */
        uint32_t remaining = client->flash_size - client->firmware_bytes_transferred;
        uint16_t chunk = (remaining > 128U) ? 128U : (uint16_t)remaining;
        if (capacity < 2U + chunk) return false;

        buf[0] = 0x36U;
        buf[1] = client->block_counter;
        if (client->firmware_image != NULL && chunk > 0U) {
            memcpy(&buf[2], &client->firmware_image[client->firmware_bytes_transferred], chunk);
        }
        *len = (uint16_t)(2U + chunk);
        return true;
    }

    case FOTA_STEP_TRANSFER_EXIT:
        /* 0x37 - RequestTransferExit */
        if (capacity < 1U) return false;
        buf[0] = 0x37U;
        *len = 1U;
        return true;

    case FOTA_STEP_VERIFY_DEPENDENCY:
        /* 0x31 0x01 0x02 0x02 - RoutineControl CheckProgrammingDependencies */
        if (capacity < 4U) return false;
        buf[0] = 0x31U;
        buf[1] = 0x01U;
        buf[2] = 0x02U;
        buf[3] = 0x02U;
        *len = 4U;
        return true;

    case FOTA_STEP_ECU_RESET:
        /* 0x11 0x01 - ECUReset (HardReset) */
        if (capacity < 2U) return false;
        buf[0] = 0x11U;
        buf[1] = 0x01U;
        *len = 2U;
        return true;

    default:
        return false;
    }
}

bool fota_client_handle_response(fota_client_t *client, const uint8_t *buf,
                                 uint16_t len)
{
    if (client == NULL || buf == NULL || len == 0U) {
        return false;
    }

    /* Check for Negative Response */
    if (buf[0] == 0x7FU) {
        client->current_step = FOTA_STEP_FAILED;
        return false;
    }

    switch (client->current_step) {
    case FOTA_STEP_DIAG_SESSION_EXT:
        if (buf[0] == 0x50U && len >= 2U && buf[1] == 0x03U) {
            client->current_step = FOTA_STEP_DIAG_SESSION_PROG;
            return true;
        }
        break;

    case FOTA_STEP_DIAG_SESSION_PROG:
        if (buf[0] == 0x50U && len >= 2U && buf[1] == 0x02U) {
            client->current_step = FOTA_STEP_DISABLE_DTC;
            return true;
        }
        break;

    case FOTA_STEP_DISABLE_DTC:
        if (buf[0] == 0xC5U) {
            client->current_step = FOTA_STEP_COMM_CONTROL;
            return true;
        }
        break;

    case FOTA_STEP_COMM_CONTROL:
        if (buf[0] == 0x68U) {
            client->current_step = FOTA_STEP_SECURITY_SEED;
            return true;
        }
        break;

    case FOTA_STEP_SECURITY_SEED:
        if (buf[0] == 0x67U && buf[1] == 0x01U && len >= 6U) {
            client->seed_received = ((uint32_t)buf[2] << 24) |
                                    ((uint32_t)buf[3] << 16) |
                                    ((uint32_t)buf[4] << 8)  |
                                    (uint32_t)buf[5];
            client->current_step = FOTA_STEP_SECURITY_KEY;
            return true;
        }
        break;

    case FOTA_STEP_SECURITY_KEY:
        if (buf[0] == 0x67U && buf[1] == 0x02U) {
            client->current_step = FOTA_STEP_ERASE_MEMORY;
            return true;
        }
        break;

    case FOTA_STEP_ERASE_MEMORY:
        if (buf[0] == 0x71U && buf[1] == 0x01U) {
            client->current_step = FOTA_STEP_REQUEST_DOWNLOAD;
            return true;
        }
        break;

    case FOTA_STEP_REQUEST_DOWNLOAD:
        if (buf[0] == 0x74U) {
            client->block_counter = 1U;
            client->firmware_bytes_transferred = 0U;
            client->current_step = FOTA_STEP_TRANSFER_DATA;
            return true;
        }
        break;

    case FOTA_STEP_TRANSFER_DATA:
        if (buf[0] == 0x76U && len >= 2U && buf[1] == client->block_counter) {
            uint32_t remaining = client->flash_size - client->firmware_bytes_transferred;
            uint16_t chunk = (remaining > 128U) ? 128U : (uint16_t)remaining;
            client->firmware_bytes_transferred += chunk;
            client->block_counter++;

            if (client->firmware_bytes_transferred >= client->flash_size) {
                client->current_step = FOTA_STEP_TRANSFER_EXIT;
            }
            return true;
        }
        break;

    case FOTA_STEP_TRANSFER_EXIT:
        if (buf[0] == 0x77U) {
            client->current_step = FOTA_STEP_VERIFY_DEPENDENCY;
            return true;
        }
        break;

    case FOTA_STEP_VERIFY_DEPENDENCY:
        if (buf[0] == 0x71U && buf[1] == 0x01U) {
            client->current_step = FOTA_STEP_ECU_RESET;
            return true;
        }
        break;

    case FOTA_STEP_ECU_RESET:
        if (buf[0] == 0x51U) {
            client->current_step = FOTA_STEP_COMPLETED;
            return true;
        }
        break;

    default:
        break;
    }

    client->current_step = FOTA_STEP_FAILED;
    return false;
}
