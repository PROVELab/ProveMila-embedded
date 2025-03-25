#ifndef VITALS_STRUCTS_H
#define VITALS_STRUCTS_H

#include <stdio.h>
#include<stdint.h>
#include "../programConstants.h"
#include "vitalsStaticDec.h"
#define R10(x) {x,x,x,x,x,x,x,x,x,x}

struct dataPoint {
    int8_t bitLength;
    int8_t isCritical;
    int32_t minCritical;
    int32_t maxCritical;
    int32_t min;
    int32_t max;
    int32_t minWarning;
    int32_t maxWarning;
    int32_t startingValue;
};

struct CANFrame {
    int8_t nodeID;
    int8_t frameID;
    int8_t frameNumData;
    list dataInfo;
    int8_t flags;
    int8_t dataLocation;
    int8_t consecutiveMisses;
    int32_t dataTimeout;
    int32_t frequency;
};

struct vitalsNode {
    int8_t flags;
    int16_t milliSeconds;
    int8_t numFrames;
    list CANFrames;
};

#endif
