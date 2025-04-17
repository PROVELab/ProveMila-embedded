#include <stdint.h>

#include "../espMutexes/mutex_declarations.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include <string.h> // memcpy
#include "pecan.h"

//Simplified Can functionality Functions: feel free to never use these, and just use them as sample code for ESP-CAN library:
int16_t defaultPacketRecv(CANPacket* p){
    if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
        printf("Default handler: id %ld\n with data:", p->id);
        for(int i=0;i<p->dataSize;i++){
            printf("% d",(p->data)[i]);
        }
        printf("\n");
        xSemaphoreGive(*printfMutex); // Release the mutex.
    }else { printf("cant print, in deadlock!\n"); }
    return 0;
}

bool (*matcher[3])(uint32_t, uint32_t) = {exact, matchID, matchFunction};   //could alwys be moved back to pecan.h as an extern variable if its needed elsewhere? I am not sure why this was declared there in the first place

int16_t waitPackets(CANPacket *recv_pack, PCANListenParamsCollection *plpc) {
    if (recv_pack == NULL) { //if no empyt packet provided, exit early. 
        return 1;
    }

    CANListenParam clp;
    twai_message_t twaiMSG;
    if (twai_receive(&twaiMSG, pdMS_TO_TICKS(0)) == ESP_OK) {   //check for message without blocking (0 ms blocking)
        
        //construct CANPacket
        if (twaiMSG.extd) {
            recv_pack->id =twaiMSG.identifier & 0xFFFFFFF;   //view first 28 bits of id
        } else{
            recv_pack->id =twaiMSG.identifier & 0b1111111111;    //view first 11 bits of id
        }
        memset(recv_pack->data, 0, 8);  //re-initialize data to all 0.
        if(twaiMSG.rtr){    //for rtr packets, no need to look at data or data-size
            recv_pack->rtr=1;
            recv_pack->dataSize=0;
            memset(recv_pack->data, 0,8);
        }else{  //not an rtr packet, copy the data_length and size
            recv_pack->rtr=0;
            recv_pack->dataSize=twaiMSG.data_length_code;
            memcpy(recv_pack->data, twaiMSG.data,recv_pack->dataSize);
        }

        // Then match the packet id with our params; if none
        // match, use default handler
        for (int16_t i = 0; i < plpc->size; i++) {
            clp = plpc->arr[i];
            if (matcher[clp.mt](recv_pack->id, clp.listen_id)) {
                return clp.handler(recv_pack);
            }
        }
        return plpc->defaultHandler(recv_pack);
        
    }
    return NOT_RECEIVED;
}

int16_t sendPacket(CANPacket *p) {
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        return PACKET_TOO_BIG;
    }
 twai_message_t message = {  //This is the struct used to create and send a CAN message. Generally speaking, only the last 3 fields should ever change.
        .extd= (p->id)>0b11111111111,              // Standard vs extended format. makes message extended if
        .rtr = p->rtr,               // Data vs RTR frame. 
        .ss = 0,                // Whether the message is single shot (i.e., does not repeat on error)
        .self = 0,              // Whether the message is a self reception request (loopback)
        .dlc_non_comp = 0,      // DLC is less than 8  I beleive, for our purposes, this should always be 0, we want to be compliant with 8 byte data frames, and not confuse arduino guys
        .identifier=p->id,  //id of vitals Heart Beat Ping
        .data_length_code = p->dataSize,
    };
    memcpy(message.data, p->data, p->dataSize); //copy data into msg
    esp_err_t err;
    while((err=twai_transmit(&message, pdMS_TO_TICKS(30))) ==(ESP_FAIL)); //Continue trying to send indefinately in the event that we are waiting for another message to transfer (from another thread). 
    //Otherwise give a timeout of 30ms for queue to open up. If the error is something else, we are not in a state to transmit (which we should handle when checking Bus State).
    //The only other code is for invalid arguments, in which case we will never be able to transmit anyway (although this should fr never happen)
    
    return err;
}

//only for ESP, no arduino equivelant. to be scheduled as a task running every ~1s. Handles bus recovery. Sends status updates for changes in bus state asw.
//no parameters needed
void check_bus_status(void * pvParameters){     //pass pointer to nodeID
    int32_t nodeId=* ((int32_t*) pvParameters);
    for(;;){
        twai_status_info_t status_info;
        if(twai_get_status_info(&status_info)){
            vTaskDelay(1000/portTICK_PERIOD_MS); //if unable to get status info, just try again next time
            continue;
        }

            if(status_info.state==TWAI_STATE_BUS_OFF){
                mutexPrint("initiating recovery\n");
                int recover;
                if ((recover=twai_initiate_recovery())!=ESP_OK){
                    mutexPrint("invalid recovery attempting to reboot. This should never happen\n");    //this should only be called here when Bus is off state, and we never unistall the driver, so error should not be possible
                    esp_restart();
                }
                xSemaphoreGive(*printfMutex); 

            } else if (status_info.state==TWAI_STATE_STOPPED){  //Presumably we have finished recovery
                mutexPrint("Recovery attempt Complete\n\n\n");
                int err=twai_start();
                if(err!=ESP_OK){    //restarts program in the event that we can't start the Bus. This should never happen 
                    if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                        printf("Failed to start or restart driver with code: %d. Rebooting\n",err);
                        xSemaphoreGive(*printfMutex); // Release the mutex.
                    }else { printf("cant print, in deadlock!\n"); }
                    esp_restart();
                } else{
                    mutexPrint("Driver Started\n\n");
                    //send update indicating Bus restarted
                    int16_t err;
                    if((err= sendStatusUpdate(canRecoveryFlag, nodeId))){
                        char buffer[50];
                        sprintf(buffer, "error sending CAN recovery Msg: %d\n", err);  // Convert the int8_t to a string
                        mutexPrint(buffer);  
                    }
                }
            }

        if(status_info.state==TWAI_STATE_BUS_OFF){  //check constantly if recovery is complete and we are ready to restart CAN in the event of Bus off
            mutexPrint("NOTICE: Bus Off\n");
            vTaskDelay(10/portTICK_PERIOD_MS);
        }else{
            // mutexPrint("bus Running\n");
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    }
}