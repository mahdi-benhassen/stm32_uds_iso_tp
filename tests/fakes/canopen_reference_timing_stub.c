#include "canopen_reference_timing.h"

#include <string.h>

volatile CANopenReferenceTimingStats canopenReferenceTimingStats;

void CANopenReferenceTiming_Init(void) { memset((void *)&canopenReferenceTimingStats, 0, sizeof(canopenReferenceTimingStats)); }
uint32_t CANopenReferenceTiming_Tim7Enter(void) { return 0U; }
void CANopenReferenceTiming_Tim7Exit(uint32_t start_cycles) { (void)start_cycles; }
uint32_t CANopenReferenceTiming_CanEnter(CANopenReferenceTimingCanContext context) { (void)context; return 0U; }
void CANopenReferenceTiming_CanExit(CANopenReferenceTimingCanContext context, uint32_t start_cycles) { (void)context; (void)start_cycles; }
uint32_t CANopenReferenceTiming_PhaseEnter(void) { return 0U; }
void CANopenReferenceTiming_PhaseExit(volatile uint32_t *max_cycles, uint32_t start_cycles) { (void)max_cycles; (void)start_cycles; }
uint32_t CANopenReferenceTiming_MainlineEnter(void) { return 0U; }
void CANopenReferenceTiming_MainlineExit(uint32_t start_cycles) { (void)start_cycles; }
void CANopenReferenceTiming_GetStats(CANopenReferenceTimingStats *stats) {
    if (stats != NULL) {
        memcpy(stats, (const void *)&canopenReferenceTimingStats, sizeof(*stats));
    }
}
