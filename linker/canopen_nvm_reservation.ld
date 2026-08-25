/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * CANopen NVM reservation guard for the CubeMX branch.
 *
 * The generated STM32F767xx_FLASH.ld declares a single 1024 KiB FLASH region
 * even though the STM32F767 provides 2 MiB. The OD 1010h/1011h persistence
 * backend uses physical sectors 10 and 11 at 0x08180000..0x081FFFFF, which
 * lie beyond that declared region. Today the region boundary keeps the
 * application image away from the NVM sectors only implicitly; these ASSERTs
 * make the reservation explicit and fail closed if the memory layout is ever
 * widened or rearranged without preserving it.
 *
 * This fragment is additive (-T on the link line) and does not modify any
 * generated file.
 */

ASSERT(SIZEOF(.isr_vector) > 0,
       "CANopen NVM guard: vector table must exist before image checks")

/* _etext marks the end of the resident application image in FLASH. */
ASSERT((_etext <= 0x08100000),
       "CANopen NVM reservation: application image must stay below sector 10 origin 0x08100000")

ASSERT(((_etext - ORIGIN(FLASH)) <= LENGTH(FLASH)),
       "CANopen NVM reservation: application load image exceeds the declared FLASH region")
