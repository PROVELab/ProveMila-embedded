#ifndef VITALS_STRUCTS_H
#define VITALS_STRUCTS_H

#include "../../programConstants.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#define R10(x) {x, x, x, x, x, x, x, x, x, x}

typedef struct {
    int8_t bitLength;
    int32_t minCritical;
    int32_t maxCritical;
    int32_t min;
    int32_t max;
    int32_t minWarning;
    int32_t maxWarning;
    int32_t startingValue;
} dataPoint;

typedef struct {
    int8_t nodeID;
    int8_t frameID;
    int8_t numData;
    dataPoint* dataInfo; /* Replaced list with dataPoint pointer */
    int8_t isCritical;
    int8_t flags;
    int8_t dataLocation;
    int8_t consecutiveMisses;
    int32_t dataTimeout;
    int32_t frequency;
    int32_t (*data)[10]; /* Init to [data points per data =10] [numData for this frame] */
} CANFrame;

typedef struct {
    _Atomic int8_t flags;
    _Atomic int16_t milliSeconds;
    int8_t numFrames;
    CANFrame* CANFrames;
} vitalsNode;

#endif
