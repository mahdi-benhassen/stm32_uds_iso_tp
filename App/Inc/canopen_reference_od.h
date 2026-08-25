/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Select the generated CANopenNode Object Dictionary for the active firmware
 * personality.  Exactly one OD source is linked by CMake.
 */
#ifndef CANOPEN_REFERENCE_OD_H
#define CANOPEN_REFERENCE_OD_H

#include "canopen_reference_config.h"

#if (CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY != 0U)
#include "inventus_battery_OD.h"
#elif (CANOPEN_REFERENCE_ENABLE_CIA418 != 0U)
#include "cia418_OD.h"
#else
#include "OD.h"
#endif

#endif /* CANOPEN_REFERENCE_OD_H */
