#include <stdint.h>
#include "mbed_toolchain.h"
MBED_PACKED(struct) UART_Packet{
//struct UART_Packet{
    //int8_t battery_level_percent;
    int32_t speedRPM;
};