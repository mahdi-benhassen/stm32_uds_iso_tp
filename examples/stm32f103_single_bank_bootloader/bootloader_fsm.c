/**
 * @file bootloader_fsm.c
 * @brief STM32F103 Single-Bank UDS Bootloader FSM Implementation (Issue #140)
 */

#include "bootloader_fsm.h"
#include "crc32.h"
#include <string.h>

static boot_state_t s_boot_state = BOOT_STATE_STARTUP;
static uint32_t s_window_remaining_ms = BOOTLOADER_WINDOW_MS;

void bootloader_init(void) {
    s_boot_state = BOOT_STATE_WINDOW_ACTIVE;
    s_window_remaining_ms = BOOTLOADER_WINDOW_MS;
}

boot_state_t bootloader_get_state(void) {
    return s_boot_state;
}

uint32_t bootloader_get_remaining_window_ms(void) {
    return s_window_remaining_ms;
}

void bootloader_on_session_request(uint8_t session_type) {
    /* ISO 14229-1: Sub-function 0x02 is ProgrammingSession */
    if (session_type == 0x02U) {
        s_boot_state = BOOT_STATE_PROGRAMMING;
        s_window_remaining_ms = 0U;
    }
}

bool bootloader_verify_app(const uint8_t *app_flash_ptr, const app_metadata_t *meta) {
    if ((app_flash_ptr == NULL) || (meta == NULL)) {
        return false;
    }

    /* 1. Magic number validation: Written as the very last step in flashing */
    if (meta->magic != APP_METADATA_MAGIC) {
        return false;
    }

    /* 2. Bounds check against physical application slot size (60 KB) */
    if ((meta->app_size == 0U) || (meta->app_size > APP_SLOT_A_SIZE)) {
        return false;
    }

    /* 3. ARM Cortex-M initial Stack Pointer validity check (must point inside SRAM 0x20000000) */
    uint32_t initial_msp = (uint32_t)(
        ((uint32_t)app_flash_ptr[3] << 24U) |
        ((uint32_t)app_flash_ptr[2] << 16U) |
        ((uint32_t)app_flash_ptr[1] << 8U)  |
        ((uint32_t)app_flash_ptr[0])
    );
    if ((initial_msp & 0x2FFE0000U) != 0x20000000U) {
        return false;
    }

    /* 4. Reset vector check (must point into application code slot) */
    uint32_t reset_handler = (uint32_t)(
        ((uint32_t)app_flash_ptr[7] << 24U) |
        ((uint32_t)app_flash_ptr[6] << 16U) |
        ((uint32_t)app_flash_ptr[5] << 8U)  |
        ((uint32_t)app_flash_ptr[4])
    );
    if ((reset_handler < APP_SLOT_A_START_ADDR) ||
        (reset_handler >= (APP_SLOT_A_START_ADDR + meta->app_size))) {
        return false;
    }

    /* 5. Complete IEEE 802.3 CRC32 verification over flash binary */
    uint32_t computed_crc = bootloader_crc32(app_flash_ptr, meta->app_size);
    if (computed_crc != meta->crc32) {
        return false;
    }

    return true;
}

bool bootloader_commit_metadata(app_metadata_t *meta_storage, uint32_t app_size,
                                uint32_t crc32, uint32_t entry_point, uint32_t version) {
    if (meta_storage == NULL) {
        return false;
    }

    /* Write parameters first; magic word is preserved as invalid until other fields succeed */
    meta_storage->magic = 0xFFFFFFFFU;
    meta_storage->app_size = app_size;
    meta_storage->crc32 = crc32;
    meta_storage->entry_point = entry_point;
    meta_storage->version = version;
    meta_storage->reserved = 0U;

    /* ATOMIC COMMIT: Magic written LAST. Power loss mid-flash leaves magic 0xFFFFFFFF */
    meta_storage->magic = APP_METADATA_MAGIC;
    return true;
}

bool bootloader_poll(uint32_t elapsed_ms) {
    if (s_boot_state == BOOT_STATE_WINDOW_ACTIVE) {
        if (elapsed_ms >= s_window_remaining_ms) {
            s_window_remaining_ms = 0U;
            s_boot_state = BOOT_STATE_VERIFYING;

            /* Check application and metadata */
            const uint8_t *app_ptr = (const uint8_t *)APP_SLOT_A_START_ADDR;
            const app_metadata_t *meta_ptr = (const app_metadata_t *)METADATA_A_START_ADDR;

            if (bootloader_verify_app(app_ptr, meta_ptr)) {
                s_boot_state = BOOT_STATE_JUMP_READY;
                return true;
            } else {
                /* Verification failed: stay in recovery mode to allow UDS re-flashing! */
                s_boot_state = BOOT_STATE_RECOVERY;
                return false;
            }
        } else {
            s_window_remaining_ms -= elapsed_ms;
        }
    }
    return (s_boot_state == BOOT_STATE_JUMP_READY);
}
