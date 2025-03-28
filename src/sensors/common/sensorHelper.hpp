#ifndef SENSOR_HELP
#define SENSOR_HELP

 #define STRINGIZE_(a) #a
#define STRINGIZE(a) STRINGIZE_(a)


#include "../../pecan/pecan.h"
// #include "../sensor_name1/myDefines.hpp"
#include STRINGIZE(../NODE_CONFIG)  //includes node Constants
#include "../../arduinoSched/arduinoSched.hpp"
#include <stdint.h>
//universal globals. Used by every sensor
#define vitalsID 0b0000010
#define sendPing 0b0011
#define sendPong 0b0100
#define transmitData 0b0111

extern struct CANFrame myframes[numFrames];    //defined in sensorStaticDec.cpp in <sensor_name> folder

//shortened versions of vitals' structs, containing only stuff the sensors need for sending
struct dataPoint{   //could define similar 
    int8_t bitLength;   
    int32_t min;   
    int32_t max;
    int32_t data;   //the actual data stored here
};
struct CANFrame{    //identified by a 2 bit identifier 0-3 in function code
    int8_t numData; //number of data pieces in this frame
    int32_t frequency;  //how often should this CANFrame be sent (in ms)
    int8_t startingDataIndex;  //what is the starting index of data in this frame? (needed for calling appropriate collecter function)
    struct dataPoint *dataInfo; 
};
int8_t vitalsInit(PCANListenParamsCollection* plpc, PScheduler* ts);


#endif