#include <mbed.h>
#include <rtos.h>

#if !DEVICE_CAN
#error [NOT_SUPPORTED] CAN not supported
#endif

Thread thread;

#define CAN_RD P0_30
#define CAN_TD P0_29

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

uint8_t data[4] = {1, 2, 3, 4};

DigitalOut led1(LED1);
DigitalOut led2(LED2);
// The constructor takes in RX, and TX pin respectively.
CAN can(p30, p29);
char counter = 0;

void sendCANOpenPacket(int functionCode, int nodeID, uint8_t* data)
{
    // Construct packetID (COB-ID).
    int packetID = (functionCode << 7) | nodeID;
    can.write(CANMessage(packetID, data));
    led1 = !led1;
}

int main()
{
    can.mode(CAN::Mode::LocalTest);
    CANMessage msg;
    while(1)
    {
        sendCANOpenPacket(T_SDO, 1, data);
        if (can.read(msg))
        {
            printf("Packet Recieved -> Function Code: (%d)\n", (msg.id >> 7));
            ThisThread::sleep_for(500ms);
            led2 = !led2;
        }
    }
}

