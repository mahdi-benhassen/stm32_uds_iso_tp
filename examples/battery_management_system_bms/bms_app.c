#include "bms_app.h"
#include <string.h>

static bms_state_t g_bms;

void bms_app_init(void)
{
    memset(&g_bms, 0, sizeof(g_bms));
    g_bms.pack_voltage_mv = 384000;         /* 384.0 V */
    g_bms.pack_current_ma = 15200;          /* 15.2 A discharge */
    g_bms.state_of_charge_permille = 850;   /* 85.0% */
    g_bms.state_of_health_permille = 985;   /* 98.5% */
    g_bms.cell_min_voltage_mv = 3980;       /* 3.980 V */
    g_bms.cell_max_voltage_mv = 4015;       /* 4.015 V */
    g_bms.temp_min_deci_celsius = 240;      /* 24.0 C */
    g_bms.temp_max_deci_celsius = 285;      /* 28.5 C */
    g_bms.isolation_resistance_kohm = 650;  /* 650 kOhm (healthy) */
    g_bms.contactor_state = BMS_CONTACTOR_CLOSED;
    g_bms.hvil_closed = true;
}

void bms_app_process(uint32_t delta_ms)
{
    (void)delta_ms;

    /* Safety checks */
    if (g_bms.cell_max_voltage_mv > 4250) {
        g_bms.fault_cell_overvoltage = true;
        g_bms.contactor_state = BMS_CONTACTOR_FAULT;
    }

    if (g_bms.cell_min_voltage_mv < 2800) {
        g_bms.fault_cell_undervoltage = true;
        g_bms.contactor_state = BMS_CONTACTOR_FAULT;
    }

    if (g_bms.isolation_resistance_kohm < 100) {
        g_bms.fault_isolation_loss = true;
    }
}

const bms_state_t *bms_app_get_state(void)
{
    return &g_bms;
}

void bms_app_set_cell_voltages(uint16_t min_mv, uint16_t max_mv)
{
    g_bms.cell_min_voltage_mv = min_mv;
    g_bms.cell_max_voltage_mv = max_mv;
}

void bms_app_set_pack_current(int32_t current_ma)
{
    g_bms.pack_current_ma = current_ma;
}

void bms_app_inject_fault_ovp(bool active)
{
    g_bms.fault_cell_overvoltage = active;
    if (active) {
        g_bms.cell_max_voltage_mv = 4280;
    } else {
        g_bms.cell_max_voltage_mv = 4015;
    }
}

void bms_app_inject_fault_isolation(bool active)
{
    g_bms.fault_isolation_loss = active;
    g_bms.isolation_resistance_kohm = active ? 45 : 650;
}

bool bms_app_execute_contactor_cycle(void)
{
    if (g_bms.fault_cell_overvoltage || g_bms.fault_cell_undervoltage) {
        return false;
    }
    g_bms.contactor_state = BMS_CONTACTOR_OPEN;
    g_bms.contactor_state = BMS_CONTACTOR_PRECHARGE;
    g_bms.contactor_state = BMS_CONTACTOR_CLOSED;
    return true;
}
