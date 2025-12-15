#ifndef VITALS_DATA_H
#define VITALS_DATA_H

#include "../../programConstants.h"
#include "vitalsStructs.h"
#include <stdbool.h>
#include <stdint.h>

// used by vitals for looking up parsing info, and tracking data for every node.
// defined in vitalsStaticDec.c
extern vitalsNode nodes[numberOfNodes];
extern int16_t missingIDs[];

#endif // VITALS_DATA_H
