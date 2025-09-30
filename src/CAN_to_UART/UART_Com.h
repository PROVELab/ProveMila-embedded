#ifndef UART_COM_H
#define UART_COM_H
#include <stdint.h>
#include "../pecan/pecan.h"	//For Can


uint16_t in_cksum(const uint8_t *addr, int len);

//UART_receive
bool recvFrame(uint8_t*& out);

//UART_send
int16_t CAN_TO_UART(CANPacket *packet);
void sendUARTFlag(int nodeID, int8_t flag);

#endif // UART_COM_H
