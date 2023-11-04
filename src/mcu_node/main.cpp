#include <mbed.h>
#include <rtos.h>

#if !DEVICE_CAN
#error[NOT_SUPPORTED] CAN not supported
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

void sendCANOpenPacket(int functionCode, int nodeID, uint8_t *data)
{
  // Construct packetID (COB-ID).
  int packetID = (functionCode << 7) | nodeID;
  can.write(CANMessage(packetID, data));
  led1 = !led1;
}

// Function, sendSDO, sends an array of data of at most 4 bytes in size.
void sendSDO(int nodeID, uint16_t index, uint8_t subindex, uint8_t *data)
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
  sendSDO(MOTOR_CONT_ID, 0x1600, 0x00, (uint8_t)1);           // Declare single value for channel.
  sendSDO(MOTOR_CONT_ID, 0x1600, 0x01, (uint16_t)0x60710010); // Map target torque to value #1.

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
  sendSDO(MOTOR_CONT_ID, 0x6060, 0, (uint8_t)9); // 9: Velocity mode

  // Set motor type.
  sendSDO(MOTOR_CONT_ID, 0x2034, 0, buffer);

  // Set controller/motor protections.
  sendSDO(MOTOR_CONT_ID, OVERVOLTAGE_LIMIT, 0, (uint16_t)PLACEHOLDER);                                         // Overvoltage limit
  sendSDO(MOTOR_CONT_ID, UNDERVOLTAGE_LIMIT, UNDERVOLTAGE_LIMIT_SUBINDEX, (uint8_t)PLACEHOLDER);               // Undervoltage limit
  sendSDO(MOTOR_CONT_ID, UNDERVOLTAGE_MIN_VOLTAGE, UNDERVOLTAGE_MIN_VOLTAGE_SUBINDEX, (uint16_t)PLACEHOLDER);  // Undervoltage min voltage
  sendSDO(MOTOR_CONT_ID, MAXIMUM_CONTROLLER_CURRENT, 0, (uint32_t)PLACEHOLDER);                                // Maximum controller current
  sendSDO(MOTOR_CONT_ID, SECONDARY_CURRENT_PROTECTION, 0, (uint32_t)PLACEHOLDER);                              // Secondary current protection
  sendSDO(MOTOR_CONT_ID, MOTOR_MAXIMUM_TEMPERATURE, MOTOR_MAXIMUM_TEMPERATURE_SUBINDEX, (uint8_t)PLACEHOLDER); // Motor maximum temperature
  sendSDO(MOTOR_CONT_ID, MAXIMUM_VELOCITY, MAXIMUM_VELOCITY_SUBINDEX, (uint32_t)PLACEHOLDER);                  // Maximum velocity

  // Perform motor position measurement.
  sendSDO(MOTOR_CONT_ID, MOTOR_POLE_PAIRS, 0, (uint8_t)PLACEHOLDER);                                                            // Motor pole pairs
  sendSDO(MOTOR_CONT_ID, FEEDBACK_TYPE, FEEDBACK_TYPE_SUBINDEX, (uint8_t)PLACEHOLDER);                                          // Feedback type
  sendSDO(MOTOR_CONT_ID, MOTOR_PHASE_OFFSET, MOTOR_PHASE_OFFSET_SUBINDEX, (uint16_t)PLACEHOLDER);                               // Motor phase offset
  sendSDO(MOTOR_CONT_ID, HALL_CONFIGURATION, HALL_CONFIGURATION_SUBINDEX, (uint8_t)PLACEHOLDER);                                // Hall configuration
  sendSDO(MOTOR_CONT_ID, FEEDBACK_RESOLUTION, FEEDBACK_RESOLUTION_SUBINDEX, (uint16_t)PLACEHOLDER);                             // Feedback resolution
  sendSDO(MOTOR_CONT_ID, ELECTRICAL_ANGLE_FILTER, ELECTRICAL_ANGLE_FILTER_SUBINDEX, (uint16_t)PLACEHOLDER);                     // Electrical angle filter
  sendSDO(MOTOR_CONT_ID, MOTOR_PHASE_OFFSET_COMPENSATION, MOTOR_PHASE_OFFSET_COMPENSATION_SUBINDEX, (uint16_t)PLACEHOLDER);     // Motor phase offset compensation
  sendSDO(MOTOR_CONT_ID, VELOCITY_ENCODER_FACTOR_NUMERATOR, VELOCITY_ENCODER_FACTOR_NUMERATOR_SUBINDEX, (uint32_t)PLACEHOLDER); // Velocity encoder factor Numerator
  sendSDO(MOTOR_CONT_ID, VELOCITY_ENCODER_FACTOR_DIVISOR, VELOCITY_ENCODER_FACTOR_DIVISOR_SUBINDEX, (uint32_t)PLACEHOLDER);     // Velocity encoder factor Divisor

  // Configure velocity mode parameters.
  sendSDO(MOTOR_CONT_ID, TARGET_VELOCITY, 0, (uint32_t)PLACEHOLDER);                                                            // Target velocity
  sendSDO(MOTOR_CONT_ID, VELOCITY_ACTUAL_VALUE, 0, (uint32_t)PLACEHOLDER);                                                      // Velocity actual value
  sendSDO(MOTOR_CONT_ID, VELOCITY_CONTROL_REGULATOR_P_GAIN, VELOCITY_CONTROL_REGULATOR_P_GAIN_SUBINDEX, (uint16_t)PLACEHOLDER); // Velocity control regulator P gain
  sendSDO(MOTOR_CONT_ID, VELOCITY_CONTROL_REGULATOR_I_GAIN, VELOCITY_CONTROL_REGULATOR_I_GAIN_SUBINDEX, (uint16_t)PLACEHOLDER); // Velocity control regulator I gain
  sendSDO(MOTOR_CONT_ID, MOTOR_RATED_CURRENT, 0, (uint32_t)PLACEHOLDER);                                                        // Motor rated current
}

int main()
{
  can.mode(CAN::Mode::LocalTest);
  CANMessage msg;
  while (1)
  {
    ThisThread::sleep_for(500ms);
    printf("Hello World!\n");
  }
}
