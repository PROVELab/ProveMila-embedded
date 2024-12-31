#include "sensorHelper.hpp"
#include "../common/pecan.hpp"
#include "../arch/arduino.hpp"

#include "Arduino.h"
#include "CAN.h"
#include<stdint.h>

int16_t respondToHB(CANPacket * recv_pack){
        delay(1);
        CANPacket packet;
        packet.id =combinedID(sendPong,myId);    
        writeData(&packet, &name1VitalsFlags,  1);  
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

int8_t vitalsInit(PCANListenParamsCollection* plpc, PScheduler* ts){
    //HB response
    CANListenParam babyDuck;
    babyDuck.handler=respondToHB;
    babyDuck.listen_id =combinedID(sendPing,vitalsID);
    babyDuck.mt=MATCH_EXACT;
    
    if (addParam(plpc,babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
}
