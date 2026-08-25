/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "canopen_reference_diagnostics.h"
#include "canopen_reference_cia302.h"
#include "canopen_reference_config.h"

#include <stdio.h>

#ifndef CANOPEN_REFERENCE_UART_DIAGNOSTICS
#define CANOPEN_REFERENCE_UART_DIAGNOSTICS 0U
#endif

#define CANOPEN_REFERENCE_DIAGNOSTICS_PERIOD_MS 1000U

static volatile uint32_t s_can_hardware_error_count;
static volatile uint32_t s_last_can_hardware_error;
static volatile uint32_t s_last_runtime_fault;
static volatile uint32_t s_can_recovery_count;

__attribute__((weak)) void
CANopenReferenceDiagnostics_Write(const uint8_t *bytes, uint16_t length) {
    (void)bytes;
    (void)length;
}

void
CANopenReferenceDiagnostics_ReportCanHardwareError(uint32_t hal_error_code) {
    (void)__atomic_fetch_add(&s_can_hardware_error_count, 1U, __ATOMIC_RELAXED);
    __atomic_store_n(&s_last_can_hardware_error, hal_error_code, __ATOMIC_RELEASE);
}

uint32_t
CANopenReferenceDiagnostics_CanHardwareErrorCount(void) {
    return __atomic_load_n(&s_can_hardware_error_count, __ATOMIC_ACQUIRE);
}

uint32_t
CANopenReferenceDiagnostics_LastCanHardwareError(void) {
    return __atomic_load_n(&s_last_can_hardware_error, __ATOMIC_ACQUIRE);
}

void
CANopenReferenceDiagnostics_ReportRuntimeFault(uint32_t fault_code) {
    __atomic_store_n(&s_last_runtime_fault, fault_code, __ATOMIC_RELEASE);
}

uint32_t
CANopenReferenceDiagnostics_LastRuntimeFault(void) {
    return __atomic_load_n(&s_last_runtime_fault, __ATOMIC_ACQUIRE);
}

void
CANopenReferenceDiagnostics_ReportCanRecovery(bool success) {
    (void)success;
    (void)__atomic_fetch_add(&s_can_recovery_count, 1U, __ATOMIC_RELAXED);
}

uint32_t
CANopenReferenceDiagnostics_CanRecoveryCount(void) {
    return __atomic_load_n(&s_can_recovery_count, __ATOMIC_ACQUIRE);
}

void
CANopenReferenceDiagnostics_Process(uint8_t node_id, uint16_t can_error_status, uint8_t led_green, uint8_t led_red,
                                    uint32_t now_ms) {
#if CANOPEN_REFERENCE_UART_DIAGNOSTICS
    static uint32_t last_report_ms;
    char line[220];
    int written;
    CANopenReferenceCia302Snapshot cia302;

    if ((uint32_t)(now_ms - last_report_ms) < CANOPEN_REFERENCE_DIAGNOSTICS_PERIOD_MS) {
        return;
    }
    last_report_ms = now_ms;
    CANopenReferenceCia302_GetSnapshot(&cia302);
#if CANOPEN_REFERENCE_ENABLE_CIA302_MASTER
    written = snprintf(line, sizeof(line),
                       "CANopen node=%u err=0x%04X led=%u/%u can_hw=%lu can_last=0x%08lX cia302=1 run=%u ready=%u "
                       "peer=%u state=0x%02X boot=%lu hb=%lu hbt=%lu bt=%lu net=%lu inv=%lu last=%u/%u/0x%02X\r\n",
                       (unsigned)node_id, (unsigned)can_error_status, (unsigned)led_green, (unsigned)led_red,
                       (unsigned long)CANopenReferenceDiagnostics_CanHardwareErrorCount(),
                       (unsigned long)CANopenReferenceDiagnostics_LastCanHardwareError(),
                       (unsigned)cia302.running, (unsigned)cia302.network_ready,
                       (unsigned)cia302.monitored_node_id, (unsigned)cia302.monitored_node_state,
                       (unsigned long)cia302.event_count_bootup, (unsigned long)cia302.event_count_heartbeat,
                       (unsigned long)cia302.event_count_heartbeat_timeout,
                       (unsigned long)cia302.event_count_boot_timeout,
                       (unsigned long)cia302.event_count_network_ready,
                       (unsigned long)cia302.event_count_invalid_frame,
                       (unsigned)cia302.last_event_type, (unsigned)cia302.last_event_node_id,
                       (unsigned)cia302.last_event_state);
#else
    written = snprintf(line, sizeof(line),
                       "CANopen node=%u err=0x%04X led=%u/%u can_hw=%lu can_last=0x%08lX cia302=0\r\n",
                       (unsigned)node_id, (unsigned)can_error_status, (unsigned)led_green, (unsigned)led_red,
                       (unsigned long)CANopenReferenceDiagnostics_CanHardwareErrorCount(),
                       (unsigned long)CANopenReferenceDiagnostics_LastCanHardwareError());
#endif
    if (written > 0) {
        uint16_t length = (uint16_t)((written >= (int)sizeof(line)) ? sizeof(line) - 1U : (unsigned)written);
        CANopenReferenceDiagnostics_Write((const uint8_t *)line, length);
    }
#else
    (void)node_id;
    (void)can_error_status;
    (void)led_green;
    (void)led_red;
    (void)now_ms;
#endif
}
