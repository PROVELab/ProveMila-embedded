#include <mbed.h>
#include <rtos.h>
#include "../common/pecan.hpp"

#if !DEVICE_CAN
#error[NOT_SUPPORTED] CAN not supported
#endif

Thread thread;

#define CAN_RD P0_30
#define CAN_TD P0_29

#define MOTOR_CONT_ID 1
#define PLACEHOLDER 0

// CAN-ID Function Codes
#define T_PDO1 0x180
#define R_PDO1 0x200
#define T_PDO2 0x280
#define R_PDO2 0x300
#define T_PDO3 0x380
#define R_PDO3 0x400
#define T_PDO4 0x480
#define R_PDO4 0x500
#define SERVER_TO_CLIENT_SDO 0x580
#define CLIENT_TO_SERVER_SDO 0x600

// Command Byte
#define SDO_UPLOAD 0x40
#define SDO_DOWNLOAD_4B 0x23
#define SDO_DOWNLOAD_3B 0x27
#define SDO_DOWNLOAD_2B 0x2B
#define SDO_DOWNLOAD_1B 0x2F

// NMT Commands
#define ENTER_OPERATIONAL 0x01
#define ENTER_STOP 0x02
#define ENTER_PREOPERATIONAL 0x03
#define RESET_NODE 0x81
#define RESET_COMMUNICATION 0x82

#define OVERVOLTAGE_LIMIT 0x2054
#define UNDERVOLTAGE_LIMIT 0x2055
#define UNDERVOLTAGE_LIMIT_SUBINDEX 0x01
#define UNDERVOLTAGE_MIN_VOLTAGE 0x2055
#define UNDERVOLTAGE_MIN_VOLTAGE_SUBINDEX 0x03
#define MAXIMUM_CONTROLLER_CURRENT 0x2050
#define SECONDARY_CURRENT_PROTECTION 0x2051
#define MOTOR_MAXIMUM_TEMPERATURE 0x2057
#define MOTOR_MAXIMUM_TEMPERATURE_SUBINDEX 0x02
#define MAXIMUM_VELOCITY 0x2052
#define MAXIMUM_VELOCITY_SUBINDEX 0x01
#define MOTOR_POLE_PAIRS 0x2033
#define FEEDBACK_TYPE 0x2040
#define FEEDBACK_TYPE_SUBINDEX 0x01
#define MOTOR_PHASE_OFFSET 0x2040
#define MOTOR_PHASE_OFFSET_SUBINDEX 0x02
#define HALL_CONFIGURATION 0x2040
#define HALL_CONFIGURATION_SUBINDEX 0x05
#define FEEDBACK_RESOLUTION 0x2040
#define FEEDBACK_RESOLUTION_SUBINDEX 0x06
#define ELECTRICAL_ANGLE_FILTER 0x2040
#define ELECTRICAL_ANGLE_FILTER_SUBINDEX 0x07
#define MOTOR_PHASE_OFFSET_COMPENSATION 0x2040
#define MOTOR_PHASE_OFFSET_COMPENSATION_SUBINDEX 0x08
#define VELOCITY_ENCODER_FACTOR_NUMERATOR 0x6094
#define VELOCITY_ENCODER_FACTOR_NUMERATOR_SUBINDEX 0x01
#define VELOCITY_ENCODER_FACTOR_DIVISOR 0x6094
#define VELOCITY_ENCODER_FACTOR_DIVISOR_SUBINDEX 0x02
#define TARGET_VELOCITY 0x60FF
#define VELOCITY_ACTUAL_VALUE 0x606C
#define VELOCITY_CONTROL_REGULATOR_P_GAIN 0x60F9
#define VELOCITY_CONTROL_REGULATOR_P_GAIN_SUBINDEX 0x01
#define VELOCITY_CONTROL_REGULATOR_I_GAIN 0x60F9
#define VELOCITY_CONTROL_REGULATOR_I_GAIN_SUBINDEX 0x02
#define MAXIMUM_CONTROLLER_CURRENT 0x2050
#define MOTOR_RATED_CURRENT 0x6075
#define CURRENT_DEMAND 0x201A
#define MOTOR_TEMPERATURE 0x2025
#define MOTOR_DC_CURRENT 0x2023

// The constructor takes in RX, and TX pin respectively.
CAN can(p30, p29);

void sendCANOpenPacket(int functionCode, int nodeID, uint8_t *data)
{
    int can_id = functionCode | nodeID;
    can.write(CANMessage(can_id, data));
}

void readSDO(int nodeID, uint16_t index, uint8_t subindex)
{
    uint8_t buffer[8] = {SDO_UPLOAD, (uint8_t) index, (uint8_t) (index >> 8), subindex, 0, 0, 0, 0};
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer);
}

void writeSDO(int nodeID, uint16_t index, uint8_t subindex, uint8_t data)
{
    uint8_t buffer[8] = {SDO_DOWNLOAD_1B, (uint8_t) index, (uint8_t) (index >> 8), subindex, data, 0, 0, 0};
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer);
}

void writeSDO(int nodeID, uint16_t index, uint8_t subindex, uint16_t data)
{
    uint8_t buffer[8] = {SDO_DOWNLOAD_2B, (uint8_t) index, (uint8_t) (index >> 8), subindex, (uint8_t) data, (uint8_t) (data >> 8), 0, 0};
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer);
}

void writeSDO(int nodeID, uint16_t index, uint8_t subindex, uint32_t data)
{
    uint8_t buffer[8] = {SDO_DOWNLOAD_4B, (uint8_t) index, (uint8_t) (index >> 8), subindex, (uint8_t) data, (uint8_t) (data >> 8), (uint8_t) (data >> 16), (uint8_t) (data >> 24)};
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer);
}

void writeSDO(int nodeID, uint16_t index, uint8_t subindex, uint8_t *data)
{
    uint8_t command = (1 << 5) | ((4 - sizeof data) << 2) | (0b11);
    uint8_t buffer[8] = {command, (uint8_t) index, (uint8_t) ((index & 0xff00) >> 8), subindex, 0, 0, 0, 0};
    memcpy(buffer + 4, data, 4);
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer);
}

// Configure PDO channels for the motor, only use when controller is in
// operational state.
void constructTPDOEntry(uint16_t index, uint8_t sub, uint8_t datasize, uint8_t* buffer) {
    // First 2 bytes are the index of the object dictionary entry
    buffer[0] = index & 0xFF;        // Low byte of index
    buffer[1] = index >> 8;          // High byte of index
    // Next byte is the subindex
    buffer[2] = sub;
    // Last byte is the datasize
    buffer[3] = datasize;
}

void setupPDO1() {
    // Disable PDO communication for TX PDO by setting COB-ID to 0x80000000
    uint32_t disableTxPdo = 0x80000000;
    writeSDO(MOTOR_CONT_ID, 0x1800, 0x01, disableTxPdo);

    // Clear the number of PDO entries to zero before mapping
    uint8_t zero = 0;
    writeSDO(MOTOR_CONT_ID, 0x1A00, 0x00, zero);

    // Construct PDO entries for velocity and motor dc current actual
    uint8_t pdoEntryBuffer[4];

    // Motor speed
    constructTPDOEntry(VELOCITY_ACTUAL_VALUE, 0, 0x20, pdoEntryBuffer);
    writeSDO(MOTOR_CONT_ID, 0x1A00, 0x01, pdoEntryBuffer);
    pdoEntryBuffer[0] = 0;
    pdoEntryBuffer[1] = 0;
    pdoEntryBuffer[2] = 0;
    pdoEntryBuffer[3] = 0;

    // Motor dc current actual
    constructTPDOEntry(MOTOR_DC_CURRENT, 0, 0x20, pdoEntryBuffer);
    writeSDO(MOTOR_CONT_ID, 0x1A00, 0x02, pdoEntryBuffer);
    pdoEntryBuffer[0] = 0;
    pdoEntryBuffer[1] = 0;
    pdoEntryBuffer[2] = 0;
    pdoEntryBuffer[3] = 0;

    // Set the actual number of PDO entries after mapping
    uint8_t numEntries = 2;
    writeSDO(MOTOR_CONT_ID, 0x1A00, 0x00, numEntries);

    // Set transmission type for TX PDO
    uint8_t transmissionType = 0xff; // Async transmission
    writeSDO(MOTOR_CONT_ID, 0x1800, 0x02, transmissionType);

    uint16_t pdoNumber = 1; // This is TPDO1
    uint16_t cobId = (pdoNumber << 11) | (1 << 7) | (MOTOR_CONT_ID & 0x7F);
    writeSDO(MOTOR_CONT_ID, 0x1800, 0x01, cobId);

    // Save parameters
    // Reset device
    // Set device to operational mode
}

void setupPDO2() {
    // Disable PDO communication for TX PDO by setting COB-ID to 0x80000000
    uint32_t disableTxPdo = 0x80000000;
    writeSDO(MOTOR_CONT_ID, 0x1801, 0x01, disableTxPdo);

    // Clear the number of PDO entries to zero before mapping
    writeSDO(MOTOR_CONT_ID, 0x1A01, 0x00, (uint8_t)0x00);

    // Construct PDO entries for motor temperature, and current requested
    uint8_t pdoEntryBuffer[4];

    // Motor temperature
    constructTPDOEntry(MOTOR_TEMPERATURE, 0, 0x08, pdoEntryBuffer);
    writeSDO(MOTOR_CONT_ID, 0x1A01, 0x01, pdoEntryBuffer);
    pdoEntryBuffer[0] = 0;
    pdoEntryBuffer[1] = 0;
    pdoEntryBuffer[2] = 0;
    pdoEntryBuffer[3] = 0;

    // Motor current requested
    constructTPDOEntry(CURRENT_DEMAND, 0, 0x10, pdoEntryBuffer);
    writeSDO(MOTOR_CONT_ID, 0x1A01, 0x02, pdoEntryBuffer);
    pdoEntryBuffer[0] = 0;
    pdoEntryBuffer[1] = 0;
    pdoEntryBuffer[2] = 0;
    pdoEntryBuffer[3] = 0;

    // Set the actual number of PDO entries after mapping
    uint8_t numEntries = 2;
    writeSDO(MOTOR_CONT_ID, 0x1A01, 0x00, numEntries);

    // Set transmission type for TX PDO
    uint8_t transmissionType = 0xff; // Async transmission
    writeSDO(MOTOR_CONT_ID, 0x1801, 0x02, transmissionType);

    uint16_t pdoNumber = 2; // This is TPDO2
    uint16_t cobId = (pdoNumber << 11) | (1 << 7) | (MOTOR_CONT_ID & 0x7F);
    writeSDO(MOTOR_CONT_ID, 0x1801, 0x01, cobId);
}

// Apply state change to all nodes by passing node_id = 0.
void configNMT(uint16_t node_id, uint8_t nmt_command) {
    uint8_t buffer[2] = {nmt_command, (uint8_t) node_id};
    can.write(CANMessage(0x00, buffer));
}

// Function, receiveSDO, consumes a CAN packet and prints the metadata.
void receiveSDO(CANPacket * packet) {
    // Decode CAN-ID
    uint8_t func_code = (packet->id) >> 7;
    uint8_t node_id = ((packet->id) && 0x007f);

    // Decode data
    uint8_t byte_0 = packet->data[0]; // Command byte
    uint8_t byte_1 = packet->data[1]; // Object dictionary index
    uint8_t byte_2 = packet->data[2]; // Object dictionary index
    uint8_t byte_3 = packet->data[3]; // Object dictionary sub-index
    uint8_t byte_4 = packet->data[4]; // Data
    uint8_t byte_5 = packet->data[5]; // ...
    uint8_t byte_6 = packet->data[6]; // ...
    uint8_t byte_7 = packet->data[7]; // ...

    // Print metadata
    printf("===== Received SDO =====\n");
    printf("Function Code: %u\n", func_code);
    printf("Node ID: %u\n\n", node_id);
    printf("Byte 0\n");
    printf("-- CCS: %u\n", (byte_0 && 0xE0) >> 5);
    printf("-- n: %u\n", (byte_0 && 0x0C) >> 2);
    printf("-- e: %u\n", (byte_0 && 0x02) >> 1);
    printf("-- s: %u\n\n", byte_0 && 0x01);
    printf("Bytes [1,2]:\n");
    printf("-- OD Index: %u\n\n", (((uint16_t)byte_1 << 8) | byte_2));
    printf("Byte 3:\n");
    printf("-- OD Sub-Index: %u\n\n", (byte_3));
    printf("Bytes [4,7]\n");
    printf("-- Data: %lu\n", ((uint32_t)byte_4 << 24) | ((uint32_t)byte_5 << 16) | ((uint32_t)byte_6 << 8) | byte_7);
    printf("========================\n\n");
}

// Starts bootload sequence.
void startBootload()
{
    // Set mode of operation.
    writeSDO(MOTOR_CONT_ID, 0x6060, 0, (uint8_t)9); // 9: Velocity mode

    // Set motor type.
    writeSDO(MOTOR_CONT_ID, 0x2034, 0, (uint8_t)0);

    // Set controller/motor protections.
    writeSDO(MOTOR_CONT_ID, OVERVOLTAGE_LIMIT, 0, (uint16_t)PLACEHOLDER);                                         // Overvoltage limit
    writeSDO(MOTOR_CONT_ID, UNDERVOLTAGE_LIMIT, UNDERVOLTAGE_LIMIT_SUBINDEX, (uint8_t)PLACEHOLDER);               // Undervoltage limit
    writeSDO(MOTOR_CONT_ID, UNDERVOLTAGE_MIN_VOLTAGE, UNDERVOLTAGE_MIN_VOLTAGE_SUBINDEX, (uint16_t)PLACEHOLDER);  // Undervoltage min voltage
    writeSDO(MOTOR_CONT_ID, MAXIMUM_CONTROLLER_CURRENT, 0, (uint32_t)PLACEHOLDER);                                // Maximum controller current
    writeSDO(MOTOR_CONT_ID, SECONDARY_CURRENT_PROTECTION, 0, (uint32_t)PLACEHOLDER);                              // Secondary current protection
    writeSDO(MOTOR_CONT_ID, MOTOR_MAXIMUM_TEMPERATURE, MOTOR_MAXIMUM_TEMPERATURE_SUBINDEX, (uint8_t)PLACEHOLDER); // Motor maximum temperature
    writeSDO(MOTOR_CONT_ID, MAXIMUM_VELOCITY, MAXIMUM_VELOCITY_SUBINDEX, (uint32_t)PLACEHOLDER);                  // Maximum velocity

    // Perform motor position measurement.
    writeSDO(MOTOR_CONT_ID, MOTOR_POLE_PAIRS, 0, (uint8_t)PLACEHOLDER);                                                            // Motor pole pairs
    writeSDO(MOTOR_CONT_ID, FEEDBACK_TYPE, FEEDBACK_TYPE_SUBINDEX, (uint8_t)PLACEHOLDER);                                          // Feedback type
    writeSDO(MOTOR_CONT_ID, MOTOR_PHASE_OFFSET, MOTOR_PHASE_OFFSET_SUBINDEX, (uint16_t)PLACEHOLDER);                               // Motor phase offset
    writeSDO(MOTOR_CONT_ID, HALL_CONFIGURATION, HALL_CONFIGURATION_SUBINDEX, (uint8_t)PLACEHOLDER);                                // Hall configuration
    writeSDO(MOTOR_CONT_ID, FEEDBACK_RESOLUTION, FEEDBACK_RESOLUTION_SUBINDEX, (uint16_t)PLACEHOLDER);                             // Feedback resolution
    writeSDO(MOTOR_CONT_ID, ELECTRICAL_ANGLE_FILTER, ELECTRICAL_ANGLE_FILTER_SUBINDEX, (uint16_t)PLACEHOLDER);                     // Electrical angle filter
    writeSDO(MOTOR_CONT_ID, MOTOR_PHASE_OFFSET_COMPENSATION, MOTOR_PHASE_OFFSET_COMPENSATION_SUBINDEX, (uint16_t)PLACEHOLDER);     // Motor phase offset compensation
    writeSDO(MOTOR_CONT_ID, VELOCITY_ENCODER_FACTOR_NUMERATOR, VELOCITY_ENCODER_FACTOR_NUMERATOR_SUBINDEX, (uint32_t)PLACEHOLDER); // Velocity encoder factor Numerator
    writeSDO(MOTOR_CONT_ID, VELOCITY_ENCODER_FACTOR_DIVISOR, VELOCITY_ENCODER_FACTOR_DIVISOR_SUBINDEX, (uint32_t)PLACEHOLDER);     // Velocity encoder factor Divisor

    // Configure velocity mode parameters.
    writeSDO(MOTOR_CONT_ID, TARGET_VELOCITY, 0, (uint32_t)PLACEHOLDER);                                                            // Target velocity
    writeSDO(MOTOR_CONT_ID, VELOCITY_ACTUAL_VALUE, 0, (uint32_t)PLACEHOLDER);                                                      // Velocity actual value
    writeSDO(MOTOR_CONT_ID, VELOCITY_CONTROL_REGULATOR_P_GAIN, VELOCITY_CONTROL_REGULATOR_P_GAIN_SUBINDEX, (uint16_t)PLACEHOLDER); // Velocity control regulator P gain
    writeSDO(MOTOR_CONT_ID, VELOCITY_CONTROL_REGULATOR_I_GAIN, VELOCITY_CONTROL_REGULATOR_I_GAIN_SUBINDEX, (uint16_t)PLACEHOLDER); // Velocity control regulator I gain
    writeSDO(MOTOR_CONT_ID, MOTOR_RATED_CURRENT, 0, (uint32_t)PLACEHOLDER);                                                        // Motor rated current
}

int16_t acceptSDOResponse(CANPacket *packet) {
    char val[4] = {0};
    memcpy(val, packet->data + 4, 4);
    printf("Received: %lu\n",  (*((uint32_t*) val)));
    return 0;
}

void testWriteSDO(uint16_t index, uint8_t subindex, uint32_t data) {
    PCANListenParamsCollection pclp;

    CANListenParam stocSDO(SERVER_TO_CLIENT_SDO | MOTOR_CONT_ID, acceptSDOResponse, MATCH_EXACT);
    addParam(&pclp, stocSDO);

    // Read current value.
    printf("Reading Current Value\n");
    readSDO(MOTOR_CONT_ID, index, subindex);

    while(waitPackets(NULL, &pclp) == NOT_RECEIVED)
        ;

    // Update value.
    printf("Writing new value\n");
    writeSDO(MOTOR_CONT_ID, index, subindex, data);

    while(waitPackets(NULL, &pclp) == NOT_RECEIVED)
        ;
    
    // Read updated value.
    printf("Reading updated value\n");
    readSDO(MOTOR_CONT_ID, index, subindex);

    while(waitPackets(NULL, &pclp) == NOT_RECEIVED)
        ;

    printf("\n");
}

void testReadSDO(uint16_t index, uint8_t subindex) {
    PCANListenParamsCollection pclp;

    CANListenParam stocSDO(SERVER_TO_CLIENT_SDO | MOTOR_CONT_ID, acceptSDOResponse, MATCH_EXACT);
    addParam(&pclp, stocSDO);

    printf("Reading OD Index\n");
    readSDO(MOTOR_CONT_ID, index, subindex);

    while(waitPackets(NULL, &pclp) == NOT_RECEIVED)
        ;

    printf("\n");
}

int main()
{
    can.frequency(500E3);
    CANMessage msg;
    uint32_t val = 10000;
    while (1)
    {
        // testWriteSDO(0x2050, 0x00, val);
        // val = val + 1;
        // testReadSDO(0x2050, 0x00);
        printf("Enter Operational State\n");
        configNMT(MOTOR_CONT_ID, ENTER_OPERATIONAL);
        printf("\n");
        ThisThread::sleep_for(1s);
    }
}