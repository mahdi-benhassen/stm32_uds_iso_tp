/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Single include boundary for the pinned CANopenNode stack in project code.
 *
 * The third-party headers contain deliberate narrowing mask idioms (for
 * example the OD_requestTPDO bit masks in 301/CO_odinterface.h) that trip
 * -Wconversion while they are parsed. Project sources must include this
 * header instead of "CANopen.h" so those known third-party diagnostics stay
 * visible but non-fatal, while all project-owned code after this point keeps
 * full -Werror=conversion enforcement from the production build.
 *
 * Note: any project file whose third-party include chain reaches CANopenNode
 * headers by another route (directly, or via CO_app_STM32.h) must open the
 * same suppression region before its first third-party include. A suppression
 * placed after an earlier unprotected parse is a no-op because the include
 * guards are already defined.
 */
#ifndef CANOPEN_REFERENCE_CO_H
#define CANOPEN_REFERENCE_CO_H

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#include "CANopen.h"

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* CANOPEN_REFERENCE_CO_H */
