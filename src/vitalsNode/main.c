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
#include "vitalsData.h"
#include "vitalsHB.h"

// Initialize space for each task
StaticTask_t sendHB_Buffer;
StackType_t sendHB_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t checkStatus_Buffer;
StackType_t checkStatus_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack


void vitals_check_bus_status(void * pvParameters){ //should send all this to telem
    for(;;){
        twai_status_info_t status_info;
        esp_err_t err;
        if((err=twai_get_status_info(&status_info))){
        if (xSemaphoreTake(printfMutex, portMAX_DELAY)) { 
            printf("Messages to TX: %lu\n", status_info.msgs_to_tx);
            printf("Messages to RX: %lu\n", status_info.msgs_to_rx);
            printf("TX Error Counter: %lu\n", status_info.tx_error_counter);
            printf("RX Error Counter: %lu\n", status_info.rx_error_counter);
            printf("TX Failed Count: %lu\n", status_info.tx_failed_count);
            printf("RX Missed Count: %lu\n", status_info.rx_missed_count);
            printf("RX Overrun Count: %lu\n", status_info.rx_overrun_count);
            printf("Arbitration Lost Count: %lu\n", status_info.arb_lost_count);
            printf("Bus Error Count: %lu\n", status_info.bus_error_count);
            xSemaphoreGive(printfMutex); // Release the mutex.
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
                if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
                    printf("failed to send status update with code %d. \n",err);
                    xSemaphoreGive(printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void printAllData(){    //not for final use. for testing only
    if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
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
        xSemaphoreGive(printfMutex); // Release the mutex.
    }else { printf("cant print, in deadlock!\n"); }
}

void recieveMSG(){   //prints information about and contents of every recieved message
    CANPacket message; //will store any recieved message
    //an array for matching recieved Can Packet's ID's to their handling functions. MAX length set to 20 by default initialized to default values
    PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };

    //HB process listen Param
    CANListenParam processBeat;
    processBeat.handler=recieveHeartbeat;
    processBeat.listen_id =combinedID(HBPong,vitalsID);   //setting vitals ID doesnt matter, just checking function
    processBeat.mt=MATCH_FUNCTION; //MATCH_EXACT to make id and function code require match. MATCH_ID for same 7 bits of node ID. MATCH_FUNCTION for same 4 bits of function code
    if (addParam(&plpc,processBeat)!= SUCCESS){ //adds the parameter
        mutexPrint("plpc no room");
        while(1);
    }
    //

    //Data process listen Param
    initializeDataTimers(); //initialize timers needed to moniter data
    CANListenParam processData;
    processData.handler=moniterData;
    processData.listen_id =combinedID(transmitData,vitalsID);   //setting vitals ID doesnt matter, just checking function
    processData.mt=MATCH_FUNCTION; //MATCH_EXACT to make id and function code require match. MATCH_ID for same 7 bits of node ID. MATCH_FUNCTION for same 4 bits of function code
    if (addParam(&plpc,processData)!= SUCCESS){ //adds the parameter
        mutexPrint("plpc no room");
        while(1);
    }
    //

    for(;;){
        waitPackets(&message, &plpc);
    }
}
void app_main(void){    
    base_ESP_init();
    pecanInit config= {.nodeId=vitalsID, .txPin=defaultPin, .rxPin=defaultPin};
    pecan_CanInit(config);
    

    TaskHandle_t sendHandler = xTaskCreateStaticPinnedToCore(  //schedules the task to run the printHello function, assigned to core 0
                     sendHB,       /* Function that implements the task. */
                     "HeartBeatSend",          /* Text name for the task. */
                     STACK_SIZE,      /* Number of indexes in the xStack array. */
                     ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                     3,/* Priority at which the task is created. */
                     sendHB_Stack,          /* Array to use as the task's stack. */
                     &sendHB_Buffer,   /* Variable to hold the task's data structure. */
                     tskNO_AFFINITY);  

    TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore(  //recieves CAN Messages 
                      recieveMSG,       /* Function that implements the task. */
                      "msgRecieve",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      tskIDLE_PRIORITY,/* Priority at which the task is created. */
                      recieveMSG_Stack,          /* Array to use as the task's stack. */
                      &recieveMSG_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  

    TaskHandle_t checkCanHandler = xTaskCreateStaticPinnedToCore(  //prints out bus status info
                      vitals_check_bus_status,       /* Function that implements the task. */
                      "checkCan",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      1,/* Priority at which the task is created. */
                      checkStatus_Stack,          /* Array to use as the task's stack. */
                      &checkStatus_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  
}