#include "Arduino.h"

#include "pecan.h"
#include "CAN.h"

#include <Arduino.h>

int16_t defaultPacketRecv(CANPacket *packet) {  //this is only to be used by vitals for current
    Serial.print("Default Func: id ");
    Serial.print(packet->id);
    Serial.print(",  data ");
    Serial.println((char*) packet->data);
    return 1;
}
bool (*matcher[3])(uint32_t, uint32_t) = {exact, matchID, matchFunction};   //could alwys be moved back to pecan.h as an extern variable if its needed elsewhere? I am not sure why this was declared there in the first place

int16_t waitPackets(CANPacket *recv_pack, PCANListenParamsCollection *plpc) {   
    if (recv_pack == NULL) {    //final product code cant be calling this with NULL, since packet loses scope
        CANPacket p;
        recv_pack = &p;
    }

    int8_t packetsize;
    CANListenParam clp;
    if ((packetsize = CAN.parsePacket())) {
        Serial.println("recieving Packet: ");
        recv_pack->id = CAN.packetId();
        recv_pack->dataSize = packetsize;
        recv_pack->rtr= CAN.packetRtr();
        memset(recv_pack->data, 0, 8);  //re-initialize data to all 0.
        // Read and temporarily store all the packet data into PCAN
        // packet (on stack memory)
        for (int8_t j = 0; j < recv_pack->dataSize; j++) {
            recv_pack->data[j] = CAN.read();
        }
        // Then match the packet id with our params; if none
        // match, use default handler
        Serial.println("trying match");
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

int16_t sendPacket(CANPacket *p) {  //note: if your id is longer than 11 bits it made into
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        return PACKET_TOO_BIG;
    }
    if(p->id>0b11111111111){
        CAN.beginExtendedPacket(p->id, p->dataSize, p->rtr);
    }else{
        CAN.beginPacket(p->id, p->dataSize, p->rtr);
    }
    for (int8_t i = 0; i < p->dataSize; i++) {
        CAN.write(p->data[i]);
    }
    if (!CAN.endPacket()) {
        return GEN_FAILURE;
    } 
    return SUCCESS;
}