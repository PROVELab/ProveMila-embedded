#include "Arduino.h"

#include "pecan.h"
#include "CAN.h"

#include <Arduino.h>

int16_t defaultPacketRecv(struct CANPacket *packet) {  //this is only to be used by vitals for current
    Serial.print("Default Func: id ");
    Serial.print(packet->id);
    Serial.print(",  data ");
    Serial.println((char*) packet->data);
    return 1;
}
bool (*matcher[3])(uint32_t, uint32_t) = {exact, matchID, matchFunction};   //could alwys be moved back to pecan.h as an extern variable if its needed elsewhere? I am not sure why this was declared there in the first place

int16_t waitPackets(struct CANPacket *recv_pack, struct PCANListenParamsCollection *plpc) {
    //Serial.println(plpc->arr[0].listen_id);
    if (recv_pack == NULL) {
        struct CANPacket p;
        recv_pack = &p;
        // We can only use this for handler
        // Because stack
    }

    int8_t packetsize;
    struct CANListenParam clp;
    //Serial.println(CAN.parsePacket());
    if ((packetsize = CAN.parsePacket())) {
        Serial.println("recieving Packet: ");
        delay(100);
        recv_pack->id = CAN.packetId();
        recv_pack->dataSize = packetsize;
        recv_pack->rtr= CAN.packetRtr();
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

int16_t sendPacket(struct CANPacket *p) {  //note: if your id is longer than 11 bits it made into
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
    if (CAN.endPacket()) {
        //Serial.print(CAN.parsePacket());
        //Serial.println(CAN.packetId());
    } else {
        return GEN_FAILURE;
    }
    return SUCCESS;
}