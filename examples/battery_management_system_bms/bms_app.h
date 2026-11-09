#ifndef BMS_APP_H
#define BMS_APP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BMS_CONTACTOR_OPEN = 0,
    BMS_CONTACTOR_PRECHARGE = 1,
    BMS_CONTACTOR_CLOSED = 2,
    BMS_CONTACTOR_FAULT = 3
} bms_contactor_state_t;

typedef struct {
    /* High-Voltage pack telemetry */
    uint32_t pack_voltage_mv;           /* Total pack voltage in mV (e.g. 384000 = 384V) */
    int32_t pack_current_ma;            /* Current in mA (signed: positive=discharge, negative=charge) */
    uint16_t state_of_charge_permille;  /* 0..1000 (0.1% resolution, e.g. 850 = 85.0%) */
    uint16_t state_of_health_permille;  /* 0..1000 (0.1% resolution, e.g. 985 = 98.5%) */
    uint16_t cell_min_voltage_mv;       /* Lowest cell in mV */
    uint16_t cell_max_voltage_mv;       /* Highest cell in mV */
    int16_t temp_min_deci_celsius;      /* Minimum thermistor in 0.1 C */
    int16_t temp_max_deci_celsius;      /* Maximum thermistor in 0.1 C */
    uint16_t isolation_resistance_kohm; /* Isolation between HV bus and chassis */

    /* Contactors & safety loop */
    bms_contactor_state_t contactor_state;
    bool hvil_closed;                   /* High-Voltage Interlock Loop */

    /* Fault flags for DTC reporting */
    bool fault_cell_overvoltage;
    bool fault_cell_undervoltage;
    bool fault_isolation_loss;
    bool fault_contactor_welded;
} bms_state_t;

void bms_app_init(void);
void bms_app_process(uint32_t delta_ms);
const bms_state_t *bms_app_get_state(void);

void bms_app_set_cell_voltages(uint16_t min_mv, uint16_t max_mv);
void bms_app_set_pack_current(int32_t current_ma);
void bms_app_inject_fault_ovp(bool active);
void bms_app_inject_fault_isolation(bool active);
bool bms_app_execute_contactor_cycle(void);

#ifdef __cplusplus
}
#endif

#endif /* BMS_APP_H */
