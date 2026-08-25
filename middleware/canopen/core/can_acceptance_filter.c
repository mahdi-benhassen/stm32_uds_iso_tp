/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "can_acceptance_filter.h"

bool
CANopenAcceptanceFilter_Add(uint16_t *ids, uint32_t capacity, uint32_t *count, uint32_t id) {
    if (ids == NULL || count == NULL) {
        return false;
    }
    id &= 0x07FFU;
    if (id == 0x7FFU) {
        return false;
    }
    for (uint32_t i = 0U; i < *count; ++i) {
        if (ids[i] == id) {
            return true;
        }
    }
    if (*count >= capacity) {
        return false;
    }
    ids[(*count)++] = (uint16_t)id;
    return true;
}
