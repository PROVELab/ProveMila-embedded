#ifndef SELF_POWER_SENSOR_HEADER
#define SELF_POWER_SENSOR_HEADER

#include <string.h>
#include <stdint.h>
#include <stdio.h>

//Performance Notes: reports 975mV for 1V, and 6024mV for 6V input when tested with divider of 114k and 57k resistors.
//This is close enough to expected values. The linear scaling found exactly 4V at 4V.
//It becomes widely inaccurate for <1V, (not this would be for less than 300mV after the voltage divider)
//Reporting 400mV at 0V. So long as we measure only above 1V with divider, or 300mV raw, we are accurate enough
//if we see 400mV, we can assume 0V input.


typedef enum {
    SUCCESSFUL_INIT_CALIBRATED = 0,
    SUCCESSFUL_INIT_NO_CALIBRATION = 1,
    INIT_FAILURE = 2,
    READ_SUCCESS = 3,
    NOTHING_TO_READ = 4,
    READ_FAILURE = 5
} selfPowerStatus_t;


// Per-channel configuration (same as your current single-pin struct)
typedef struct {
    int ADCPin;          // e.g., 34 for GPIO34
    int ADCUnit;         // must be 1 (ADC1)
    int R1;              // resistor between Vin and ADC (ohms)
    int R2;              // resistor between ADC and GND (ohms)
} selfPowerConfig;



/**
 * - configs: array of per-channel selfPowerConfig entries (length >= num_channels)
 * - num_channels: number of active channels (1..MAX_CHANNELS)
 * All configs must have ADCUnit == 1. ADCPin must be ADC1-capable (GPIO32–39 on ESP32).
 */
selfPowerStatus_t initializeSelfPowerN(const selfPowerConfig *configs, int num_channels);


//returns the voltage supplied to the microcontroller (by Vin pin)
//Assumption: For both ESP and
selfPowerStatus_t collectSelfPowermV(int32_t *out_vin_mV);

#include "../../pecan/pecan.h"  //for sendStatusUpdate

//checks for errors. If theres an error, sends CAN status update, and aborts.
void selfPowerStatusCheck(selfPowerStatus_t ADC_status, int id);

#endif