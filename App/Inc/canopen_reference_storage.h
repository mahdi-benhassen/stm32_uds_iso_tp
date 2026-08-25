/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_REFERENCE_STORAGE_H
#define CANOPEN_REFERENCE_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canopen_reference_co.h"

#ifndef CANOPEN_REFERENCE_STORAGE_SLOT_SIZE
#define CANOPEN_REFERENCE_STORAGE_SLOT_SIZE (256U * 1024U)
#endif

#ifndef CANOPEN_REFERENCE_STORAGE_MIN_STORE_INTERVAL_MS
#define CANOPEN_REFERENCE_STORAGE_MIN_STORE_INTERVAL_MS 0U
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize OD 1010h/1011h storage and restore a validated communication image.
 * Must run after CO_CANinit() and before CO_CANopenInit().
 */
CO_ReturnError_t CANopenReferenceStorage_Init(CO_t *co);

/**
 * Board persistence seam. On STM32F767 builds the default implementation is
 * a CRC-validated two-slot internal-Flash backend. A board may override both
 * hooks for external EEPROM, FRAM, or a product-specific wear-leveling layer.
 * The linker script reserves the final two 256 KiB sectors for this backend.
 */
bool CANopenReferenceStorage_BoardStore(const void *data, size_t length);
bool CANopenReferenceStorage_BoardRestore(void *data, size_t length);

/** Number of successful 0x1010 store operations since startup. */
uint32_t CANopenReferenceStorage_StoreCount(void);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_REFERENCE_STORAGE_H */
