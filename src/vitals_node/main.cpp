#include "../common/pecan.hpp"
#include "Arduino.h"
#include "CAN.h"
#include "../arch/arduino.hpp"


const int vitalsID=0b0000010;
const int sendPing=0b0000;
const int sendPong=0b0001;

const int numberOfNodes=1;
int startingSensorId=3;  //the starting id of sensor nodes, sensor nodes will be arranged in the order the appear in the text file. currently setting to three
uint32_t ponged[numberOfNodes]={0};

PCANListenParamsCollection plpc;

//Task t[MAX_PCAN_PARAMS];
PScheduler ts;
CANPacket packet;
PTask sampleTask;
//vitalsNode start

//to account for mcu having id 1, and vitals having id 2


//tasks to be used make sure these get initialized before passing them anywhere
int sendBeat=-1;
int checkBeats=-1;
int genericPrint=-1;

//generic sensor node
int respondToBeat=-1;

//declare one time tasks at the top, so this node will have access to them to tell them to run when it wants to


//layout for data sent in heartbeat pong, currently uses 2 bytes. 10 most significant bits=arduinos voltage.  6 least significant bits=special flags for how arduino is doing. layout expanded on below
// 0b      1 1 1 1 1 1 1 1 1 1                                              1 1 1                         1                      1                  1
//   supplied voltage in deciVolts, should be between           //reserved for any future use        //data too low        //data too high      //responded
//   70 and 120 absolute min is 50, absolute max is 200.           
//   10 bits can hold values between 0-1024 (0-100 Volts)

void sendHeartbeat(){
    CANPacket packet;
    packet.id =combinedID(sendPing,vitalsID);    //
    Serial.println(1);
    Serial.println(packet.id);
    writeData(&packet, 0,  1);  //no need to write any data yet
    sendPacket(&packet);
    ts.runOneTimeTask(checkBeats, 1000); //pass in task to execute and time to execution.
}

int16_t listenForPong(CANPacket * packet){ //a listen param that will process a response to heartbeat
    int NodeId=(packet->id)&0b1111111;  //exclude function code, just the id
    int pos=NodeId-startingSensorId;//location of this id in vital's node array
    ponged[pos]=1;  //mark node as having responded in future, this should encode all necessary information from the packet
    Serial.println(3);
    
    return 1;
}
void harassNode(){
    Serial.println("will begin harassing Node");
}
void checkPongs(){
    Serial.println(4);
    //Serial.println("checkingHeartBeats");
    //Serial.println("all nodes succesful heartbeat");
    
    bool problem=false;
    
    for(int i=0;i<numberOfNodes;i++){
        
        uint32_t response=ponged[i];
        /*
        if(response!=1){   //problem with nodesHeartbeat
            problem=true;   //will not send success message at end of iteration
            
            if(response==0){  //no pong recieved
                Serial.print("node ");
                Serial.print(i);
                Serial.println(" did not respond with heart beat!"); //vitals/telemetry node will use this information in error report
            }else{
                for(int j=0;j<6;j++){   //iterate through all problems
                    if(bitRead(response,j)==1){ //problem indicated
                        
                    }
                    uint32_t deciVolts=response>>6;
                }
            }
        }else{  //no problem with nodesHeartbeat
               //setup for following iteration
        }*/
        //ponged[i]=-1;
     
    }
    if(!problem){   //all nodes responded to heartbeat with 0
    }
}
//vitalsNode end

//genericNode start

const int sensorNodeID=0b0000110;   //CHANGE THIS VALUE TO THE CAN ID OF YOUR NODE
int8_t currentStatus=0;    //update current Status when you check your own data.


int16_t respondToHeartBeat(CANPacket *){
    Serial.println(20000);
    CANPacket packet;
    packet.id =combinedID(sendPong,sensorNodeID);    
    writeData(&packet, &currentStatus,  1);  //no additional data= working properly
    sendPacket(&packet);
    return 1;
}

//genericNode end

//end of stuff to delete for can impl


void anotherPrint(){
    //Serial.println("otherPrint");
//example send an int: 
//int16_t x = 5;
//writeData(&p, (int8_t*) &x, 2);
}

//more than 85.5% Ram usage= CAN startup fails!
void setup() {
    Serial.begin(9600);
    // delay(1000);
    
    // Serial.println(sampleTask.delay);  //why this and not line above
    
    //Serial.println(packet.dataSize);
    
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    
    Serial.println("Vitals Starting");
    CAN.loopback();
    //vitals node start
    PTask mamaDuck;
    mamaDuck.function = sendHeartbeat;
    mamaDuck.interval = 5000;
    sendBeat=ts.scheduleTask(&mamaDuck);
    
    PTask anotherTask;
    anotherTask.function =anotherPrint;
    anotherTask.interval=1000;  
    genericPrint=ts.scheduleTask(&anotherTask);
    
    
    PTask checkHeartBeats;  //we will call this to run 250ms after heatbeat
    checkHeartBeats.function=checkPongs;
    checkHeartBeats.interval=10;//can be any number doesnt matter, setting it to something just in case
    checkBeats=ts.scheduleOneTimeTask(&checkHeartBeats);
    Serial.println(checkBeats);
    
    CANListenParam processBeat;
    processBeat.handler=listenForPong;
    processBeat.listen_id =combinedID(sendPong,vitalsID);//id Doesnt matter
    processBeat.mt=MATCH_FUNCTION;
    if (addParam(&plpc,processBeat)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
    //vitalsNode end

    //genericNode start
    
    CANListenParam babyDuck;
    babyDuck.handler=respondToHeartBeat;
    babyDuck.listen_id =combinedID(sendPing,vitalsID);
    babyDuck.mt=MATCH_EXACT;
    if (addParam(&plpc,babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
    
    //genericNode end
    
    
}

void loop() {
    delay(1000);
    Serial.println("loop start");
    ts.mainloop((PCANListenParamsCollection*) &plpc);
    //everything should be written as a task in the setup part. the ps.mainloop call never ends
    //u have up to 85.5% Ram that can be used, possibly less depending on future stuff added. for an idea of how class to current RAM u are at. just a hello World is 67%, so 76% means using about half of available space
}
