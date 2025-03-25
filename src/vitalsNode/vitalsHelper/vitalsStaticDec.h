#ifndef VITALS_DATA_H
#define VITALS_DATA_H

#include "../programConstants.h"
#include "vitalsStructs.h"
#include <stdint.h>


// Declare the nodes array as extern so it can be accessed from other files
//extern struct vitalsData *nodes;   //used by vitals for looking up parsing info, and tracking data for every node. defined in vitalsStaticDec.c
extern struct vitalsNode nodes[numberOfNodes];
#endif // VITALS_DATA_H