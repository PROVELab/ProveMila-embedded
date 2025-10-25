#ifndef PEDAL_INTERPOLATION_H
#define PEDAL_INTERPOLATION_H

#include <stdint.h>

// We interpret between 5.0, 5.5, 6.0, 6.5, 7.0 V
#define interpolationSteps      5
#define interpolationDistancemV 500  // 0.5 V per step
#define startingPedalmV         5000 // 5.0 V baseline
#define maxPedalmV              (startingPedalmV + (interpolationSteps * interpolationDistancemV))

#define risingPedalIndex  0 // pedal1
#define fallingPedalIndex 1 // pedal2

// convert raw_mV to a percentage for pedal reading
// - raw_mV: measured at the ADC pin (after divider/etc.)
// - powerReading_mV: supply (e.g. pedal power) in mV
// - c: characterization at 5.0, 5.5, 6.0, 6.5, 7.0 V
// behavior:
//   * if power in [4.5, 5.0) snaps to 5.0
//   * if power in (7.0, 7.5] snaps to 7.0
//   * outside [4.5, 7.5] still snaps to nearest endpoint (no status here)
int32_t transformPedalReading(int32_t raw_mV, int32_t powerReading_mV, int pedalIndex);

#endif
