/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CIA401_REFERENCE_H
#define CIA401_REFERENCE_H

#include <stdint.h>

/* Called once after CANopen and the generated Object Dictionary are available. */
void Cia401Reference_Init(void);

/* Called from the deterministic 1 ms CANopen timer context after RPDO handling
 * and before TPDO processing. It must remain bounded and non-blocking. */
void Cia401Reference_Process1ms(void);

/* Immediately move software-commanded I/O to a safe state. Board-level safety
 * measures must be independent and may be stricter. */
void Cia401Reference_ForceSafeOutputs(void);

#endif /* CIA401_REFERENCE_H */
