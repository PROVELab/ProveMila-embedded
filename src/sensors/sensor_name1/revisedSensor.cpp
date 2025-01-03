#include <Arduino.h>
#include "CAN.h"
#include "../../pecan/pecan.h"                  //used for CAN
#include "../../arduinoSched/arduinoSched.hpp"  //used for scheduling
#include "../common/sensorHelper.hpp"           //used for compliance with vitals and sending data
#include "myDefines.hpp"          //containts #define statements specific to this node like myId.


PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };    //use for adding Can listen parameters (may not be necessary)   
PScheduler ts;                      //use for scheduling tasks (may not be necessary)
                                    //^ data collection and vitals compliance tasks are already scheduled.
                                    //if no special behavior, all you need to fill in the the collectData<NAME>() function(s)
    int32_t d1=1;
    int32_t collectData1(){
        Serial.println("collecting 1");
        return d1;
    }
    int32_t d2=0;
    int32_t collectData2(){
        Serial.println("collecting 2");
        return d2++;
    }
    int32_t d3=0;
    int32_t collectData3(){
        Serial.println("collecting 3");
        return d3--;
    }
    int32_t d4=5;
    int32_t collectData4(){
        Serial.println("collecting 4");
        return d4;
    }


void setup() {
    Serial.begin(9600);
    Serial.println("senseor");
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    // CAN.loopback(); i think this be frickin things up sometime idrk

    vitalsInit(&plpc, &ts);

    /*
    PTask name1Data1234;  
    name1Data1234.function=name1ProcessData1234;
    name1Data1234.interval=10000;
    ts.scheduleTask(&name1Data1234);

    //if applicable, add another task that will attempt adding on another data less often


    //name2
    CANListenParam name2babyDuck;
    name2babyDuck.handler=name2RespondToHeartBeat;
    name2babyDuck.listen_id =combinedID(sendPing,vitalsID);
    name2babyDuck.mt=MATCH_EXACT;
    if (addParam(&plpc,name2babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }

    PTask name2Data1234;  
    name2Data1234.function=name2ProcessData1234;
    name2Data1234.interval=10007;
    ts.scheduleTask(&name2Data1234);

    //name3
    CANListenParam name3babyDuck;
    name3babyDuck.handler=name3RespondToHeartBeat;
    name3babyDuck.listen_id =combinedID(sendPing,vitalsID);
    name3babyDuck.mt=MATCH_EXACT;
    if (addParam(&plpc,name3babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
    PTask name3Data1;  
    name3Data1.function=name3ProcessData1;
    name3Data1.interval=10015;
    ts.scheduleTask(&name3Data1);*/
}

void loop() {
    //delay(1000);
    Serial.println("op start");
    ts.mainloop( &plpc);
    //everything should be written as a task in the setup part. the ps.mainloop call never ends
    //u have up to 85.5% Ram that can be used, possibly less depending on future stuff added. for an idea of how class to current RAM u are at. just a hello World is 67%, so 76% means using about half of available space
}
