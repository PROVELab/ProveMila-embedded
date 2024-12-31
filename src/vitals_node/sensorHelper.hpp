#ifndef SENSOR_HELP
#define SENSOR_HELP

#include "../common/pecan.hpp"
#include "../arch/arduino.hpp"

#include<stdint.h>

//universal globals
const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;

//node globals
const int myId=6;
int8_t name1dataArray[8];
int8_t name1VitalsFlags=0;

int8_t vitalsInit(PCANListenParamsCollection* plpc, PScheduler* ts);

#endif