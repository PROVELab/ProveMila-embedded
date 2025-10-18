#include <Arduino.h>
#include <avr/wdt.h>
#include "CAN.h"
#include "../../pecan/pecan.h"                  //used for CAN
#include "../../arduinoSched/arduinoSched.hpp"  //used for scheduling
#include "../common/sensorHelper.hpp"           //used for compliance with vitals and sending data
#include "myDefines.hpp"          //contains #define statements specific to this node like myId.

PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0};
PScheduler ts;

int32_t collect_pedalReadingOne(){
    int32_t pedalReadingOne = 50;
    Serial.println("c pedalReadingOne");
    return pedalReadingOne;
}

int32_t collect_pedalReadingTwo(){
    int32_t pedalReadingTwo = 50;
    Serial.println("c pedalReadingTwo");
    return pedalReadingTwo;
}

void setup() {
	wdt_reset();
	Serial.begin(9600);
	while(!Serial);
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
