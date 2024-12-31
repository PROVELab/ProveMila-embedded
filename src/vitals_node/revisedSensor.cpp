#include "../common/pecan.hpp"
#include "Arduino.h"
#include "CAN.h"
#include "../arduino/arduinoSched.hpp"
#include "sensorHelper.hpp"

PCANListenParamsCollection plpc;    //use for adding Can listen parameters (may not be necessary)   
PScheduler ts;                      //use for scheduling tasks (may not be necessary)
                                    //^ data collection and vitals compliance tasks are already scheduled.
                                    //if no special behavior, all you need to fill in the the collectData<NAME>() function(s)


//universal globals
const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;

//node globals
const int name1Id=6;
int8_t name1dataArray[8];
int8_t name1VitalsFlags=0;



int16_t name1RespondToHeartBeat(CANPacket *recvPack){
        CANPacket responsePacket;
        packet.id =combinedID(sendPong,name1Id);   
        setRTR(&packet)
        name1VitalsFlags=0;
        long milliAmps=1111;
        long milliVolts=1111;
        int8_t milliVoltsArr[3]={0};
        int8_t milliAmpsArr[3]={0};
        
        milliAmps=constrain(milliAmps,-8388606L,8388606L);
        milliVolts=constrain(milliVolts,-8388606L,8388606L);
        
        memcpy(milliVoltsArr,&milliVolts,3);
        memcpy(milliAmpsArr,&milliAmps,3);
        if(milliAmps<0){    //indicate if number is negative
            bitWrite(milliAmpsArr[2],7,1);
        }
        if(milliVolts<0){
            bitWrite(milliVoltsArr[2],7,1);
        }
        writeData(&packet,milliVoltsArr,3);
        writeData(&packet,milliAmpsArr,3);
        Serial.println("N1RHB");
        sendPacket(&packet);
    return 1;
}


void setup() {
    Serial.begin(9600);
    Serial.println("senseor");
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    randomSeed(100);
    // CAN.loopback(); i think this be frickin things up sometime idrk

    vitalsInit(&plpc, &ts);
    //name1
    CANListenParam name1babyDuck;
    name1babyDuck.handler=respondToHB;
    name1babyDuck.listen_id =combinedID(sendPing,vitalsID);
    name1babyDuck.mt=MATCH_EXACT;
    
    if (addParam(&plpc,name1babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
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
    ts.scheduleTask(&name3Data1);
}

void loop() {
    delay(1000);
    Serial.println("loop start");
    ts.mainloop((PCANListenParamsCollection*) &plpc);
    //everything should be written as a task in the setup part. the ps.mainloop call never ends
    //u have up to 85.5% Ram that can be used, possibly less depending on future stuff added. for an idea of how class to current RAM u are at. just a hello World is 67%, so 76% means using about half of available space
}
