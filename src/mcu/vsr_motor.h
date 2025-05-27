#ifndef VSR_MOTOR_H
#define VSR_MOTOR_H
#include <stdint.h> // for fixed size

// status registers for the motor controller
// motor speed status
typedef struct {
    int32_t quadrature_current; // current in Arms
    int32_t direct_current;     // current in Arms
    int16_t motor_speed;        // speed in RPM
} motor_hspeed_status_s;        // high speed status messages

typedef struct {
    int32_t measured_dc_voltage;   // in Volts
    int32_t calculated_dc_current; // in Amps
    uint32_t motor_current_limit;  // in Arms
} motor_mspeed_status_s;           // medium speed status messages

typedef struct {
    uint8_t protection_code;      // TODO: where is this defined?
    uint8_t safety_error_code;    // TODO: where is this defined?
    int16_t motor_temp;           // motor temp, F
    int16_t inverter_bridge_temp; // bridge temp, F
    int16_t bus_cap_temp;         // bus capacitor temp, F
    uint8_t pwm_status;           // i doubt I'll use this
} motor_safety_status_s;          // low speed status messages

// motor control register
typedef struct {
    int32_t speed_reference; // speed reference in RPM (from 0.125 RPM)
    uint8_t discharge_limit_pct;
    uint8_t charge_limit_pct;
} motor_control_s;

// 0 by default so MOTOR_OK initially
typedef enum { MOTOR_OK = 0, MOTOR_ERROR_STOP } high_level_motor_state;

typedef struct {
    high_level_motor_state motor_state;
} motor_error_state;

#endif