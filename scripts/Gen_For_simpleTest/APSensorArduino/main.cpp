#include <Arduino.h>
#include <avr/wdt.h>
#include "CAN.h"
#include "../../pecan/pecan.h"                  //used for CAN
#include "../../arduinoSched/arduinoSched.hpp"  //used for scheduling
#include "../common/sensorHelper.hpp"      //used for compliance with vitals and sending data
#include "myDefines.hpp"    //contains #define statements specific to this node like myId.

PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0};
PScheduler ts;
//For Standard behavior, fill in the collectData<NAME>() function(s).
//In the function, return an int32_t with the corresponding data
int32_t collect_airPressure(){
    int32_t airPressure = 50;
                                Serial.println("collecting airPressure");
    return airPressure;
}

void setup() {
	Serial.begin(9600);
	Serial.println("sensor begin");
	wdt_enable(WDTO_2S); // enable watchdog with 2s timeout. reset in ts.mainloop
	pecanInit config={.nodeId= myId, .pin1= defaultPin, .pin2= defaultPin};
	pecan_CanInit(config);
	sensorInit(&plpc, &ts);
}

void loop() {
	wdt_reset();
	while( waitPackets(&plpc) != NOT_RECEIVED);	//handle CAN messages
	ts.execute();	//Execute scheduled tasks
}
