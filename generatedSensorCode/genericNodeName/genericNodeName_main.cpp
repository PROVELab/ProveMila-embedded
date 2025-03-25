#include <Arduino.h>
#include "CAN.h"
#include "../../pecan/pecan.h"                  //used for CAN
#include "../../arduinoSched/arduinoSched.hpp"  //used for scheduling
#include "../common/sensorHelper.hpp"           //used for compliance with vitals and sending data
#include "myDefines.hpp"          //contains #define statements specific to this node like myId.

PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };
PScheduler ts;
//if no special behavior, all you need to fill in the collectData<NAME>() function(s). Have them return an int32_t with the corresponding data
int32_t collect_temperature1(){
    int32_t temperature1 = 1;
    Serial.println("collecting temperature1");
    return temperature1;
}

int32_t collect_temperature2(){
    int32_t temperature2 = 2;
    Serial.println("collecting temperature2");
    return temperature2;
}

int32_t collect_temperature3(){
    int32_t temperature3 = 3;
    Serial.println("collecting temperature3");
    return temperature3;
}

int32_t collect_airPressure(){
    int32_t airPressure = 4;
    Serial.println("collecting airPressure");
    return airPressure;
}

void setup() {
	Serial.begin(9600);
	Serial.println("sensor begin");
	if (!CAN.begin(500E3)) {
		Serial.println("Starting CAN failed!");
		while (1);
	}
	vitalsInit(&plpc, &ts);
}

void loop() {
	ts.mainloop(&plpc);
}
