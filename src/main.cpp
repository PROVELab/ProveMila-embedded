#include <CAN.h> // Include ESP32 CAN library
#include "mcu_node.hpp" // Include the parameter definitions from mcu_node.hpp

// Define the ESP32 CAN RX and TX pins (from mcu_node.hpp)
#define CAN_RX_PIN GPIO_NUM_4
#define CAN_TX_PIN GPIO_NUM_5

void setup() {
    // Start serial communication
    Serial.begin(115200);

    // Initialize CAN at 500Kbps baud rate
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }

    Serial.println("CAN Started");
}

// Function to send a CAN message with CANopen data
void sendCANOpenPacket(int functionCode, int nodeID, uint8_t *data, uint8_t dataLength) {
    int can_id = functionCode | nodeID;
    CAN.beginPacket(can_id);
    CAN.write(data, dataLength);
    CAN.endPacket();
}

// Function to send an SDO message (Service Data Object)
void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint8_t *data, uint8_t dataLength) {
    uint8_t buffer[8] = {SDO_DOWNLOAD_1B, (uint8_t)index, (uint8_t)(index >> 8), subindex};
    memcpy(&buffer[4], data, dataLength);
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer, 8);
}

// Overloaded sendSDO functions for different data sizes (8, 16, 32 bits)
void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint8_t data) {
    uint8_t buffer[8] = {SDO_DOWNLOAD_1B, (uint8_t)index, (uint8_t)(index >> 8), subindex, data, 0, 0, 0};
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer, 8);
}

void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint16_t data) {
    uint8_t buffer[8] = {SDO_DOWNLOAD_2B, (uint8_t)index, (uint8_t)(index >> 8), subindex, (uint8_t)data, (uint8_t)(data >> 8), 0, 0};
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer, 8);
}

void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint32_t data) {
    uint8_t buffer[8] = {SDO_DOWNLOAD_4B, (uint8_t)index, (uint8_t)(index >> 8), subindex, (uint8_t)data, (uint8_t)(data >> 8), (uint8_t)(data >> 16), (uint8_t)(data >> 24)};
    sendCANOpenPacket(CLIENT_TO_SERVER_SDO, nodeID, buffer, 8);
}

// Function to configure TPDO1 (Transmit Process Data Object 1)
void configTPDO1() {
    // Disable TPDO1 communication
    sendSDO(MOTOR_CONT_ID, TPDO1_PARAMETER_INDEX, TPDO1_PARAMETER_COBID_SUBINDEX, (uint32_t)DISABLE_PDO);

    // Disable TPDO1 mapping (set number of mapped objects to 0)
    sendSDO(MOTOR_CONT_ID, TPDO1_MAPPING_INDEX, TPDO1_MAPPING_COUNT_SUBINDEX, (uint8_t)0);

    // Map TPDO1 entries (velocity actual value and motor DC current)
    uint32_t velocity_mapping = (VELOCITY_ACTUAL_VALUE << 16) | (0x00 << 8) | 32; // 32-bit data, correctly packed
    sendSDO(MOTOR_CONT_ID, TPDO1_MAPPING_INDEX, TPDO1_MAPPING_ENTRY_1_SUBINDEX, velocity_mapping);

    uint32_t current_mapping = (MOTOR_DC_CURRENT << 16) | (0x00 << 8) | 32; // 32-bit data, correctly packed
    sendSDO(MOTOR_CONT_ID, TPDO1_MAPPING_INDEX, TPDO1_MAPPING_ENTRY_2_SUBINDEX, current_mapping);

    // Set transmission type to asynchronous
    sendSDO(MOTOR_CONT_ID, TPDO1_PARAMETER_INDEX, TPDO1_PARAMETER_TRANSMISSION_TYPE_SUBINDEX, (uint8_t)ASYNC);

    // Enable TPDO1 communication
    sendSDO(MOTOR_CONT_ID, TPDO1_PARAMETER_INDEX, TPDO1_PARAMETER_COBID_SUBINDEX, (uint32_t)(T_PDO1 | MOTOR_CONT_ID));

    // Enable TPDO1 mapping (set number of mapped objects to 2)
    sendSDO(MOTOR_CONT_ID, TPDO1_MAPPING_INDEX, TPDO1_MAPPING_COUNT_SUBINDEX, (uint8_t)2);
}

// Function to send an NMT (Network Management) command
void sendNMT(uint16_t node_id, uint8_t nmt_command) {
    uint8_t buffer[2] = {nmt_command, (uint8_t)node_id};
    CAN.beginPacket(0x00); // NMT messages have CAN ID 0x00
    CAN.write(buffer, 2);
    CAN.endPacket();
}

// Main loop
void loop() {
    // Example: Configure TPDO1 and send NMT command
    configTPDO1();
    
    // Send NMT to enter operational state
    sendNMT(MOTOR_CONT_ID, ENTER_OPERATIONAL); // 0x01 is the "Enter Operational" command in CANopen
    delay(4000);

    // Insert other operations as needed
}
