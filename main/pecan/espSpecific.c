#include <stdint.h>

#include "mutex_declarations.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include <string.h> // memcpy
#include "pecan.h"

//Simplified Can functionality Functions: feel free to never use these, and just use them as sample code for ESP-CAN library:
int16_t defaultPacketRecv(struct CANPacket* p){
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

int16_t waitPackets(struct CANPacket *recv_pack, struct PCANListenParamsCollection *plpc) {
    //Serial.println(plpc->arr[0].listen_id);
    if (recv_pack == NULL) { //if no empyt packet provided, exit early
        return 1;
    }

    struct CANListenParam clp;
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

int16_t sendPacket(struct CANPacket *p) {
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        return PACKET_TOO_BIG;
    }
    //(p->id)>0b11111111111
 twai_message_t message = {  //This is the struct used to create and send a CAN message. Generally speaking, only the last 3 fields should ever change.
        .extd= (p->id)>0b11111111111,              // Standard vs extended format. makes message extended if
        .rtr = 0,               // Data vs RTR frame.  We should avoid sending RTR frames since the other CAN libraries don't explicitly support it (just send a data message with no data instead)
        .ss = 0,                // Whether the message is single shot (i.e., does not repeat on error)
        .self = 0,              // Whether the message is a self reception request (loopback)
        .dlc_non_comp = 0,      // DLC is less than 8  I beleive, for our purposes, this should always be 0, we want to be compliant with 8 byte data frames, and not confuse arduino guys
        .identifier=p->id,  //id of vitals Heart Beat Ping
        .data_length_code = p->dataSize,
    };
    memcpy(message.data, p->data, p->dataSize);
    return twai_transmit(&message, pdMS_TO_TICKS(10000));
}