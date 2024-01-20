#include "mbed.h"
#include "USBSerial.h"
#include "uart_gen.hpp"

//create mouse object
USBSerial serial;

int main(){
    Sensor_Data spruart_packet;
    spruart_packet.TP1_data = 0;
    spruart_packet.TP2_data = 1;
    spruart_packet.TP3_data = 0;
    spruart_packet.TP4_data = 3;


    while (1) {
        spruart_packet.TP4_data++; spruart_packet.TP1_data++; spruart_packet.TP2_data++; spruart_packet.TP3_data++;

        uint8_t buf[HEADER_SIZE_BYTES + MAX_DATA_SIZE_BYTES + CRC_SIZE_BYTES] = {0};
        int out = generate_header(spruart_packet, buf);
        int data_size = emplace(spruart_packet, buf + HEADER_SIZE_BYTES);
        generate_CRC(buf, HEADER_SIZE_BYTES + data_size, &buf[HEADER_SIZE_BYTES + data_size ]);

        serial.write(buf, HEADER_SIZE_BYTES + data_size + CRC_SIZE_BYTES);
    }
}
