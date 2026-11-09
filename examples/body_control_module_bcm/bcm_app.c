#include "bcm_app.h"
#include <string.h>

static bcm_state_t g_bcm;

void bcm_app_init(void)
{
    memset(&g_bcm, 0, sizeof(g_bcm));
    g_bcm.headlights = BCM_LIGHT_OFF;
    g_bcm.wipers = BCM_WIPER_OFF;
    g_bcm.driver_door_locked = true;
    g_bcm.passenger_door_locked = true;
    g_bcm.battery_voltage_mv = 12600; /* 12.6V nominal */
    g_bcm.ambient_temp_deci_celsius = 215; /* 21.5 C */
    g_bcm.vehicle_speed_kph = 0;
}

void bcm_app_process(uint32_t delta_ms)
{
    (void)delta_ms;

    /* Low-voltage monitor */
    if (g_bcm.battery_voltage_mv < 10500) {
        g_bcm.fault_battery_low = true;
    } else if (g_bcm.battery_voltage_mv > 11500) {
        g_bcm.fault_battery_low = false;
    }

    /* Apply diagnostic overrides if active */
    if (g_bcm.headlight_override_active) {
        g_bcm.headlights = g_bcm.headlight_override_val;
    }
    if (g_bcm.horn_override_active) {
        g_bcm.horn_active = g_bcm.horn_override_val;
    }
    if (g_bcm.hazard_override_active) {
        g_bcm.hazard_lights_active = g_bcm.hazard_override_val;
    }
}

const bcm_state_t *bcm_app_get_state(void)
{
    return &g_bcm;
}

void bcm_app_set_door_ajar(bool driver, bool ajar)
{
    if (driver) {
        g_bcm.driver_door_ajar = ajar;
    } else {
        g_bcm.passenger_door_ajar = ajar;
    }
}

void bcm_app_set_battery_voltage(uint16_t mv)
{
    g_bcm.battery_voltage_mv = mv;
}

void bcm_app_inject_fault_light_short(bool active)
{
    g_bcm.fault_light_short_circuit = active;
}

void bcm_app_inject_fault_door_stall(bool active)
{
    g_bcm.fault_door_lock_stall = active;
}

void bcm_app_io_override_headlight(bool enable, bcm_light_state_t val)
{
    g_bcm.headlight_override_active = enable;
    g_bcm.headlight_override_val = val;
    if (!enable) {
        g_bcm.headlights = BCM_LIGHT_OFF;
    }
}

void bcm_app_io_override_horn(bool enable, bool val)
{
    g_bcm.horn_override_active = enable;
    g_bcm.horn_override_val = val;
    if (!enable) {
        g_bcm.horn_active = false;
    }
}

void bcm_app_io_override_hazard(bool enable, bool val)
{
    g_bcm.hazard_override_active = enable;
    g_bcm.hazard_override_val = val;
    if (!enable) {
        g_bcm.hazard_lights_active = false;
    }
}
