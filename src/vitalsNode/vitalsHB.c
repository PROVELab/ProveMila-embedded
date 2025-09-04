#include <stdio.h>
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_timer.h"

#include "../pecan/pecan.h"    //helper code for CAN stuff
#include "../espBase/debug_esp.h" //for checking and restarting CAN bus
#include "programConstants.h"
#include "vitalsHelper/vitalsHelper.h"
#include "vitalsHelper/vitalsStaticDec.h"

static void checkHB(void * pvParameters);
StaticTask_t checkHB_Buffer;
StackType_t checkHB_Stack[ STACK_SIZE ]; 

int64_t HBSendTime=0;
void sendHB( void * pvParameters ) {
    //creates the checkHB task
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
            if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
                printf("failed to send HB with code %d. \n",err);
                xSemaphoreGive(printfMutex); // Release the mutex.
            }else { printf("cant print, in deadlock!\n"); }
        }else{
            HBSendTime=esp_timer_get_time();
            mutexPrint("\n\nsent HB\n\n!");
            vTaskResume(processHBResp); //run task to process HB responses
            printAllData(); //for debugging, just printing data periodically to view it.
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }

    }
}


int16_t recieveHeartbeat(CANPacket* message){    //mark the HB for given node as recieved, recording time to respond
    mutexPrint("recieved Pong\n");
    int16_t nodeId=IDTovitalsIndex(message->id);
    nodes[nodeId].flags |= HBFlag;
    nodes[nodeId].milliSeconds =esp_timer_get_time()-HBSendTime;
    return 0;
}

static void checkHB(void * pvParameters){
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
                if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
                    printf("failed to send status update with code %d. \n",err);
                    xSemaphoreGive(printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
            }
        }
        vTaskSuspend(NULL);
    }

}