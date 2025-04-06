#include <stdio.h>
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_timer.h"

#include "mutex_declarations.h" //sets uo static mutexes. To add another mutex, declare it in this file, and its .c file, and increment mutexCount
#include "../pecan/pecan.h"    //helper code for CAN stuff
#include "programConstants.h"
#include "vitalsHelper/vitalsHelper.h"
#include "vitalsHelper/vitalsStaticDec.h"

//tracing functionality to give warning for free or alloc.
#include "esp_heap_caps.h"

int init=0;//indicates that mutexes have been initialized, so we may print a warning for allocations and frees
void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps){  //is called every time memory is allocated, feature enabled in menuconfig
    if(init){
        mutexPrint("Warning, allocating memory!\n");
    }
}
void esp_heap_trace_free_hook(void* ptr){
    if(init){
        mutexPrint("Warning, freeing memory!\n");
    }
}
//

//initialize space for each task
#define STACK_SIZE 20000    //for deciding stack size, could check this out later, or not: https://www.freertos.org/Why-FreeRTOS/FAQs/Memory-usage-boot-times-context#how-big-should-the-stack-be
StaticTask_t sendHB_Buffer;
StackType_t sendHB_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t checkStatus_Buffer;
StackType_t checkStatus_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t checkHB_Buffer;
StackType_t checkHB_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack

TimerHandle_t missingDataTimers [ totalNumFrames ];  //one of these timers going off trigers callback function for missing CAN Data Frane
StaticTimer_t xTimerBuffers[ totalNumFrames ];      //array for the buffers of these timers

//Making nodes of vitals data struct
int16_t startBus(){  //should only be called on start up and AFTER Bus has finished recovery
    mutexPrint("Attempting StartBus\n\n\n");
    int err=twai_start();
    if(err!=ESP_OK){    //restarts program in the event that we can't start the Bus. This should never happen 
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
            printf("Failed to start or restart driver with code: %d. Rebooting\n",err);
            xSemaphoreGive(*printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }
        esp_restart();
    } else{
        mutexPrint("Driver Started\n\n\n\n");
        return 1;
    }
    return 0;
}


void check_bus_status(void * pvParameters){ //should send all this to telem
    for(;;){
        twai_status_info_t status_info;
        esp_err_t err;
        if((err=twai_get_status_info(&status_info))){
            if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                printf("aquire CAN status error code: %d\n",err);
                xSemaphoreGive(*printfMutex); // Release the mutex.
            }else { printf("cant print, in deadlock!\n"); }

            vTaskDelay(1000/portTICK_PERIOD_MS);        //if unable to get status info, just try again next time
            continue;
        }
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
            printf("TWAI Status Information:\n");
            printf("State: %d\n", status_info.state); // 
            xSemaphoreGive(*printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }

        if(status_info.state==TWAI_STATE_BUS_OFF){
            mutexPrint("initiating recovery\n");
            int recover;
            if ((recover=twai_initiate_recovery())!=ESP_OK){
                mutexPrint("invalid recovery attempting to reboot. This should never happen\n");    //this should only be called here when Bus is off state, and we never unistall the driver, so error should not be possible
                esp_restart();
            }
        } else if (status_info.state==TWAI_STATE_STOPPED){  //Presumably we have finished recovery
            mutexPrint("Recovery attempt Complete\n\n\n");            
            if(startBus()){
                int16_t err;
                if((err= sendStatusUpdate(canRecoveryFlag, vitalsID))){
                    char buffer[50];
                    sprintf(buffer, "error sending CAN recovery Msg: %d\n", err);  // Convert the int8_t to a string
                    mutexPrint(buffer);  
                }
            }

        }else{  //TWAI is running
             //print status info as is
            if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) { 
                printf("Messages to TX: %lu\n", status_info.msgs_to_tx);
                printf("Messages to RX: %lu\n", status_info.msgs_to_rx);
                printf("TX Error Counter: %lu\n", status_info.tx_error_counter);
                printf("RX Error Counter: %lu\n", status_info.rx_error_counter);
                printf("TX Failed Count: %lu\n", status_info.tx_failed_count);
                printf("RX Missed Count: %lu\n", status_info.rx_missed_count);
                printf("RX Overrun Count: %lu\n", status_info.rx_overrun_count);
                printf("Arbitration Lost Count: %lu\n", status_info.arb_lost_count);
                printf("Bus Error Count: %lu\n", status_info.bus_error_count);
                xSemaphoreGive(*printfMutex); // Release the mutex.
            }else { printf("cant print, in deadlock!\n"); }

            //send Status update
            int8_t frameNumData=8;
            int8_t currBit=0;
            static uint32_t errCnt=0, txFails=0, rxOverrun=0, rxMissed=0;   //records previous value
            uint32_t dataPoints[8]={(uint32_t)status_info.state, status_info.tx_error_counter, status_info.rx_error_counter, status_info.bus_error_count-errCnt,
                                    status_info.tx_failed_count-txFails, status_info.rx_overrun_count-rxOverrun, status_info.rx_missed_count-rxMissed, status_info.msgs_to_rx};
            const int8_t bitLengths[8]={2,8,8,12,10,10,10,4};
            const int32_t dataMaxes[8]={3, 255, 255,4095, 1023, 1023, 1023, 15};
            errCnt=status_info.bus_error_count; //record current value for next time
            txFails=status_info.tx_failed_count;
            rxOverrun=status_info.rx_overrun_count;
            rxMissed=status_info.rx_missed_count;
            int8_t tempData[8]={0};
            for(int i=0;i<frameNumData;i++){//iterate over each data. Colect data from dataCollectors, and store compressed version into tempdata.
                uint32_t unsignedConstrained= formatValue(dataPoints[i],0, dataMaxes[i]);   //constraining and subtracting min forces this value to be positive
                copyValueToData(&unsignedConstrained, (uint8_t*)tempData,currBit,bitLengths[i]);
                currBit+=bitLengths[i];
            }
            //Send HB
            CANPacket message={0};
            writeData(&message, tempData, 8);
            message.id=combinedID(warningCode,vitalsID); 
            int err;
            if((err=sendPacket(&message))){
                if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                    printf("failed to send status update with code %d. \n",err);
                    xSemaphoreGive(*printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
            }
        }

        if(status_info.state==TWAI_STATE_BUS_OFF){  //check constantly if recovery is complete and we are ready to restart CAN in the event of Bus off
            mutexPrint("NOTICE: Bus Off\n");
            vTaskDelay(10/portTICK_PERIOD_MS);
        }else{
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    }
}

    void printAllData(){    //not for final use. for testing only
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {

            for(int i=0;i<numberOfNodes;i++){   //each node
                printf("printData: node (vitalsId): %d, numFrams: %d\n",i, (nodes[i]).numFrames);
                for (int8_t j=0;j<nodes[i].numFrames;j++){  //each frame
                    printf("frameInfo: id: %d  numData: %d\n",j, ((nodes[i]).CANFrames[j]).numData);
                    if((nodes[i]).CANFrames ==NULL){
                        printf("error printg, framesptr not initialized, terminating\n");
                        return;
                    }
                    for(int8_t k=0;k<(((nodes[i]).CANFrames)[j]).numData;k++){ //each data
                        printf("node: %d. frame: %d. datanum: %d data: ",i,j,k);
                        for(int l=0;l<pointsPerData;l++){
                            printf("%ld ",nodes[i].CANFrames[j].data[k][l]);
                        }
                        printf("\n");
                    }
                }
            }
            xSemaphoreGive(*printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }
    }
    //
    void checkHB(void * pvParameters){
        for(;;){
            vTaskDelay(250/portTICK_PERIOD_MS); //give nodes 250ms to respond
            mutexPrint("\n\nchecking HB\n\n\n");
            for(int i=0;3*i<numberOfNodes;i++){
                CANPacket message={0};
                message.id=combinedID(HBRespUpdate,vitalsID); 
                uint8_t tempData[8]={0};
                int8_t currBit=0;
                for(int j=0;(3*i) + j<3;j++){   //sending in groups of 3 nodes
                                    //ID                           //responded?                         //time to respond
                    uint32_t value= vitalsIndexToID((3*i) +j) | (((nodes[3*i+j]).flags & HBFlag)<<7) | ((nodes[3*i+j]).milliSeconds & ((0b1<<10)-1) <<8) ; 
                    copyValueToData(&value, tempData,currBit,18);                
                    currBit+=18;
                }
                writeData(&message,(int8_t*)&tempData, (warningFrameNumBitsOffset+maxFrameCntBits+maxDataInFrameBits)>>3);  //relies on esp32 being little endian (since interpretting uint32_t as array of bytes)
                int err;
                if((err=sendPacket(&message))){
                    if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                        printf("failed to send status update with code %d. \n",err);
                        xSemaphoreGive(*printfMutex); // Release the mutex.
                    }else { printf("cant print, in deadlock!\n"); }
                }
            }
            vTaskSuspend(NULL);
        }

    }
    int64_t HBSendTime=0;
    void sendHB( void * pvParameters )    {
            //creates the task that process HB responses 
            TaskHandle_t processHBResp = xTaskCreateStaticPinnedToCore(  //checksHB responses
                    checkHB,       /* Function that implements the task. */
                    "checkHeartBeatResponses",          /* Text name for the task. */
                    STACK_SIZE,      /* Number of indexes in the xStack array. */
                    ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                    tskIDLE_PRIORITY,/* Priority at which the task is created. */
                    checkHB_Stack,          /* Array to use as the task's stack. */
                    &checkHB_Buffer,   /* Variable to hold the task's data structure. */
                    tskNO_AFFINITY);  //assigns printHello to core 0
        
        for( ;; ) {
            //Send HB
            CANPacket message={0};
            setRTR(&message);
            message.id=combinedID(HBPing,vitalsID); //HBPing, vitalsID
            int err;
            if((err=sendPacket(&message))){
                if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                    printf("failed to send HB with code %d. \n",err);
                    xSemaphoreGive(*printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
                vTaskDelay(10/portTICK_PERIOD_MS);  //try to send HB again in 10ms
            }else{
                HBSendTime=esp_timer_get_time();
                mutexPrint("\n\nsent HB\n\n!");
                vTaskResume(processHBResp); //run task to process HB responses
                printAllData(); //for debugging, just printing data periodically to view it.
                vTaskDelay(1000/portTICK_PERIOD_MS);
            }

        }
    }
    
    void vTimerCallback(TimerHandle_t xTimer){   //called when data is not correctly recieved. Triggers extrapolation, and extrapolation warning, sent directly to telem
        CANFrame* missingFrame = ( CANFrame* ) pvTimerGetTimerID( xTimer );
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
            printf("missing Data frame number: %d from node %d. \n", missingFrame->frameID, missingFrame->nodeID);
            xSemaphoreGive(*printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }
        //Add code or fnct call here to trigger extrapolation

        //Send warningFrame:
                       //problem node    //extrapolation flag        (mask                    <-ID of missingFrame->       offset      )
        uint32_t data=(missingFrame->nodeID) | (0b1<<10) | (( ((1<<maxFrameCntBits) - 1) & missingFrame->frameID)<<warningFrameNumBitsOffset);
        CANPacket message={0};
        writeData(&message,(int8_t*)&data, (warningFrameNumBitsOffset+maxFrameCntBits+maxDataInFrameBits)>>3);  //relies on esp32 being little endian (since interpretting uint32_t as array of bytes)
        message.id=combinedID(warningCode,vitalsID); 
        int err;
        if((err=sendPacket(&message))){
            if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                printf("failed to send status update with code %d. \n",err);
                xSemaphoreGive(*printfMutex); // Release the mutex.
            }else { printf("cant print, in deadlock!\n"); }
        }
    }
    int16_t initializeTimers(){ //initializes timeOuts for Data collection, as soon as this runs, we need data from every node to be sending their data to prevent them getting flagged, or Bus off if critical
        int32_t numInits=0;
        mutexPrint("initializing Timers\n");
        for(int i=0;i<numberOfNodes;i++){
            for(int j=0;j<nodes[i].numFrames;j++){
                missingDataTimers[numInits]= xTimerCreateStatic
                  ( /* Just a text name, not used by the RTOS kernel. */
                    "Timer",
                    /* The timer period in ticks, must be greater than 0. */
                    pdMS_TO_TICKS(nodes[i].CANFrames[j].dataTimeout),
                    /* The timers will auto-reload themselves when they expire. */
                    pdTRUE,
                    /* The ID is used to store a count of the number of times
                       the timer has expired, which is initialised to 0. */
                    ( void * ) &(nodes[i].CANFrames[j]),    //"ID" for this function, which we use to store the corresponding Can Frame for this timer
                    /* Each timer calls the same callback when it expires. */
                    vTimerCallback,
                    /* Pass in the address of a StaticTimer_t variable, which
                       will hold the data associated with the timer being
                       created. */
                    &( xTimerBuffers[numInits] )
                  );
                  if(missingDataTimers[numInits]==NULL){
                    mutexPrint("Error creating timer, aborting\n");
                    while (1);
                  }
                numInits++;
            }
        }
        //start the timers:
        for(int i=0;i<numInits;i++){
            if(xTimerStart(missingDataTimers[i],pdMS_TO_TICKS(1000))==pdFAIL){    //no time crunch yet, but if this isnt starting we want to be notified, instead of it to running forever
                mutexPrint("warning, unable to start a timer");
                while(1);
            } 
        }
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
        printf("numer of inits: %ld\n",numInits); // Call the non-reentrant function safely.
        xSemaphoreGive(*printfMutex); // Release the mutex.
    }else { printf("cant print, in deadlock!\n"); }
        return 0;
    }
    int16_t moniterData(CANPacket* message){ //for now just stores the data (printing the past 10 node-frame- data (past 10) on each line)
        
        mutexPrint("recievingData\n");
        int16_t nodeId=IDTovitalsIndex(message->id);
        if(nodeId>numberOfNodes){
            mutexPrint("recieved data from invalid nodeId, ignoring\n");
            return 1;
        }

        vitalsNode* node = &(nodes[nodeId]); //the node which sent the message

        uint32_t CanFrameNumber=getDataFrameId(message->id);    //the Can frame index is stored in extension
        if(CanFrameNumber>node->numFrames){
            mutexPrint("invalid dataFrame. Ignoring data\n");
            return 1;
        }
        char str[15]; // Enough to store "-128" and null terminator
        sprintf(str, "%ld, %d", CanFrameNumber, nodeId);
        mutexPrint(str);
        CANFrame* frame=& (node->CANFrames[CanFrameNumber]);   //the frame this data corresponds to
        //mark this data as collected.
        sprintf(str, "%d", frame->frameID);
        mutexPrint(str);
        mutexPrint("markingFrame\n");

        if(xTimerReset(missingDataTimers[frame->frameID],pdMS_TO_TICKS(10))==pdFAIL){    //wait up to 10ms to reset timer
            mutexPrint("warning, unable to reset timer");
            //Send warningFrame:
                       //problem node    //failedTimer flag        (mask                    <-ID of missingFrame->       offset      )
            uint32_t data=(message->id) | (0b1<<11) | (( ((1<<maxFrameCntBits) - 1) & CanFrameNumber)<<warningFrameNumBitsOffset);
            CANPacket message={0};
            writeData(&message,(int8_t*)&data, (warningFrameNumBitsOffset+maxFrameCntBits+maxDataInFrameBits)>>3);  //relies on esp32 being little endian (since interpretting uint32_t as array of bytes)
            message.id=combinedID(warningCode,vitalsID); 
            int err;
            if((err=sendPacket(&message))){
                if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                    printf("failed to send warning with code %d. \n",err);
                    xSemaphoreGive(*printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
            }
        } else{
            if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                printf("timer Reset for node: %d, frame: %ld\n",nodeId, CanFrameNumber); 
                xSemaphoreGive(*printfMutex); // Release the mutex.
            }else { printf("cant print, in deadlock!\n"); }
        }
        mutexPrint("timersSet\n");


        //parse each data from frame
        int8_t bitIndex=0;    //which bit of CANFrame we are currently reading from (as we iterate through the data)
        for(int i=0;i<(*frame).numData;i++){
            dataPoint* dataInfo=& (((*frame).dataInfo)[i]);
            uint32_t temp=0;
            copyDataToValue(&temp,message->data,bitIndex,dataInfo->bitLength);
            int32_t recvdata = ((int32_t)temp) + dataInfo->min;

            frame->data[i][frame->dataLocation]= recvdata;  //update the data
            sprintf(str, "recD: %ld", recvdata);
            mutexPrint(str);
            //increment bitIndex
            bitIndex+=dataInfo->bitLength;
        }
        //increment dataLocation, mark that we have recorded the data:
        frame->dataLocation++;  //increment the dataIndex
        if(frame->dataLocation==10){frame->dataLocation=0;}
        frame->consecutiveMisses=0;
        return 0;
    }
    int16_t recieveHeartbeat(CANPacket* message){    //mark the HB for given node as recieved, recording time to respond
        mutexPrint("recieved Pong\n");
        int16_t nodeId=IDTovitalsIndex(message->id);
        nodes[nodeId].flags |= HBFlag;
        nodes[nodeId].milliSeconds =esp_timer_get_time()-HBSendTime;
        return 0;
    }
    void recieveMSG(){   //prints information about and contents of every recieved message
        CANPacket message; //will store any recieved message
        //an array for matching recieved Can Packet's ID's to their handling functions. MAX length set to 20 by default initialized to default values
        PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };
        //declare parameters here, each param has 3 entries. When recieving a msg whose id matches 'listen_id' according to 'mt', 'handler' is called.
        CANListenParam processBeat;
        processBeat.handler=recieveHeartbeat;
        processBeat.listen_id =combinedID(HBPong,vitalsID);   //setting vitals ID doesnt matter, just checking function
        processBeat.mt=MATCH_FUNCTION; //MATCH_EXACT to make id and function code require match. MATCH_ID for same 7 bits of node ID. MATCH_FUNCTION for same 4 bits of function code
        if (addParam(&plpc,processBeat)!= SUCCESS){ //adds the parameter
            mutexPrint("plpc no room");
            while(1);
        }
        initializeTimers(); //initialize timers needed to moniter data
        CANListenParam processData;
        processData.handler=moniterData;
        processData.listen_id =combinedID(transmitData,vitalsID);   //setting vitals ID doesnt matter, just checking function
        processData.mt=MATCH_FUNCTION; //MATCH_EXACT to make id and function code require match. MATCH_ID for same 7 bits of node ID. MATCH_FUNCTION for same 4 bits of function code
        if (addParam(&plpc,processData)!= SUCCESS){ //adds the parameter
            mutexPrint("plpc no room");
            while(1);
        }
        //this task will the call the appropriate ListenParams function when a CAN message is recieved
        for(;;){
            waitPackets(&message, &plpc);
            taskYIELD();    //task runs constantly since no delay, but on lowest priority, so effectively runs in the background
        }
    }
void app_main(void){    
    //Initialize configuration structures using macro initializers
    //a hasty testing of each pin found each numbered PIN un the board (0-35) worked for TWAI for each number that appears on the board, except for pin 34, and 35, which I don't beleive are actually GPIO pins, since the datasheet says the MCU has only 34 pins, so having a pin 35 wouldnt make much sense.
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_32, TWAI_MODE_NORMAL); //TWAI_MODE_NORMAL for standard behavior  
    // twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_32, TWAI_MODE_LISTEN_ONLY); //TWAI_MODE_NORMAL for standard behavior  
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    //Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    //Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }

    mutexInit();    //initialize mutexes
    init=1;

    int16_t err;
    if((err= sendStatusUpdate(canRecoveryFlag, vitalsID))){
        char buffer[50];
        sprintf(buffer, "error sending CAN recovery Msg: %d\n", err);  // Convert the int8_t to a string
        mutexPrint(buffer);  
    }

    TaskHandle_t sendHandler = xTaskCreateStaticPinnedToCore(  //schedules the task to run the printHello function, assigned to core 0
                     sendHB,       /* Function that implements the task. */
                     "HeartBeatSend",          /* Text name for the task. */
                     STACK_SIZE,      /* Number of indexes in the xStack array. */
                     ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                     3,/* Priority at which the task is created. */
                     sendHB_Stack,          /* Array to use as the task's stack. */
                     &sendHB_Buffer,   /* Variable to hold the task's data structure. */
                     tskNO_AFFINITY);  //assigns printHello to core 0

    TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore(  //recieves CAN Messages 
                      recieveMSG,       /* Function that implements the task. */
                      "msgRecieve",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      tskIDLE_PRIORITY,/* Priority at which the task is created. */
                      recieveMSG_Stack,          /* Array to use as the task's stack. */
                      &recieveMSG_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  //assigns printHello to core 0

    TaskHandle_t checkCanHandler = xTaskCreateStaticPinnedToCore(  //prints out bus status info
                      check_bus_status,       /* Function that implements the task. */
                      "checkCan",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      1,/* Priority at which the task is created. */
                      checkStatus_Stack,          /* Array to use as the task's stack. */
                      &checkStatus_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  //assigns printHello to core 0
    //vTaskStartScheduler();      /do not write vTaskStartScheduler anywhere if using IDF FreeRTOS, the scheduler begins running on initialization, we cant toggle it, and program crashes if you attempt to.
}