#ifndef SELF_POWER_SENSOR_HEADER
#define SELF_POWER_SENSOR_HEADER

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Performance Notes: reports 975mV for 1V, and 6024mV for 6V input when tested with divider of 114k and 57k resistors.
// This is close enough to expected values. The linear scaling found exactly 4V at 4V.
// It becomes widely inaccurate for <1V, (not this would be for less than 300mV after the voltage divider)
// Reporting 400mV at 0V. So long as we measure only above 1V with divider, or 300mV raw, we are accurate enough
// if we see 400mV, we can assume 0V input.
// Note: very important to have accurate resistors. With bottom resistor at 2% less than top ones,
//  the projected mesurement is ~50mV below what it should be.

typedef enum {
    SUCCESSFUL_INIT_CALIBRATED = 0,
    SUCCESSFUL_INIT_NO_CALIBRATION = 1,
    INIT_FAILURE = 2,
    READ_SUCCESS = 3,
    NOTHING_TO_READ = 4,
    READ_FAILURE = 5,
    READ_CRITICAL = 6
} selfPowerStatus_t;

#define VP_Pin 36 // ADC1_CH0
#define VN_Pin 39 // ADC1_CH3

// Per-channel configuration (same as your current single-pin struct)
typedef struct {
    int ADCPin; // e.g., 34 for GPIO34
    int R1;     // resistor between Vin and ADC (ohms)
    int R2;     // resistor between ADC and GND (ohms)
} selfPowerConfig;

/**
 * - configs: array of per-channel selfPowerConfig entries (length >= num_channels)
 * - num_channels: number of active channels (1..MAX_CHANNELS)
 * All configs must have ADCUnit == 1. ADCPin must be ADC1-capable (GPIO32–39 on ESP32).
 */
void initializeSelfPower(const selfPowerConfig* configs, int num_channels, int adc_unit,
                         selfPowerStatus_t* out_statuses);

/**
 * Drain DMA and compute Vin (mV) for all configured channels, in the same order
 * as provided to initializeSelfPower().
 * out_vin_mV: array containing mV value for each channel.
 * out_statuses: array containing err status for each channel
 */
void collectSelfPowerAllmV(int32_t* out_vin_mV, selfPowerStatus_t* out_statuses);

/** Number of channels configured at init time. */
int getSelfPowerChannelCount();

#include "../../pecan/pecan.h" //for send status updates
void selfPowerStatusCheck(const selfPowerStatus_t* statuses, int num_channels, int id);

#endif
