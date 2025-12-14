#ifndef CIRCULAR_ARRAY_H
#define CIRCULAR_ARRAY_H
#include <stdint.h>

typedef struct Point{
    int32_t posX;
    int32_t posY;
    int32_t posZ;
} Point;


void addDataPoint(Point pos);


#endif
