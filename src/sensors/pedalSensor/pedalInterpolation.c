#include <stdint.h>

#include "pedalInterpolation.h"

typedef struct {
    int32_t minReadings[interpolationSteps]; // ADC pin mV at 0% for each power point
    int32_t maxReadings[interpolationSteps]; // ADC pin mV at 100% for each power point
} pedalCharacterization;
// original as recorded in lab. not found to be most accurate on breadboard. good for a reference since final circuit
// may be different as well
//      .minReadings = {1137, 1255, 1351, 1469, 1577},
//      .maxReadings = {2793, 3077, 3340, 3610, 3901},
//  },
//  {   // [0] inverted
//      .minReadings = {3535, 3885, 4235, 4586, 4954},
//      .maxReadings = {2999, 3319, 3608, 3901, 4212},
//  }
// good characterstic on Torrey's breadboard circuit (will likely want to change for final use)
// changes: 1. subtract 80, 85, 90, 95, 100 from all maxes. 2. small bump down for inverted min readings
pedalCharacterization pedalProfiles[2] = {{
                                              // [0] rising (uninverted)
                                              .minReadings = {1137, 1255, 1351, 1469, 1577},
                                              .maxReadings = {2793, 3077, 3340, 3610, 3901},
                                          },
                                          {
                                              // [0] inverted
                                              .minReadings = {3535, 3890, 4250, 4590, 4954},
                                              .maxReadings = {2999, 3319, 3608, 3901, 4212},
                                          }};

// linear interpolation
static inline float lerp(float a, float b, float t) { return a + t * (b - a); }

int32_t transformPedalReading(int32_t raw_mV, int32_t powerReading_mV, int pedalIndex) {
    const pedalCharacterization c = pedalProfiles[pedalIndex];

    // constrain characterization to min/max
    if (powerReading_mV < startingPedalmV)
        powerReading_mV = startingPedalmV; // snap to min
    else if (powerReading_mV > maxPedalmV)
        powerReading_mV = maxPedalmV;

    // locate segment and interpolation factor along power axis
    const int32_t offset = powerReading_mV - startingPedalmV;                // valid index by constain
    int idx_lo = offset / interpolationDistancemV;                           // 0..(steps-1)
    if (idx_lo >= (interpolationSteps - 1)) idx_lo = interpolationSteps - 2; // cap to last segment
    const int idx_hi = idx_lo + 1;

    const float base_lo = (float) (startingPedalmV + idx_lo * interpolationDistancemV);
    // what percent 0-1 are we between our two extrapolation points? EX: lo=5.5V; high=6V. given 5.9V, compute .8
    const float power_ratio = (float) (powerReading_mV - (int32_t) base_lo) / (float) interpolationDistancemV; //(0-1)

    // interpolate min/max at this power (first linear op along power axis)
    const float min_mV = lerp((float) c.minReadings[idx_lo], (float) c.minReadings[idx_hi], power_ratio);
    const float max_mV = lerp((float) c.maxReadings[idx_lo], (float) c.maxReadings[idx_hi], power_ratio);

    // map raw between [min,max] to percentage (second linear op along signal axis)
    float percent;
    if (max_mV == min_mV) {
        percent = 0.0f; // degenerate; choose 0
    } else if (max_mV > min_mV) {
        percent = 100.0f * ((float) raw_mV - min_mV) / (max_mV - min_mV);
    } else {
        // inverted sensor
        percent = 100.0f * (min_mV - (float) raw_mV) / (min_mV - max_mV);
    }

    // return rounded integer (but not clamped)
    return (int32_t) (percent + (percent >= 0 ? 0.5f : -0.5f));
}
