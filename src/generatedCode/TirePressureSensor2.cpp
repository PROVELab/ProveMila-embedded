#include "../common/pecan.hpp"
#include "Arduino.h"
#include "CAN.h"
#include "../arch/arduino.hpp"

//universal globals
const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;
PCANListenParamsCollection plpc;
PScheduler ts;
//TP2_2 globals
const int TP2_2Id=7;
int8_t TP2_2dataArray[8];
int8_t TP2_2VitalsFlags=0;