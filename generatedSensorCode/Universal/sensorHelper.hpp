#ifndef SENSOR_HELP
#define SENSOR_HELP

#ifdef __cplusplus
extern "C" //Need C linkage since ESP uses C "C"
#endif
#endif#include "../../vitalsNode/programConstants.h"#define STRINGIZE_(a) #a
#define STRINGIZE(a) STRINGIZE_(a)
#include STRINGIZE(../NODE_CONFIG)  //includes node Constants

#include "../../pecan/pecan.h"
#include "../../arduinoSched/arduinoSched.hpp"
#include <stdint.h>

//universal globals. Used by every sensor
extern CANFrame myframes[numFrames];    //defined in sensorStaticDec.cpp in <sensor_name> folder

//shortened versions of vitals structs, containing only stuff the sensors need for sending
typedef struct{
    int8_t bitLength;
    int32_t min;
    int32_t max;
    int32_t data;   //the actual data stored here
} dataPoints;

typedef struct{    //identified by a 2 bit identifier 0-3 in function code
    int8_t numData;
    int32_t frequency;
    int8_t startingDataIndex;  //what is the starting index of data in this frame? (needed for calling appropriate collector function)
    dataPoint *dataInfo;
} CANFrame;
int8_t vitalsInit(PCANListenParamsCollection* plpc, void* ts);  //for arduino, this should be a PScheduler*. Otherwise, just pass Null
#ifdef __cplusplus
}  // End extern "C"
#endif
#endif