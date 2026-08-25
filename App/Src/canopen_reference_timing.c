#include "canopen_reference_timing.h"

#include <stddef.h>
#include <string.h>

#include "canopen_reference_config.h"
#include "stm32f7xx.h"

#ifndef CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION
#define CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION 0U
#endif

#ifndef CANOPEN_REFERENCE_TIMING_ISR_BUDGET_US
#define CANOPEN_REFERENCE_TIMING_ISR_BUDGET_US 1000U
#endif
#ifndef CANOPEN_REFERENCE_TIMING_ISR_WARNING_US
#define CANOPEN_REFERENCE_TIMING_ISR_WARNING_US 500U
#endif

volatile CANopenReferenceTimingStats canopenReferenceTimingStats;
static volatile uint32_t canopenReferenceTimingLastTim7Cycle;
static volatile uint8_t canopenReferenceTimingTim7Primed;

#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
static uint32_t
CANopenReferenceTiming_ReadCycles(void) {
    return DWT->CYCCNT;
}

static void
CANopenReferenceTiming_SaturatingIncrement(volatile uint32_t *value) {
    if (*value != UINT32_MAX) {
        ++(*value);
    }
}

static void
CANopenReferenceTiming_SaturatingMax(volatile uint32_t *value, uint32_t sample) {
    if (sample > *value) {
        *value = sample;
    }
}
#endif

void
CANopenReferenceTiming_Init(void) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    memset((void *)&canopenReferenceTimingStats, 0, sizeof(canopenReferenceTimingStats));
    canopenReferenceTimingStats.core_clock_hz = SystemCoreClock;
    canopenReferenceTimingLastTim7Cycle = 0U;
    canopenReferenceTimingTim7Primed = 0U;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#else
    /* Keep the public lifecycle call valid in production images. */
#endif
}

uint32_t
CANopenReferenceTiming_Tim7Enter(void) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t now = CANopenReferenceTiming_ReadCycles();
    if (canopenReferenceTimingTim7Primed != 0U) {
        uint32_t period = now - canopenReferenceTimingLastTim7Cycle;
        CANopenReferenceTiming_SaturatingMax(&canopenReferenceTimingStats.tim7_period_cycles_max, period);
    } else {
        canopenReferenceTimingTim7Primed = 1U;
    }
    canopenReferenceTimingLastTim7Cycle = now;
    return now;
#else
    return 0U;
#endif
}

void
CANopenReferenceTiming_Tim7Exit(uint32_t start_cycles) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t elapsed = CANopenReferenceTiming_ReadCycles() - start_cycles;
    uint32_t budget_cycles = (canopenReferenceTimingStats.core_clock_hz / 1000000U)
                           * CANOPEN_REFERENCE_TIMING_ISR_BUDGET_US;
    uint32_t warning_cycles = (canopenReferenceTimingStats.core_clock_hz / 1000000U)
                            * CANOPEN_REFERENCE_TIMING_ISR_WARNING_US;
    CANopenReferenceTiming_SaturatingIncrement(&canopenReferenceTimingStats.tim7_irq_count);
    CANopenReferenceTiming_SaturatingMax(&canopenReferenceTimingStats.tim7_irq_cycles_max, elapsed);
    if (elapsed > warning_cycles) {
        CANopenReferenceTiming_SaturatingIncrement(&canopenReferenceTimingStats.tim7_warning_count);
    }
    if (elapsed > budget_cycles) {
        CANopenReferenceTiming_SaturatingIncrement(&canopenReferenceTimingStats.tim7_overrun_count);
    }
#else
    (void)start_cycles;
#endif
}

uint32_t
CANopenReferenceTiming_CanEnter(CANopenReferenceTimingCanContext context) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    (void)context;
    return CANopenReferenceTiming_ReadCycles();
#else
    (void)context;
    return 0U;
#endif
}

void
CANopenReferenceTiming_CanExit(CANopenReferenceTimingCanContext context, uint32_t start_cycles) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    if (context < CANOPEN_REFERENCE_TIMING_CAN_CONTEXT_COUNT) {
        uint32_t elapsed = CANopenReferenceTiming_ReadCycles() - start_cycles;
        CANopenReferenceTiming_SaturatingIncrement(&canopenReferenceTimingStats.can_irq_count[context]);
        CANopenReferenceTiming_SaturatingMax(&canopenReferenceTimingStats.can_irq_cycles_max[context], elapsed);
    }
#else
    (void)context;
    (void)start_cycles;
#endif
}

uint32_t
CANopenReferenceTiming_PhaseEnter(void) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    return CANopenReferenceTiming_ReadCycles();
#else
    return 0U;
#endif
}

void
CANopenReferenceTiming_PhaseExit(volatile uint32_t *max_cycles, uint32_t start_cycles) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    if (max_cycles != NULL) {
        uint32_t elapsed = CANopenReferenceTiming_ReadCycles() - start_cycles;
        CANopenReferenceTiming_SaturatingMax(max_cycles, elapsed);
    }
#else
    (void)max_cycles;
    (void)start_cycles;
#endif
}

uint32_t
CANopenReferenceTiming_MainlineEnter(void) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    return CANopenReferenceTiming_ReadCycles();
#else
    return 0U;
#endif
}

void
CANopenReferenceTiming_MainlineExit(uint32_t start_cycles) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t elapsed = CANopenReferenceTiming_ReadCycles() - start_cycles;
    CANopenReferenceTiming_SaturatingIncrement(&canopenReferenceTimingStats.mainline_sample_count);
    CANopenReferenceTiming_SaturatingMax(&canopenReferenceTimingStats.mainline_cycles_max, elapsed);
#else
    (void)start_cycles;
#endif
}

void
CANopenReferenceTiming_GetStats(CANopenReferenceTimingStats *stats) {
    if (stats == NULL) {
        return;
    }
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    memcpy(stats, (const void *)&canopenReferenceTimingStats, sizeof(*stats));
    __set_PRIMASK(primask);
#else
    memset(stats, 0, sizeof(*stats));
#endif
}
