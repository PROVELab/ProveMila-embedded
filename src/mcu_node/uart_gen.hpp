#include "mbed.h"
#include "packet_config.hpp"

void print_byte(uint8_t byte);

int32_t pack_byte(uint8_t* remaining_bits, uint32_t data, uint8_t * byte, uint8_t byte_size);

int32_t emplace(Sensor_Data &sd, uint8_t * arr);

int32_t generate_header(Sensor_Data &sd, uint8_t * arr);

int32_t generate_CRC(uint8_t* packet_arr, int32_t size_of_packet, uint8_t* crc_pos);

