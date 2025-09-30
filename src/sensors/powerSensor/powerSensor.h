#ifndef SELF_POWER_SENSOR_HEADER
#define SELF_POWER_SENSOR_HEADER


//gives a function

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "../../pecan/pecan.h"

// Usage as a power sensor requires a larger votage divider between the ADCPin
typedef struct{
    int ADCPin;
    int R1;     //resistor between Vin and ADC. (For Arduino, power should use Vin.)
    int R2;     //resisotr between ADC and GND
 } selfPowerConfig;

//Optionally overide the defaultValues. Hardware must always match what is specified.
// Ideally
// Default Values for Arduino: ADCPin = . R1=. R2 =
// Default Values for ESP:     ADCPin = . R1=. R2 = 
//must be called before first instance of calling collectVoltagemV
void initializeSelfPower(selfPowerConfig config);

//returns the voltage supplied to the microcontroller (by Vin pin)
//Assumption: For both ESP and
int32_t collectSelfPowermV(){

}