#include <Arduino.h>
#include "CAN.h"
#include "../../pecan/pecan.h"                  //used for CAN
#include "../../arduinoSched/arduinoSched.hpp"  //used for scheduling
#include "../common/sensorHelper.hpp"           //used for compliance with vitals and sending data
#include "myDefines.hpp"          //containts #define statements specific to this node like myId.


PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };    //use for adding Can listen parameters (may not be necessary)
PScheduler ts;                      //use for scheduling tasks (may not be necessary)
                                    //^ data collection and vitals compliance tasks are already scheduled.
    //if no special behavior, all you need to fill in the the collectData<NAME>() function(s). Have them return an int32_t with the corresponding data
int32_t collect_rizzMeter(){
    int32_t rizzMeter = 5;
    Serial.println("collecting rizzMeter");
    return rizzMeter;
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
