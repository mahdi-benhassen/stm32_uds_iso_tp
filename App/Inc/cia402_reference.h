/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CIA402_REFERENCE_H
#define CIA402_REFERENCE_H

#include <stdint.h>

void Cia402Reference_Init(void);

/* Runs in the deterministic 1 ms timer context after RPDO transfer into the OD.
 * The implementation is a bounded controlword/statusword state machine, not a
 * motion-control loop. Position, velocity, torque, braking, and safety control
 * remain board/product responsibilities. */
void Cia402Reference_Process1ms(void);

/* Forces the CiA 402 reference into switch-on-disabled semantics and requests
 * removal of drive enable from the hardware adapter. */
void Cia402Reference_ForceDisable(void);

#endif /* CIA402_REFERENCE_H */
