#include <Arduino.h>
#include "CAN.h"
#include "../../pecan/pecan.h"                  //used for CAN
#include "../../arduinoSched/arduinoSched.hpp"  //used for scheduling
#include "../common/sensorHelper.hpp"           //used for compliance with vitals and sending data
#include "myDefines.hpp"          //contains #define statements specific to this node like myId.

PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0};
PScheduler ts;
//if no special behavior, all you need to fill in the collectData<NAME>() function(s). Have them return an int32_t with the corresponding data
int32_t collect_rizzMeter(){
    int32_t rizzMeter = 9;
    Serial.println("collecting rizzMeter");
    return rizzMeter;
}

void setup() {
	Serial.begin(9600);
	Serial.println("sensor begin");
	pecanInit config={.nodeId= myId, .txPin= defaultPin, .rxPin= defaultPin};
	pecan_CanInit(config);
	vitalsInit(&plpc, &ts);
}

void loop() {
	ts.mainloop(&plpc);
}
