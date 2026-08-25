/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "can_acceptance_filter.h"

#define TEST_CAPACITY 4U

static void
test_null_arguments_are_rejected(void) {
    uint16_t ids[TEST_CAPACITY] = {0};
    uint32_t count = 0U;

    assert(!CANopenAcceptanceFilter_Add(NULL, TEST_CAPACITY, &count, 0x100U));
    assert(!CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, NULL, 0x100U));
    assert(count == 0U);
}

static void
test_global_identifier_is_rejected(void) {
    uint16_t ids[TEST_CAPACITY] = {0};
    uint32_t count = 0U;

    assert(!CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x7FFU));
    assert(!CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x1FFFU));
    assert(count == 0U);
}

static void
test_identifier_is_masked_to_eleven_bits(void) {
    uint16_t ids[TEST_CAPACITY] = {0};
    uint32_t count = 0U;

    assert(CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x1800U | 0x123U));
    assert(count == 1U);
    assert(ids[0] == 0x123U);
}

static void
test_duplicates_are_ignored(void) {
    uint16_t ids[TEST_CAPACITY] = {0};
    uint32_t count = 0U;

    assert(CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x080U));
    assert(CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x080U));
    assert(count == 1U);
    assert(ids[0] == 0x080U);
}

static void
test_capacity_is_enforced(void) {
    uint16_t ids[TEST_CAPACITY] = {0};
    uint32_t count = 0U;

    assert(CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x000U));
    assert(CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x001U));
    assert(CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x002U));
    assert(CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x003U));
    assert(!CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x004U));
    /* A duplicate is "already present", which succeeds regardless of capacity
     * because de-duplication precedes the bounds check. */
    assert(CANopenAcceptanceFilter_Add(ids, TEST_CAPACITY, &count, 0x000U));
    assert(count == TEST_CAPACITY);
    for (uint32_t i = 0U; i < TEST_CAPACITY; ++i) {
        assert(ids[i] == i);
    }
}

static void
test_zero_capacity_rejects_everything(void) {
    uint16_t ids[1] = {0};
    uint32_t count = 0U;

    assert(!CANopenAcceptanceFilter_Add(ids, 0U, &count, 0x100U));
    assert(count == 0U);
}

int
main(void) {
    test_null_arguments_are_rejected();
    test_global_identifier_is_rejected();
    test_identifier_is_masked_to_eleven_bits();
    test_duplicates_are_ignored();
    test_capacity_is_enforced();
    test_zero_capacity_rejects_everything();
    return 0;
}
