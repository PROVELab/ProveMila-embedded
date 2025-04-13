#include "../../vitalsNode/programConstants.h"
#include "sensorHelper.hpp"
#include "../../pecan/pecan.h"
//recomended for viewing this file, select one env for which SENSOR_ESP_BUILD is defined (like genericNodeNameESP)
//, and then switch to one with SENSOR_ARDUINO_BUILD defined (like genericNodeName)
//this file contains lots of code specific to each platform, but enough similar code to make it worth keeping as one file

#ifdef SENSOR_ESP_BUILD
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_timer.h"
#include "../../pecan/espBusRestart.h"
#include "../../vitalsNode/mutex_declarations.h" //sets uo static mutexes. To add another mutex, declare it in this file, and its .c file, and increment mutexCount

//Declare Timers for data collection and sending
TimerHandle_t dataCollection_Timers [ numFrames ];  //one of these timers going off trigers callback function for missing CAN Data Frane
StaticTimer_t xTimerBuffers[ numFrames ];      //array for the buffers of these timers
//

#elif defined(SENSOR_ARDUINO_BUILD)
#include "../../arduinoSched/arduinoSched.hpp"
#include "Arduino.h"
#include "CAN.h"
#endif


#ifdef SENSOR_ESP_BUILD
    #define flexiblePrint(str) mutexPrint(str)
#elif defined(SENSOR_ARDUINO_BUILD)
    #define flexiblePrint(str) Serial.print(str)
#else
    #error "Unknown build environment"
#endif


#ifdef SENSOR_ESP_BUILD
int32_t checkBus_myId=myId;    //pass this as a parameter for checkCanHandler
#define STACK_SIZE 20000    //for deciding stack size
StaticTask_t checkBus_Buffer;       //task for checkBus status, and restarting when necessary
StackType_t checkBus_Stack[ STACK_SIZE ]; 

#endif



#include<stdint.h>

 int32_t (*mydataCollectors[node_numData])(void) = {dataCollectorsList};  //the list of functions to be called for collecting data. These are to be defined in the main file for each sensor

int16_t respondToHB(CANPacket *recvPack){
        CANPacket responsePacket;
        memset(&responsePacket, 0, sizeof(CANPacket));
        // CANPacket responsePacket={0};
        responsePacket.id =combinedID(HBPong,myId);       //sendPong, myId
        setRTR(&responsePacket);
        
        flexiblePrint("N1RHB\n");
        if(sendPacket(&responsePacket)){
            flexiblePrint("error sending\n");
        }
    return 1;
}

/*Note: myframes is an extern variable declared in sensorHelper.hpp, and defined in <nodeName>/sensorStaticDec.cpp.
 This variable is needed to format and send CAN Data. While with the default implementation this variable is only needed in this file,
 however it is possible that sensor-specific code will want to access it in the future, which is why its defined externally*/

#ifdef SENSOR_ESP_BUILD
int8_t dataIndices[numFrames]={0};    //initialized to 0, 1, 2, 3,... in vitalsInit, used to store sendFrame Indices
void sendFrame(TimerHandle_t xTimer){
    int8_t frameNum = *((int8_t*) (pvTimerGetTimerID( xTimer )));
#elif defined(SENSOR_ARDUINO_BUILD)
void sendFrame(int8_t frameNum){
#endif
    if (frameNum<0 || frameNum>numFrames){
        flexiblePrint("attempted to send out of bounds frame. not sending!\n");
    }
    char buffer[50];  
    sprintf(buffer, "sending frame NO.: %d\n", frameNum);  
    flexiblePrint(buffer);  

    int8_t frameNumData=myframes[frameNum].numData;
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
    CANPacket dataPacket;
    memset(&dataPacket, 0, sizeof(CANPacket));
    dataPacket.extendedID=1;
    dataPacket.id =combinedIDExtended(transmitData,myId,(uint32_t)frameNum);   
    writeData(&dataPacket,(int8_t*) tempData,(7+currBit)/8);
    int temp=0;
    if((temp=sendPacket(&dataPacket))){
        sprintf(buffer, "error sending: %d\n", temp);  // Convert the int8_t to a string
        flexiblePrint(buffer);  
    }
    //
}
#ifdef SENSOR_ARDUINO_BUILD  //On arduino task library, task functions cant have parameters, using templates to define a distinct function for collecting each frame
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
    // Base template  (for N = 0)
    template <>
    struct GenerateFunctions<1> {
        static void generate(void (*sendFrameHandlers[])(void)) {
            sendFrameHandlers[0] = &func<0>;  // Base case: assign func<0>()
        }
    };
#endif



int8_t vitalsInit(PCANListenParamsCollection* plpc, void* ts){    //PScheduler will be NULL if not arduino
    flexiblePrint("initializing\n");

#ifdef SENSOR_ESP_BUILD
    for(int i=0;i<numFrames;i++){   //create timers for sendingData
        dataIndices[i]=i;   //initialize indices for frames.
        dataCollection_Timers[i]= xTimerCreateStatic
        (   "Timer",                                    // Just a text name, not used by the RTOS kernel. 
            pdMS_TO_TICKS(myframes[i].frequency),       //The timer period in ticks, must be greater than 0.              
            pdTRUE,                                     // The timers will auto-reload themselves when they expire. 
            (void *) &(dataIndices[i]),                    //"ID" for this function, which we use to store the corresponding Can Frame for this timer
            sendFrame,                                  /* callBack functoin, sends corresponding frame */
            &( xTimerBuffers[i] )                       //Pass in the address of a StaticTimer_t variable, which will hold the data associated with the timer being created.
        );
    }
    //start the timers:
    for(int i=0;i<numFrames;i++){
        if(xTimerStart(dataCollection_Timers[i],pdMS_TO_TICKS(1000))==pdFAIL){    //no time crunch yet, but if this isnt starting we want to be notified, instead of it to running forever
        mutexPrint("warning, unable to start a timer");
        while(1);
        } 
    }

#elif defined(SENSOR_ARDUINO_BUILD)
    PScheduler* sched=(PScheduler*)ts;
    GenerateFunctions<numFrames>::generate(sendFrameHandlers);
    //schedules dataCollection + frame Sends:
    for(int i=0;i<numFrames;i++){
        sendFrameTasks[i].function=sendFrameHandlers[i];
        sendFrameTasks[i].interval=myframes[i].frequency;
        sendFrameTasks[i].delay=0;
        flexiblePrint("scheduling");
        sched->scheduleTask(&(sendFrameTasks[i]));
    }
#endif  
    int16_t err;
    if((err=sendStatusUpdate(initFlag, myId))){
        char buffer[50];
        sprintf(buffer, "error sending Init Msg: %d\n", err);  // Convert the int8_t to a string
        flexiblePrint(buffer);  
    }

    CANListenParam babyDuck;
    babyDuck.handler=respondToHB;
    babyDuck.listen_id =combinedID(HBPing,vitalsID);
    babyDuck.mt=MATCH_EXACT;
 
    if (addParam(plpc,babyDuck)!= SUCCESS){    //plpc declared above setup()
        flexiblePrint("plpc no room");
        while(1);
    }

    TaskHandle_t checkCanHandler = xTaskCreateStaticPinnedToCore(  //prints out bus status info
        check_bus_status,       /* Function that implements the task. */
        "checkCan",          /* Text name for the task. */
        STACK_SIZE,      /* Number of indexes in the xStack array. */
        ( void * ) &checkBus_myId,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
        1,/* Priority at which the task is created. */
        checkBus_Stack,          /* Array to use as the task's stack. */
        &checkBus_Buffer,   /* Variable to hold the task's data structure. */
        tskNO_AFFINITY);  //assigns printHello to core 0

    return 0;
}

