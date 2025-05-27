#include "h300.h"

void parse_packet(PACKET_PARSE_INPUTS) {
    // H300 packets are 2.0B
    if (!msg->extd)
        return;

    // Check if the message ID matches any of the response message IDs
    switch (msg->identifier) {
    case HIGH_FREQ_DATA_ID:
        // Handle high frequency data
        parse_high_freq_data(msg, vsr);
        break;
    case MED_FREQ_DATA_ID:
        // Handle medium frequency data
        parse_med_freq_data(msg, vsr);
        break;
    case LOW_FREQ_DATA_ID:
        // Handle low frequency data
        parse_low_freq_data(msg, vsr);
        break;
    case CALIBRATION_RESULT_MSG_ID:
        // TODO: implement this
        break;
    default:
        // Unknown message ID, ignore or log it
        break;
    }
}

/* Parse high frequency data from the motor controller
 * and puts it in the VSR
 * Format:
 * [0,1] - Quadrature Current lq (units 0.125 Arms)
 * [2, 3] - Direct motor current (for field weakening, units 0.125 Arms)
 * [4, 5] - Motor speed (units RPM)
 */
void parse_high_freq_data(PACKET_PARSE_INPUTS) {
    int16_t lq, direct_current, motor_speed;

    memcpy(&lq, &msg->data[0], sizeof(int16_t));
    memcpy(&direct_current, &msg->data[2], sizeof(int16_t));
    memcpy(&motor_speed, &msg->data[4], sizeof(int16_t));

    // take ownership of the mutex to write to it
    xSemaphoreTake(vsr->motor_speed_mutex, portMAX_DELAY);

    // convert to Arms
    vsr->motor_speed.quadrature_current = (int32_t)lq * 8;
    // convert to Arms
    vsr->motor_speed.direct_current = (int32_t)direct_current * 8;
    vsr->motor_speed.motor_speed = motor_speed; // already in RPM
    xSemaphoreGive(vsr->motor_speed_mutex);
}

/* Parse medium frequency data from the motor controller
 * and puts it in the VSR
 * Format:
 * [0, 1] - DC Battery Voltage (measured, units 0.125 V)
 * [2, 3] - DC Battery Current (calculated, units 0.125 A)
 * [4, 5] - Motor Current Limit (units 0.125 Arms) - Maximum allowed current
 */
void parse_med_freq_data(PACKET_PARSE_INPUTS) {
    int16_t measured_dc_voltage, calculated_dc_current;
    uint16_t motor_current_limit;

    memcpy(&measured_dc_voltage, &msg->data[0], sizeof(int16_t));
    memcpy(&calculated_dc_current, &msg->data[2], sizeof(int16_t));
    memcpy(&motor_current_limit, &msg->data[4], sizeof(uint16_t));

    // take ownership of the mutex to write to it
    xSemaphoreTake(vsr->motor_power_mutex, portMAX_DELAY);
    // convert to Volts
    vsr->motor_power.measured_dc_voltage = (int32_t)measured_dc_voltage * 8;
    // convert to Amps
    vsr->motor_power.calculated_dc_current = (int32_t)calculated_dc_current * 8;
    // convert to Arms
    vsr->motor_power.motor_current_limit = (uint32_t)motor_current_limit * 8;
    xSemaphoreGive(vsr->motor_power_mutex);
}

/* Parse low frequency data from the motor controller
 * and puts it in the VSR
 * Format:
 * [0] - Protection Code (TODO: where is this defined?)
 * [1] - Safety Error Code (TODO: where is this defined?)
 * [2] - Motor Temperature (units Celsius, -60 offset)
 * [3] - Inverter Bridge Temperature (units Celsius -60 offset)
 * [4] - Bus Capacitor Temperature (units Celsius, -60 offset)
 * [5] - PWM Status (i doubt I'll use this)
 */
void parse_low_freq_data(PACKET_PARSE_INPUTS) {
    // initial declarations
    uint8_t protection_code, safety_error_code, motor_temp,
        inverter_bridge_temp, bus_cap_temp, pwm_status;

    memcpy(&protection_code, &msg->data[0], sizeof(uint8_t));
    memcpy(&safety_error_code, &msg->data[1], sizeof(uint8_t));
    memcpy(&motor_temp, &msg->data[2], sizeof(uint8_t));
    memcpy(&inverter_bridge_temp, &msg->data[3], sizeof(uint8_t));
    memcpy(&bus_cap_temp, &msg->data[4], sizeof(uint8_t));
    memcpy(&pwm_status, &msg->data[5], sizeof(uint8_t));

    // take ownership of the mutex to write to it
    xSemaphoreTake(vsr->motor_safety_mutex, portMAX_DELAY);
    vsr->motor_safety.protection_code = protection_code;
    vsr->motor_safety.safety_error_code = safety_error_code;

    // convert to Fahrenheit from Celsius, with the offset
    vsr->motor_safety.motor_temp =
        CELSIUS_TO_FAHRENHEIT(BYTE_TO_TEMP(motor_temp));
    vsr->motor_safety.inverter_bridge_temp =
        CELSIUS_TO_FAHRENHEIT(BYTE_TO_TEMP(inverter_bridge_temp));
    vsr->motor_safety.bus_cap_temp =
        CELSIUS_TO_FAHRENHEIT(BYTE_TO_TEMP(bus_cap_temp));

    vsr->motor_safety.pwm_status = pwm_status; // not used
    xSemaphoreGive(vsr->motor_safety_mutex);
}