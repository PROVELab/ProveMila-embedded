#ifndef EMDRIVE_H300_H
#define EMDRIVE_H300_H

#include "driver/twai.h"
#include "vsr.h"

#define SLAVE_ID 0x300 // Change this as necessary

enum {
    // messages that we send out
    REFERENCE_MSG_ID = SLAVE_ID * 10,
    RESET_CAN_MSG_ID = SLAVE_ID * 100 + 3,

    // messages that we receive
    HIGH_FREQ_DATA_ID = SLAVE_ID * 100,
    MED_FREQ_DATA_ID = SLAVE_ID * 100 + 1,
    LOW_FREQ_DATA_ID = SLAVE_ID * 100 + 2,

    // calib stuff
    CALIBRATION_INIT_MSG_ID = SLAVE_ID * 10 + 1,
    CALIBRATION_RESULT_MSG_ID = SLAVE_ID * 100 + 4,
} H300_MSG_IDS;

// is it an h300 id?
#define IS_H300_ID(id)                                                         \
    ((id <= CALIBRATION_RESULT_MSG_ID && id >= HIGH_FREQ_DATA_ID) ||           \
     (id <= CALIBRATION_INIT_MSG_ID && id >= REFERENCE_MSG_ID))

#define PACKET_PARSE_INPUTS                                                    \
    twai_message_t *msg, volatile vehicle_status_reg_s *vsr

// This header file
// defines a bunch of things for the H300 motor controller
// including flags, deserialization functions, etc
void parse_packet(PACKET_PARSE_INPUTS);
void parse_high_freq_data(PACKET_PARSE_INPUTS);
void parse_med_freq_data(PACKET_PARSE_INPUTS);
void parse_low_freq_data(PACKET_PARSE_INPUTS);

// helper macros
#define CELSIUS_TO_FAHRENHEIT(c) ((c) * 9 / 5 + 32)
#define BYTE_TO_TEMP(b)                                                        \
    ((int16_t)(b) - 60) // convert byte to temperature in Fahrenheit

#endif // end header guard