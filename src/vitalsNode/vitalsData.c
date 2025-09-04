#include <stdio.h>
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_timer.h"

#include "../pecan/pecan.h"   
#include "../espBase/debug_esp.h" 
#include "programConstants.h"
#include "vitalsHelper/vitalsHelper.h"
#include "vitalsHelper/vitalsStaticDec.h"

TimerHandle_t missingDataTimers [ totalNumFrames ];  //one of these timers going off trigers callback function for missing CAN Data Frane
StaticTimer_t xTimerBuffers[ totalNumFrames ];      //array for the buffers of these timers

static void vTimerCallback(TimerHandle_t xTimer);

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
            if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
                printf("failed to send warning with code %d. \n",err);
                xSemaphoreGive(printfMutex); // Release the mutex.
            }else { printf("cant print, in deadlock!\n"); }
        }
    } else{
        if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
            printf("timer Reset for node: %d, frame: %ld\n",nodeId, CanFrameNumber); 
            xSemaphoreGive(printfMutex); // Release the mutex.
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

int16_t initializeDataTimers(){ //initializes timeOuts for Data collection, as soon as this runs, we need data from every node to be sending their data to prevent them getting flagged, or Bus off if critical
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
    if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
    printf("numer of inits: %ld\n",numInits); // Call the non-reentrant function safely.
    xSemaphoreGive(printfMutex); // Release the mutex.
}else { printf("cant print, in deadlock!\n"); }
}

static void vTimerCallback(TimerHandle_t xTimer){   //called when data is not correctly recieved. Triggers extrapolation, and extrapolation warning, sent directly to telem
    CANFrame* missingFrame = ( CANFrame* ) pvTimerGetTimerID( xTimer );
    if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
        printf("missing Data frame number: %d from node %d. \n", missingFrame->frameID, missingFrame->nodeID);
        xSemaphoreGive(printfMutex); // Release the mutex.
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
        if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
            printf("failed to send status update with code %d. \n",err);
            xSemaphoreGive(printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }
    }
}