#include "Arduino.h"
#include "../pecan.hpp"

int waitPacket(CANPACKET * recv_pack, int listen_id, void (*handler)(CANPACKET *)){
    
}
CANPACKET waitPackets(CANLISTEN_PARAM * params); // Pass by Ref

void sendPacket(CANPACKET p);