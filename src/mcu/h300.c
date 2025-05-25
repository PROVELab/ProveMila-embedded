#include "h300.h"

void parse_packet(PACKET_PARSE_INPUTS) {
    // H300 packets are 2.0B
    if (!msg->extd)
        return;

    // Check if the message ID matches any of the response message IDs
    switch (msg->identifier) {
    case HIGH_FREQ_DATA:
        // Handle high frequency data
        // Deserialize the data and update the VSR
        break;
    case MED_FREQ_DATA:
        // Handle medium frequency data
        // Deserialize the data and update the VSR
        break;
    case LOW_FREQ_DATA:
        // Handle low frequency data
        // Deserialize the data and update the VSR
        break;
    case CALIBRATION_RESULT_MSG:
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
    vsr->motor_speed.quadrature_current = lq * 8; // convert to Arms
    vsr->motor_speed.direct_current = direct_current * 8; // convert to Arms
    vsr->motor_speed.motor_speed = motor_speed; // already in RPM
    xSemaphoreGive(vsr->motor_speed_mutex);
}

/* 
*/
