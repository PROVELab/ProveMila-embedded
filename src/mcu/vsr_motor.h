#ifndef VSR_MOTOR_H
#define VSR_MOTOR_H
#include <stdint.h> // for fixed size

// status registers for the motor controller
// motor speed status
typedef struct {
    int16_t quadrature_current; // current in 0.125 Arms
    int16_t direct_current;     // current in 0.125 Arms
    int16_t motor_speed;        // speed in RPM
} motor_speed_status_s;

typedef struct {
    int16_t measured_dc_voltage;   // 0.125 V
    int16_t calculated_dc_current; // 0.125 A
    uint16_t motor_current_limit;  // in 0.125 Arms
} motor_power_status_s;

typedef struct {
    uint8_t protection_code;      // TODO: where is this defined?
    uint8_t safety_error_code;    // TODO: where is this defined?
    uint8_t motor_temp;           // TODO: document units and add functions
    uint8_t inverter_bridge_temp; // TODO: document units and add functions
    uint8_t bus_cap_temp;         // TODO: document units and add functions
    uint8_t pwm_status;           // i doubt I'll use this
} motor_safety_status_s;

// motor control register
typedef struct {
    int16_t speed_reference; // speed reference in 0.125 RPM
    uint8_t discharge_limit_pct;
    uint8_t charge_limit_pct;
} motor_control_s;

#endif