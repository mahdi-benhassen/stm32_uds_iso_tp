#ifndef BCM_APP_H
#define BCM_APP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BCM_LIGHT_OFF = 0,
    BCM_LIGHT_LOW_BEAM = 1,
    BCM_LIGHT_HIGH_BEAM = 2,
    BCM_LIGHT_AUTO = 3
} bcm_light_state_t;

typedef enum {
    BCM_WIPER_OFF = 0,
    BCM_WIPER_INT = 1,
    BCM_WIPER_LOW = 2,
    BCM_WIPER_HIGH = 3
} bcm_wiper_state_t;

typedef struct {
    /* Actuator states */
    bcm_light_state_t headlights;
    bool high_beam_active;
    bool hazard_lights_active;
    bool horn_active;
    bcm_wiper_state_t wipers;
    bool driver_door_locked;
    bool passenger_door_locked;

    /* Sensor & telemetry inputs */
    bool driver_door_ajar;
    bool passenger_door_ajar;
    uint16_t battery_voltage_mv;
    int16_t ambient_temp_deci_celsius;
    uint16_t vehicle_speed_kph;

    /* Override flags for Service 0x2F (InputOutputControl) */
    bool headlight_override_active;
    bcm_light_state_t headlight_override_val;
    bool horn_override_active;
    bool horn_override_val;
    bool hazard_override_active;
    bool hazard_override_val;

    /* Fault injection flags for DTCs */
    bool fault_light_short_circuit;
    bool fault_door_lock_stall;
    bool fault_battery_low;
} bcm_state_t;

void bcm_app_init(void);
void bcm_app_process(uint32_t delta_ms);
const bcm_state_t *bcm_app_get_state(void);

void bcm_app_set_door_ajar(bool driver, bool ajar);
void bcm_app_set_battery_voltage(uint16_t mv);
void bcm_app_inject_fault_light_short(bool active);
void bcm_app_inject_fault_door_stall(bool active);

void bcm_app_io_override_headlight(bool enable, bcm_light_state_t val);
void bcm_app_io_override_horn(bool enable, bool val);
void bcm_app_io_override_hazard(bool enable, bool val);

#ifdef __cplusplus
}
#endif

#endif /* BCM_APP_H */
