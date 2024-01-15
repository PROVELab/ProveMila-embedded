#include "packet_config.hpp"
/**
 * Author: SHYNN!!!
 * This code takes in sensor data
 * and other details, and constructs various parts
 * of a UART packet out of it, in conformance with
 * the "standard"
 **/
// For Debugging purposes
void print_byte(uint8_t byte){
    for (int i = 7; i >= 0; i--){
        printf("%d", (byte >> i) & 1);
    }
    printf("|");
}

/**
 * @brief Takes in some data, how many bits of the data is
 *      to be packed, and other such details, and packs
 *      "data" into 1 "byte", keeping track of remainders
 *      to be packed in later bytes.
 * 
 * @param remaining_bits The amount of remaining bits in data 
 *                          that need to be packed
 * @param data The data to be packed
 * @param byte Where to pack the data to (in memory)
 * @param byte_size how many bits are already filled in this byte
 * @return int32_t The amount of bits that remain to be filled in this byte
 */
int32_t pack_byte(uint8_t* remaining_bits, uint32_t data, uint8_t * byte, uint8_t byte_size){
    if (*remaining_bits <= 8-byte_size){
        *byte = (*byte) | (data << ((8-*remaining_bits)-byte_size));
        byte_size += (*remaining_bits);
        *remaining_bits = 0;
        return byte_size;
    } else {
        data = data << (32-*remaining_bits);
        data = data >> (32-(8-byte_size));
        data = (uint8_t) data;
        *byte = (*byte) | (data);
        *remaining_bits = *remaining_bits - (8 - byte_size);
        return 8;
    }
}

/**
 * @brief Takes in sensor data as a struct and fills in
 *      all the valid sensor data into byte array arr
 * 
 * @param sd The SensorData struct that specifies all the sensor
 *              data to be sent in this packet
 * @param arr A byte array to start filling out with the sensor data
 *              (The place to put the serialized data)
 * @param size idk tbh, just delete this later
 * @return int32_t How many bytes we put into arr, so that we
 *                  send only the exact amount of bytes over
 *                  UART rather than sending excess.
 */
int32_t emplace(Sensor_Data &sd, uint8_t * arr, uint8_t size){
    uint8_t * working_ptr = (uint8_t*)&sd;

    // Use this index to iterate and place
    // values in arr
    int32_t byte_index = 0;
    // The amount of bits filled in the byte at arr[byte_index]
    uint8_t bit_ct = 0;

    // Size of this particular sensor data in bits
    // and the data itself
    uint8_t bits_to_pack;
    int32_t data;

    for (int32_t i = 0; i < DATA_STRUCT_SIZE; i+=5){
        bits_to_pack = working_ptr[i];
        // If there is valid data there (it's not ~null), then we do the packing
        if ((data = *((int32_t*) (working_ptr + i + 1))) != SENTINEL){
            // Basically, while we haven't packed all the bits, pack the 
            // remaining bits and iterate over the buffer
            while (bits_to_pack != 0){
                bit_ct = pack_byte(&bits_to_pack, data, &arr[byte_index], bit_ct);
                if (bit_ct == 8){
                    byte_index += 1;
                    bit_ct = 0;
                }
            }
            // By this point, we packed all the bits for this sensor's data
        }
    }
    // If we reach here, we packed all the bits for 
    // all the sensor's data that we have so far; let's finish off by returning
    // the amount of bytes we used. Recall index is always true size-1
    return (byte_index != 0 || bit_ct != 0) ? byte_index+1 : 0;
}

/**
 * @brief Generates a header from the filled-in
 *          sensor data and puts it into arr
 * If we have Sensors TP1, TP2, TP3, TP4 but only data
 * for TP1 and TP4, the header would look like:
 * 0b1001....
 * @param sd The Sensor Data to send in this packet
 * @param arr The place to put the header
 * @return int32_t Amount of Bytes written
 */
int32_t generate_header(Sensor_Data &sd, uint8_t * arr){
    uint8_t * working_ptr = (uint8_t*)&sd;
    uint32_t working_byte_index = 0;
    uint32_t bits_written = 0;
    uint8_t bits_to_write = 1;

    int32_t sensor_num;
    int32_t sensor_struct_pos;
    for (sensor_num = 0; sensor_num < SENSOR_CT; sensor_num++){
        sensor_struct_pos = sensor_num * 5 + 1
        // Is there data at this sensor position?
        bool data = ((int32_t) (&working_ptr[sensor_struct_pos])) != SENTINEL;
        // Place 1 bit (1 if there is data, 0 if there isn't) in the proper place
        bits_written = pack_byte(&bits_to_write, (uint32_t) data, &arr[working_byte_index], bits_written);
        bits_to_write = 0;
        if (bits_written == 8){
            working_byte_index += 1;
            bits_written = 0;
        }
    }
    return (SENSOR_CT % 8 == 0) ? SENSOR_CT / 8 : (SENSOR_CT/8)+1;
}

int32_t generate_CRC(uint8_t* packet_arr, int32_t size_of_packet, uint8_t* crc_pos){
   
    // Index into the polynomial array;
    // Because it's static, it will persist across
    // function calls, meaning that the 2nd time this function is called
    // it will be = 1, then 2, etc according to the ++ below
    static uint8_t count = 0;

    uint16_t crc = polys[count];

    for (int i = 0; i < size_of_packet; i++){
        uint8_t data_byte = packet_arr[i];

        crc ^= (uint16_t)data_byte << 8;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    count %= 5;
    count ++;
    return crc;
}

