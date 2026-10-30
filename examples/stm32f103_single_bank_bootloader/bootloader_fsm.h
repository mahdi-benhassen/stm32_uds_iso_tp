/**
 * @file bootloader_fsm.h
 * @brief STM32F103 Single-Bank UDS Bootloader FSM & Anti-Bricking Logic
 */

#ifndef STM32F103_BOOTLOADER_FSM_H
#define STM32F103_BOOTLOADER_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "flash_layout.h"
#include "app_metadata.h"

typedef enum {
    BOOT_STATE_STARTUP = 0,
    BOOT_STATE_WINDOW_ACTIVE,
    BOOT_STATE_PROGRAMMING,
    BOOT_STATE_VERIFYING,
    BOOT_STATE_JUMP_READY,
    BOOT_STATE_RECOVERY
} boot_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize bootloader state machine and arm the 2.0s diagnostic window
 */
void bootloader_init(void);

/**
 * @brief Hook invoked when a UDS session change request is received (Service 0x10)
 * @param session_type UDS session subfunction (e.g. 0x02 ProgrammingSession)
 */
void bootloader_on_session_request(uint8_t session_type);

/**
 * @brief Periodic poll handler for bootloader timer and state transitions
 * @param elapsed_ms Elapsed milliseconds since previous call
 * @return true if ready to jump to application, false if staying in bootloader
 */
bool bootloader_poll(uint32_t elapsed_ms);

/**
 * @brief Verify application integrity in Slot A against metadata
 * @param app_flash_ptr Pointer to application flash base (0x0800C000)
 * @param meta Pointer to metadata block (0x0801B000)
 * @return true if magic, size, entry point and CRC32 are strictly valid
 */
bool bootloader_verify_app(const uint8_t *app_flash_ptr, const app_metadata_t *meta);

/**
 * @brief Atomic write of application metadata ensuring magic is written LAST (Anti-Bricking)
 * @param meta_storage Target metadata storage buffer or flash record
 * @param app_size Programmed binary byte count
 * @param crc32 Computed application CRC32
 * @param entry_point Programmed Reset_Handler address
 * @param version Application firmware version
 * @return true if successfully committed
 */
bool bootloader_commit_metadata(app_metadata_t *meta_storage, uint32_t app_size,
                                uint32_t crc32, uint32_t entry_point, uint32_t version);

/**
 * @brief Get current bootloader state
 */
boot_state_t bootloader_get_state(void);

/**
 * @brief Get remaining time in boot window in ms
 */
uint32_t bootloader_get_remaining_window_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32F103_BOOTLOADER_FSM_H */
