/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Product-level configuration for the STM32F767 CANopen reference firmware.
 * This file is intentionally outside third_party so it can be owned and
 * versioned by the product team.
 */
#ifndef CANOPEN_REFERENCE_CONFIG_H
#define CANOPEN_REFERENCE_CONFIG_H

#include <stdint.h>

/* Exactly one certified device-profile personality is the normal product mode.
 * A combined I/O + drive build is useful for integration experiments only and
 * requires a product-specific device type, EDS/XDD, and conformance scope. */
#ifndef CANOPEN_REFERENCE_ENABLE_CIA401
#define CANOPEN_REFERENCE_ENABLE_CIA401 1U
#endif

#ifndef CANOPEN_REFERENCE_ENABLE_CIA402
#define CANOPEN_REFERENCE_ENABLE_CIA402 0U
#endif

/* CiA 418 is an opt-in dedicated CANopenNode live-OD personality. It must
 * not be combined with the default CiA 401/402 OD because the profile index
 * ranges and generated source selection are mutually exclusive. */
#ifndef CANOPEN_REFERENCE_ENABLE_CIA418
#define CANOPEN_REFERENCE_ENABLE_CIA418 0U
#endif

/* Inventus battery OD is an isolated, non-commercial test personality. */
#ifndef CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY
#define CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY 0U
#endif

#ifndef CANOPEN_REFERENCE_ALLOW_COMBINED_PROFILES
#define CANOPEN_REFERENCE_ALLOW_COMBINED_PROFILES 0U
#endif

#if ((CANOPEN_REFERENCE_ENABLE_CIA418 != 0U) \
     && ((CANOPEN_REFERENCE_ENABLE_CIA401 != 0U) || (CANOPEN_REFERENCE_ENABLE_CIA402 != 0U)))
#error "CiA 418 personality cannot be combined with the default CiA 401/402 OD personality."
#endif

#if ((CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY != 0U) \
     && ((CANOPEN_REFERENCE_ENABLE_CIA401 != 0U) \
         || (CANOPEN_REFERENCE_ENABLE_CIA402 != 0U) \
         || (CANOPEN_REFERENCE_ENABLE_CIA418 != 0U)))
#error "Inventus battery test personality must be built as an exclusive OD personality."
#endif

#if ((CANOPEN_REFERENCE_ENABLE_CIA401 != 0U) && (CANOPEN_REFERENCE_ENABLE_CIA402 != 0U) \
     && (CANOPEN_REFERENCE_ALLOW_COMBINED_PROFILES == 0U))
#error "Select one device profile or explicitly authorize the non-conformant combined reference mode."
#endif

#if ((CANOPEN_REFERENCE_ENABLE_CIA401 == 0U) && (CANOPEN_REFERENCE_ENABLE_CIA402 == 0U) \
     && (CANOPEN_REFERENCE_ENABLE_CIA418 == 0U) \
     && (CANOPEN_REFERENCE_ENABLE_INVENTUS_BATTERY == 0U))
#error "At least one application profile or the explicit CiA 418 adapter mode must be selected."
#endif

/* Default CiA 301 settings. Node-ID and bitrate remain reconfigurable through
 * LSS when a valid identity is provisioned. The F767 bxCAN peripheral is set
 * to 500 kbit/s by the CubeMX/HAL initialization. */
#define CANOPEN_REFERENCE_DEFAULT_NODE_ID       10U
#define CANOPEN_REFERENCE_DEFAULT_BITRATE_KBPS  500U
#define CANOPEN_REFERENCE_HEARTBEAT_MS          1000U

/* Bus-off recovery is mainline-only: stop CAN, wait for the bus to settle,
 * rebuild filters/OD state, and enter a latched safe fault after bounded
 * failures. The board may override these values at compile time. */
#ifndef CANOPEN_REFERENCE_CAN_RECOVERY_WAIT_MS
#define CANOPEN_REFERENCE_CAN_RECOVERY_WAIT_MS  100U
#endif
#ifndef CANOPEN_REFERENCE_CAN_RECOVERY_MAX_ATTEMPTS
#define CANOPEN_REFERENCE_CAN_RECOVERY_MAX_ATTEMPTS 3U
#endif

/* bxCAN list filters are intentionally bounded. A product that enables more
 * active COB-IDs must raise this value and verify available filter banks. */
#ifndef CANOPEN_REFERENCE_CAN_FILTER_MAX_IDS
#define CANOPEN_REFERENCE_CAN_FILTER_MAX_IDS 20U
#endif

/* Replace before release. These are deliberately non-production reference
 * values. The same values must be updated in the EDS/XDD and production label. */
#define CANOPEN_REFERENCE_VENDOR_ID             UINT32_C(0x00000000)
#define CANOPEN_REFERENCE_PRODUCT_CODE          UINT32_C(0xF7670401)
#define CANOPEN_REFERENCE_REVISION              UINT32_C(0x00010000)
#define CANOPEN_REFERENCE_SERIAL                UINT32_C(0x00000001)

/* Fail safe by default. Hardware adapters must explicitly permit outputs and
 * drive power after their own interlocks and diagnostics have passed. */
#define CANOPEN_REFERENCE_OUTPUTS_DEFAULT_SAFE  1U

/* Optional mainline-only UART diagnostic summary. A board integration must
 * override CANopenReferenceDiagnostics_Write() with a bounded, non-blocking
 * implementation before enabling this switch. */
#ifndef CANOPEN_REFERENCE_UART_DIAGNOSTICS
#define CANOPEN_REFERENCE_UART_DIAGNOSTICS       0U
#endif

/* Independent watchdog supervision is opt-in until the board validates the
 * nominal LSI frequency and reset-recovery behavior. Both the 1 ms timer path
 * and the mainline must make progress before refresh is permitted. */
#ifndef CANOPEN_REFERENCE_ENABLE_IWDG
#define CANOPEN_REFERENCE_ENABLE_IWDG            0U
#endif

/* DWT timing instrumentation is opt-in for board qualification. It records
 * cycle-counter maxima and overrun counts without changing the default image. */
#ifndef CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION
#define CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION 0U
#endif
#ifndef CANOPEN_REFERENCE_TIMING_ISR_BUDGET_US
#define CANOPEN_REFERENCE_TIMING_ISR_BUDGET_US 1000U
#endif
#ifndef CANOPEN_REFERENCE_TIMING_ISR_WARNING_US
#define CANOPEN_REFERENCE_TIMING_ISR_WARNING_US 500U
#endif
#if (CANOPEN_REFERENCE_TIMING_ISR_WARNING_US >= CANOPEN_REFERENCE_TIMING_ISR_BUDGET_US)
#error "ISR warning threshold must be less than the hard budget limit."
#endif
#ifndef CANOPEN_REFERENCE_IWDG_TIMEOUT_MS
#define CANOPEN_REFERENCE_IWDG_TIMEOUT_MS        200U
#endif
#ifndef CANOPEN_REFERENCE_IWDG_STARTUP_GRACE_MS
#define CANOPEN_REFERENCE_IWDG_STARTUP_GRACE_MS  100U
#endif
#if (CANOPEN_REFERENCE_IWDG_STARTUP_GRACE_MS >= CANOPEN_REFERENCE_IWDG_TIMEOUT_MS)
#error "IWDG startup grace must be shorter than the watchdog timeout."
#endif

/* UDS is enabled by default for the validated reference build. A product may
 * explicitly disable it at compile time after reviewing its diagnostic policy.
 * The identifiers remain configurable; 0x7E0/0x7E8 are the reference defaults. */
#ifndef CANOPEN_REFERENCE_ENABLE_UDS
#define CANOPEN_REFERENCE_ENABLE_UDS             1U
#endif
#ifndef UDS_RX_CAN_ID
#define UDS_RX_CAN_ID                            0x7E0U
#endif
#ifndef UDS_TX_CAN_ID
#define UDS_TX_CAN_ID                            0x7E8U
#endif
#if (UDS_RX_CAN_ID > 0x7FFU) || (UDS_TX_CAN_ID > 0x7FFU) || (UDS_RX_CAN_ID == UDS_TX_CAN_ID)
#error "UDS CAN identifiers must be distinct standard 11-bit identifiers."
#endif

/* CiA 309-3 ASCII gateway support is disabled unless the product has an
 * authenticated/physical diagnostic access policy and a bounded UART bridge. */
#ifndef CANOPEN_REFERENCE_ENABLE_GATEWAY
#define CANOPEN_REFERENCE_ENABLE_GATEWAY         0U
#endif

/* CiA 302 NMT master is an opt-in personality. The default image remains an
 * NMT slave and does not emit remote-node commands. The peer defaults are
 * deliberately explicit so the hardware procedure is reproducible. */
#ifndef CANOPEN_REFERENCE_ENABLE_CIA302_MASTER
#define CANOPEN_REFERENCE_ENABLE_CIA302_MASTER   0U
#endif
#ifndef CANOPEN_REFERENCE_CIA302_PEER_NODE_ID
#define CANOPEN_REFERENCE_CIA302_PEER_NODE_ID    11U
#endif
#ifndef CANOPEN_REFERENCE_CIA302_HEARTBEAT_TIMEOUT_MS
#define CANOPEN_REFERENCE_CIA302_HEARTBEAT_TIMEOUT_MS 1500U
#endif
#ifndef CANOPEN_REFERENCE_CIA302_BOOT_TIMEOUT_MS
#define CANOPEN_REFERENCE_CIA302_BOOT_TIMEOUT_MS 10000U
#endif
#ifndef CANOPEN_REFERENCE_CIA302_AUTO_START
#define CANOPEN_REFERENCE_CIA302_AUTO_START      0U
#endif

#if (CANOPEN_REFERENCE_CIA302_PEER_NODE_ID < 1U) || (CANOPEN_REFERENCE_CIA302_PEER_NODE_ID > 127U)
#error "CANOPEN_REFERENCE_CIA302_PEER_NODE_ID must be in the CANopen range 1..127."
#endif

#endif /* CANOPEN_REFERENCE_CONFIG_H */
