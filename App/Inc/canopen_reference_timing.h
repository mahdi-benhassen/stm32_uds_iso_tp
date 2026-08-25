#ifndef CANOPEN_REFERENCE_TIMING_H
#define CANOPEN_REFERENCE_TIMING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CANOPEN_REFERENCE_TIMING_CAN_TX = 0,
    CANOPEN_REFERENCE_TIMING_CAN_RX0,
    CANOPEN_REFERENCE_TIMING_CAN_RX1,
    CANOPEN_REFERENCE_TIMING_CAN_ERROR,
    CANOPEN_REFERENCE_TIMING_CAN_CONTEXT_COUNT
} CANopenReferenceTimingCanContext;

typedef enum {
    CANOPEN_REFERENCE_TIMING_APP_SYNC = 0,
    CANOPEN_REFERENCE_TIMING_APP_RPDO,
    CANOPEN_REFERENCE_TIMING_APP_CIA401,
    CANOPEN_REFERENCE_TIMING_APP_CIA402,
    CANOPEN_REFERENCE_TIMING_APP_CIA418,
    CANOPEN_REFERENCE_TIMING_APP_TPDO
} CANopenReferenceTimingAppPhase;

typedef struct {
    uint32_t core_clock_hz;
    uint32_t tim7_irq_count;
    uint32_t tim7_irq_cycles_max;
    uint32_t tim7_period_cycles_max;
    uint32_t tim7_overrun_count;
    uint32_t tim7_warning_count;
    uint32_t can_irq_count[CANOPEN_REFERENCE_TIMING_CAN_CONTEXT_COUNT];
    uint32_t can_irq_cycles_max[CANOPEN_REFERENCE_TIMING_CAN_CONTEXT_COUNT];
    uint32_t mainline_sample_count;
    uint32_t mainline_cycles_max;
    uint32_t app_interrupt_cycles_max;
    uint32_t sync_cycles_max;
    uint32_t rpdo_cycles_max;
    uint32_t cia401_cycles_max;
    uint32_t cia402_cycles_max;
    uint32_t cia418_cycles_max;
    uint32_t tpdo_cycles_max;
    uint32_t uds_rx_cycles_max;
    uint32_t isotp_cycles_max;
    uint32_t uds_dispatch_cycles_max;
    uint32_t uds_tx_cycles_max;
    uint32_t uds_mainline_cycles_max;
} CANopenReferenceTimingStats;

/** Initialize the optional DWT cycle-counter measurement path. */
void CANopenReferenceTiming_Init(void);

/** Record entry/exit around the complete TIM7 interrupt handler. */
uint32_t CANopenReferenceTiming_Tim7Enter(void);
void CANopenReferenceTiming_Tim7Exit(uint32_t start_cycles);

/** Record entry/exit around a complete CAN interrupt handler. */
uint32_t CANopenReferenceTiming_CanEnter(CANopenReferenceTimingCanContext context);
void CANopenReferenceTiming_CanExit(CANopenReferenceTimingCanContext context, uint32_t start_cycles);

/** Return the cycle count at the start of a bounded application interval. */
uint32_t CANopenReferenceTiming_PhaseEnter(void);

/** Update a phase maximum from a cycle-count interval. */
void CANopenReferenceTiming_PhaseExit(volatile uint32_t *max_cycles, uint32_t start_cycles);

/** Project-owned statistics object used by the timing hooks. */
extern volatile CANopenReferenceTimingStats canopenReferenceTimingStats;

/** Record one mainline canopen_app_process() interval. */
uint32_t CANopenReferenceTiming_MainlineEnter(void);
void CANopenReferenceTiming_MainlineExit(uint32_t start_cycles);

/** Copy a best-effort statistics snapshot for diagnostics/evidence export. */
void CANopenReferenceTiming_GetStats(CANopenReferenceTimingStats *stats);

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_REFERENCE_TIMING_H */
