#include "sensorHelper.hpp"
#include "../../pecan/pecan.h"
#include "../../arduinoSched/arduinoSched.hpp"
// #define STRINGIZE_(a) #a
// #define STRINGIZE(a) STRINGIZE_(a)

// #include STRINGIZE(../NODE_CONFIG)  //concatinates "../" to return to sensor directory, with path given build flags NODE_CONFIG


#include "Arduino.h"
#include "CAN.h"
#include<stdint.h>
/*Note: myframes is an extern variable declared in sensorHelper.hpp, and defined in <nodeName>/sensorStaticDec.cpp.
 This variable is needed to format and send CAN Data. While with the defualt implementation this variable is onlyneeded in this file,
 it is possible that sensor-specific code will want to access it in the future, which is why its defined externally*/
//int32_t (**dataCollectors)();
int32_t (*mydataCollectors[numData])(void) = {dataCollectorsList};  //the list of functions to be called for collecting data. These are to be defined in the main file for each sensor

int16_t respondToHB(CANPacket *recvPack){
        CANPacket responsePacket={{0}};
        responsePacket.id =combinedID(sendPong,myId);       //sendPong, myId
        setRTR(&responsePacket);
        Serial.println("N1RHB");
        if(sendPacket(&responsePacket)){
            Serial.println("error sending\n");
        }
    return 1;
}
void sendFrame(int8_t frameNum){
    if (frameNum<0 || frameNum>numFrames){
        Serial.println("attempted to send out of bounds frame. not sending!");
    }
    Serial.print("sending frame NO.: "); Serial.println(frameNum);
    int8_t frameNumData=myframes[frameNum].frameNumData;
    int8_t collectorFuncIndex=myframes[frameNum].startingDataIndex;
    int8_t currBit=0;
    uint8_t tempData[8]={0};
    for(int i=0;i<frameNumData;i++){//iterate over each data. Colect data from dataCollectors, and store compressed version into tempdata.
        int32_t data= mydataCollectors[collectorFuncIndex + i]();  //collects the data point
        dataPoint info=myframes[frameNum].dataInfo[i];
        uint32_t unsignedConstrained= formatValue(data,info.min, info.max);   //constraining and subtracting min forces this value to be positive
        copyValueToData(&unsignedConstrained, tempData,currBit,info.bitLength);
        currBit+=info.bitLength;
    }

    //send the packet
    CANPacket dataPacket={{0}}; 
    dataPacket.extendedID=1;
    dataPacket.id =combinedIDExtended(transmitData,myId,(uint32_t)frameNum);   
    writeData(&dataPacket,tempData,(7+currBit)/8);
    if(sendPacket(&dataPacket)){
        Serial.println("error sending\n");
    }
    //
}

PTask sendFrameTasks [numFrames];
void (*sendFrameHandlers[numFrames])(void); //each of these functions collects data for, and sends a CAN frame

template <int N>
void func() {
    sendFrame(N);
}

// Recursively defines func<1> func<2>, ... func<numFrames>, and sets the handler[i]=func<i>, ei, initializes callbacks for sendFrame tasks for any arbitrary value of numFrames.
template <int N>
struct GenerateFunctions {
    static void generate(void (*sendFrameHandlers[])(void)) {
        sendFrameHandlers[N - 1] = &func<N - 1>;   // Assign func<N-1> to the array
        GenerateFunctions<N - 1>::generate(sendFrameHandlers); // Recurse
    }
};
// Base template specialization (for N == 0)
template <>
struct GenerateFunctions<1> {
    static void generate(void (*sendFrameHandlers[])(void)) {
        sendFrameHandlers[0] = &func<0>;  // Base case: assign func<0>()
    }
};
//


//int8_t dataIndices[numData]={0};    //initialized to 0, 1, 2, 3,... in vitalsInit, used to store sendFrame Indices
int8_t vitalsInit(PCANListenParamsCollection* plpc, PScheduler* ts){
    Serial.println("initializing\n");
    // defines handlers array
    GenerateFunctions<numFrames>::generate(sendFrameHandlers);
    //schedules dataCollection + frame Sends:
    for(int i=0;i<numFrames;i++){
        sendFrameTasks[i].function=sendFrameHandlers[i];
        sendFrameTasks[i].interval=myframes[i].frequency;
        sendFrameTasks[i].delay=0;
        Serial.println("scheduling");
        ts->scheduleTask(&(sendFrameTasks[i]));
    }
    //HB response
    CANListenParam babyDuck;
    babyDuck.handler=respondToHB;
    babyDuck.listen_id =combinedID(sendPing,vitalsID);
    babyDuck.mt=MATCH_EXACT;
    //
    // //static_for<0,numFrames>()();
    // //sendFrameSpecific<1>;
    // for(int8_t i=0;i<numFrames;i++){
    //     //dataIndices[i]=i;
    //     //frameSendTasks[i].function= sendFrameSpecific<i>();
    // }
    // frame1.function=sendFrame0;
    // frame1.interval=100; //TODO: constant, should be attached to frame.
    // (*ts).scheduleTask(&frame1);
    
    if (addParam(plpc,babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
    return 0;
}

