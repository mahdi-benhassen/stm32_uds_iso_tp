/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "canopen_core.h"

#include "canopen_reference_co.h"
#include "canopen_reference_hw.h"
#include "cia401_reference.h"
#include "cia402_reference.h"

/* Implemented by App/Src/CO_app_STM32_reference.c. */
extern CO_t *CO;

int
canopen_core_init(CANopenNodeSTM32 *instance) {
    return canopen_app_init(instance);
}

void
canopen_core_process(void) {
    canopen_app_process();
}

void
canopen_core_process_cycle(void) {
    canopen_app_interrupt();
}

void
canopen_core_force_safe_state(void) {
    Cia401Reference_ForceSafeOutputs();
    Cia402Reference_ForceDisable();
}

int
canopen_core_status(void) {
    return CO == NULL ? -1 : 0;
}
