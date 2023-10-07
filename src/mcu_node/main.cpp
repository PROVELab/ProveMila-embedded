#include <mbed.h>
#include <rtos.h>

#if !DEVICE_CAN
#error [NOT_SUPPORTED] CAN not supported
#endif

Thread thread;

#define CAN_RD P0_30
#define CAN_TD P0_29

#define MOTOR_CONT_ID 1
#define PLACEHOLDER 1

// COB-ID Function Codes
#define T_PDO1 3
#define R_PDO1 4
#define T_PDO2 5
#define R_PDO2 6
#define T_PDO3 7
#define R_PDO3 8
#define T_PDO4 9
#define R_PDO4 10
#define T_SDO 11
#define R_SDO 12

DigitalOut led1(LED1);
DigitalOut led2(LED2);
// The constructor takes in RX, and TX pin respectively.
CAN can(p30, p29);
char counter = 0;

// Function, configNMT, sends a CANOpen message to change the state of a device.
void configNMT(uint8_t nmtComm, uint8_t addrNode)
{
    uint8_t data[2] = {nmtComm, addrNode};
    can.write(CANMessage(0x000, data));
}

void sendCANOpenPacket(int functionCode, int nodeID, uint8_t* data)
{
    // Construct packetID (COB-ID).
    int packetID = (functionCode << 7) | nodeID;
    can.write(CANMessage(packetID, data));
    led1 = !led1;
}

// Function, sendSDO, sends an array of data of at most 4 bytes in size.
void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint8_t* data)
{
    // Build command byte
    uint8_t command = (0b001 << 5) | ((4 - sizeof data) << 2) | (0b11);

    // Build CANOpen packet byte
    uint8_t buffer[8] = {command, index, ((index & 0xff00) >> 8), subindex, 0, 0, 0, 0};
    memcpy(buffer + 4, data, 4);

    // Send packet
    sendCANOpenPacket(R_SDO, nodeID, buffer);
}

// Function, configPDO, sends SDOs to configure the PDO channels.
void configPDO()
{
    // Config RX PDO 1
    sendSDO(MOTOR_CONT_ID, 0x1600, 0x00, (uint8_t) 1);              // Declare single value for channel.
    sendSDO(MOTOR_CONT_ID, 0x1600, 0x01, (uint16_t) 0x60710010);    // Map target torque to value #1.

    // Config RX PDO 2 ...
}

// Function, sendSDO, sends data of one byte in size.
void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint8_t data)
{
    // Build command byte
    uint8_t command = (0b001 << 5) | ((4 - sizeof(data)) << 2) | (0b11);

    // Build CANOpen packet byte
    uint8_t buffer[8] = {command, index, ((index & 0xff00) >> 8), subindex, data, 0, 0, 0};

    // Send packet
    sendCANOpenPacket(R_SDO, nodeID, buffer);
}

// Function, sendSDO, consumes node id, object dictionary index, object dictionary subindex, and the uint16_t SDO data, and sends an SDO packet.
void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint16_t data)
{
    // Build command byte
    uint8_t command = (0b001 << 5) | ((4 - sizeof(data)) << 2) | (0b11);

    // Build CANOpen packet byte
    uint8_t buffer[8] = {command, index, ((index & 0xff00) >> 8), subindex, (0x00ff & data), ((0xff00 & data) >> 8), 0, 0};

    // Send packet
    sendCANOpenPacket(R_SDO, nodeID, buffer);
}

// Function, sendSDO, consumes node id, object dictionary index, object dictionary subindex, and the uint32_t SDO data, and sends an SDO packet.
void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint32_t data)
{
    // Build command byte
    uint8_t command = (0b001 << 5) | ((4 - sizeof(data)) << 2) | (0b11);

    // Build CANOpen packet byte
    uint8_t buffer[8] = {command, index, ((index & 0xff00) >> 8), subindex, ((0xff000000 & data) >> 24), ((0x00ff0000 & data) >> 16), ((0x0000ff00 & data) >> 8), (0x000000ff & data)};

    // Send packet
    sendCANOpenPacket(R_SDO, nodeID, buffer);
}

// Function, startBootload, starts the bootloading initialization.
void startBootload() 
{
    uint8_t buffer[1] = {0};

    // Set mode of operation.
    sendSDO(MOTOR_CONT_ID, 0x6060, 0, (uint8_t) 9) // 9: Velocity mode

    // Set motor type.
    sendSDO(MOTOR_CONT_ID, 0x2034, 0, buffer);

    // Set controller/motor protections.
    sendSDO(MOTOR_CONT_ID, 0x2054, 0, (uint16_t) PLACEHOLDER);      // Overvoltage limit
    sendSDO(MOTOR_CONT_ID, 0x2055, 0x01, (uint8_t) PLACEHOLDER);    // Undervoltage limit
    sendSDO(MOTOR_CONT_ID, 0x2055, 0x03, (uint16_t) PLACEHOLDER);   // Undervoltage min voltage
    sendSDO(MOTOR_CONT_ID, 0x2050, 0, (uint32_t) PLACEHOLDER);      // Maximum controller current
    sendSDO(MOTOR_CONT_ID, 0x2051, 0, (uint32_t) PLACEHOLDER);      // Secondary current protection
    sendSDO(MOTOR_CONT_ID, 0x2057, 0x02, (uint8_t) PLACEHOLDER);    // Motor maximum temperature
    sendSDO(MOTOR_CONT_ID, 0x2052, 0x01, (uint32_t) PLACEHOLDER);   // Maximum velocity

    // Perform motor position measurement.
    sendSDO(MOTOR_CONT_ID, 0x2033, 0, (uint8_t) PLACEHOLDER);       // Motor pole pairs
    sendSDO(MOTOR_CONT_ID, 0x2040, 0x01, (uint8_t) PLACEHOLDER);    // Feedback type
    sendSDO(MOTOR_CONT_ID, 0x2040, 0x02, (uint16_t) PLACEHOLDER);   // Motor phase offset
    sendSDO(MOTOR_CONT_ID, 0x2040, 0x05, (uint8_t) PLACEHOLDER);    // Hall configuration
    sendSDO(MOTOR_CONT_ID, 0x2040, 0x06, (uint16_t) PLACEHOLDER);   // Feedback resolution
    sendSDO(MOTOR_CONT_ID, 0x2040, 0x07, (uint16_t) PLACEHOLDER);   // Electrical angle filter
    sendSDO(MOTOR_CONT_ID, 0x2040, 0x08, (uint16_t) PLACEHOLDER);   // Motor phase offset compensation
    sendSDO(MOTOR_CONT_ID, 0x6094, 0x01, (uint32_t) PLACEHOLDER);   // Velocity encoder factor Numerator
    sendSDO(MOTOR_CONT_ID, 0x6094, 0x02, (uint32_t) PLACEHOLDER);   // Velocity encoder factor Divisor

    // Configure velocity mode parameters.
    sendSDO(MOTOR_CONT_ID, 0x60FF, 0, (uint32_t) PLACEHOLDER);      // Target velocity
    sendSDO(MOTOR_CONT_ID, 0x606C, 0, (uint32_t) PLACEHOLDER);      // Velocity actual value
    sendSDO(MOTOR_CONT_ID, 0x60F9, 0x01, (uint16_t) PLACEHOLDER);   // Velocity control regulator P gain
    sendSDO(MOTOR_CONT_ID, 0x60F9, 0x02, (uint16_t) PLACEHOLDER);   // Velocity control regulator I gain
    sendSDO(MOTOR_CONT_ID, 0x6075, 0, (uint32_t) PLACEHOLDER);      // Motor rated current
}

int main()
{
    can.mode(CAN::Mode::LocalTest);
    CANMessage msg;
    while(1)
    {
        ThisThread::sleep_for(500ms);
        printf("Hello World!\n");
    }
}

