#include "Arduino.h"
#include "../common/pecan.hpp"
#include "CAN.h"

int defaultPacketRecv(CANPacket * packet){
    Serial.print("Default Received packet, id ");
    Serial.print(packet->id);
    Serial.print(", with data ");
    Serial.println(packet->data);
}

int waitPackets(CANPacket * recv_pack, PCANListenParamsCollection * plpc){
    if (recv_pack == NULL){
        CANPacket p;
        recv_pack = &p;
        // We can only use this for handler
        // Because stack
    }
    
    int packetsize;
    CANListenParam clp;
    int id;
    if ((packetsize = CAN.parsePacket())){
        id = CAN.packetId();
        recv_pack->id = id;
        recv_pack->dataSize = packetsize;
        for (int j = 0; j < recv_pack->dataSize; j++){
            recv_pack->data[j] = CAN.read();
        }
        for (int i = 0; i < plpc->size; i++){
            clp = plpc->arr[i];
            if (matcher[clp.mt](id, clp.listen_id)){
                return clp.handler(recv_pack);
            }
        }
        return plpc->defaultHandler(recv_pack);
    }
    return NOT_RECEIVED;
}


int sendPacket(CANPacket * p){
    if (p->dataSize > MAX_SIZE_PACKET_DATA){
        return PACKET_TOO_BIG;
    }
    CAN.beginPacket(p->id);
    for (int i = 0; i < p->dataSize; i++){
        CAN.write( p->data[i]);
    }
    if(CAN.endPacket()){
        Serial.println("Success!!!");
    } else {
        return GEN_FAILURE;
    }
    return SUCCESS;

}
