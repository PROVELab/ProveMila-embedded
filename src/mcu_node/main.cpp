#include "mbed.h"
#include "USBSerial.h"
#include "uart_gen.hpp"

uint32_t send_packet(Sensor_Data& packet);

//create mouse object
USBSerial serial;

int main(){
    Sensor_Data spruart_packet;
    spruart_packet.TP1_data = 0;
    spruart_packet.TP2_data = 1;
    spruart_packet.TP3_data = 0;
    spruart_packet.TP4_data = 3;


    while (1) {
        send_packet(spruart_packet);
    }
}

uint32_t send_packet(Sensor_Data& packet){
    
    packet.TP4_data++; packet.TP1_data++; packet.TP2_data++; packet.TP3_data++;

    uint8_t buf[HEADER_SIZE_BYTES + MAX_DATA_SIZE_BYTES + CRC_SIZE_BYTES] = {0};
    generate_header(packet, buf);
    int data_size = emplace(packet, buf + HEADER_SIZE_BYTES);
    generate_CRC(buf, HEADER_SIZE_BYTES + data_size, &buf[HEADER_SIZE_BYTES + data_size ]);

    serial.write(buf, HEADER_SIZE_BYTES + data_size + CRC_SIZE_BYTES);
}
