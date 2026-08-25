#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "canopen_reference_lss.h"

int main(void) {
    assert(CANopenReferenceLss_BitrateSupported(10U));
    assert(CANopenReferenceLss_BitrateSupported(125U));
    assert(CANopenReferenceLss_BitrateSupported(500U));
    assert(CANopenReferenceLss_BitrateSupported(1000U));
    assert(!CANopenReferenceLss_BitrateSupported(333U));
    assert(!CANopenReferenceLss_StoreConfiguration(0U, 500U));
    assert(!CANopenReferenceLss_StoreConfiguration(128U, 500U));
    assert(!CANopenReferenceLss_StoreConfiguration(10U, 333U));
    assert(!CANopenReferenceLss_StoreConfiguration(10U, 500U));
    puts("lss_policy: PASS");
    return 0;
}
