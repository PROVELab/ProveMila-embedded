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
//TEMP1_1 globals
const int TEMP1_1Id=8;
int8_t TEMP1_1dataArray[8];
int8_t TEMP1_1VitalsFlags=0;long readMilliAMps(){
