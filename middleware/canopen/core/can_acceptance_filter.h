/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_MIDDLEWARE_CORE_CAN_ACCEPTANCE_FILTER_H
#define CANOPEN_MIDDLEWARE_CORE_CAN_ACCEPTANCE_FILTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Add one 11-bit standard CAN identifier to a bounded acceptance list.
 *
 * The identifier is masked to 11 bits, rejected when it addresses the global
 * 0x7FF value, de-duplicated, and dropped when the list is already full.
 * Transport-neutral by design so host tests can exercise the exact policy
 * used by the firmware bxCAN filter builder.
 *
 * @param ids List storage owned by the caller.
 * @param capacity Maximum number of entries in @p ids.
 * @param count Current number of valid entries in @p ids; updated on success.
 * @param id Raw identifier to add; upper bits are ignored.
 * @return true when the identifier is present in the list after the call.
 */
bool CANopenAcceptanceFilter_Add(uint16_t *ids, uint32_t capacity, uint32_t *count, uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_MIDDLEWARE_CORE_CAN_ACCEPTANCE_FILTER_H */
