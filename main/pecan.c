#include <stdint.h>
#include "mutex_declarations.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include <string.h> // memcpy
#include "pecan.h"

uint32_t combinedID(uint32_t fn_id, uint32_t node_id){
    return (fn_id << 7) + node_id;
}

void setSensorID(struct CANPacket * p, uint8_t sensorId){
    p->data[0] = sensorId;
}

int16_t addParam(struct PCANListenParamsCollection * plpc, struct CANListenParam clp){
    if (plpc->size + 1 > MAX_PCAN_PARAMS){
        return NOSPACE;
    } else {
        plpc->arr[plpc->size] = clp;
        plpc->size++;
        return SUCCESS;
    }
}

int16_t writeData(struct CANPacket * p, int8_t * dataPoint, int16_t size){
    
    int16_t current_size = p->dataSize;
    int16_t i = 0;
    if (i + size > MAX_SIZE_PACKET_DATA){
        return NOSPACE;
    }
    for (; current_size + i < current_size + size; i++){
        // DataSize can be interpreted as both
        // Size, and Index
        // Casting to 16-bit because compiler not happy
        p->data[(int16_t)p->dataSize] = dataPoint[i];
        p->dataSize++;

        // This check should've been working above
        // But just in case, we'll do it in the loop as well
        if (i > MAX_SIZE_PACKET_DATA){
            return NOSPACE;
        }
    }
    return SUCCESS;
}

bool exact(uint32_t id, uint32_t mask) {  // entire id must match mask
    return id == mask;
}
//note: ID sent over CAN is 11 bit long, with first 7 bitsbeing the identifier of sending node, and last 4 bits being the function code
bool matchID(uint32_t id, uint32_t mask){ //check if the 7 bits of node ID must match mask
    return ( (id & 0b1111111) & mask) == id;
}

bool matchFunction(uint32_t id,uint32_t mask){    //mask should be 4 bit function code
    return ((id>>7)&0b1111) == ((mask>>7)&0b1111);//only compares the functionCodes
}


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



int16_t waitPackets(struct CANPacket *recv_pack, struct PCANListenParamsCollection *plpc) {
    //Serial.println(plpc->arr[0].listen_id);
    if (recv_pack == NULL) { //if no empyt packet provided, exit early
        return 1;
    }

    struct CANListenParam clp;
    twai_message_t twaiMSG;
    if (twai_receive(&twaiMSG, pdMS_TO_TICKS(0)) == ESP_OK) {   //check for message without blocking (0 ms blocking)
        if (twaiMSG.extd) {//matcher functions only set to support 11 bit IDs
            return 1;
        } 

        //construct CANPacket
        recv_pack->id =twaiMSG.identifier;
        recv_pack->dataSize=twaiMSG.data_length_code;
        memcpy(recv_pack->data, twaiMSG.data,recv_pack->dataSize);
        //

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
 twai_message_t message = {  //This is the struct used to create and send a CAN message. Generally speaking, only the last 3 fields should ever change.
        .extd= 0,              // Standard vs extended format
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

