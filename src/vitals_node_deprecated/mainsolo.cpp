#include "../common/pecan.hpp"
#include "Arduino.h"
#include "CAN.h"
#include "../arch/arduino.hpp"

//Ids for specific pings sent to sensors
const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;
//Number of children Mommy duck has to ping
const int numberOfNodes=3;

int startingSensorId=6;  //the starting id of sensor nodes, sensor nodes will be arranged in the order the appear in the text file. currently setting to three
//Written in hard drive
const int8_t nodeData [numberOfNodes*5*8] PROGMEM={0};//each node gets 8 bytes per data chunk, holds 5 most recent data, shift everyhting 8 bytes to make room for new data when we get it.
//to access for example, the data for node2: 2*40 /*gets to index of first chunk of data*/ +0 first data chunk,  + 8 second data chunk, + 16 third data chunk, +24 fourth, +32 fifth. take each of these and process into long

//Struct for sensor ping
struct vitalsData{        
    int flags;      //different status of info
    long milliVolts; //are u alive? cool
    long milliAmps;
    int milliSeconds;
    int8_t data[8];   //sending what data actually holds //storing node data here currently not necessary for vitals, but likely will be in future depending on what alogirithm is used to decide node reliability (if it needs to be able to see previous data?) could make this 2d to store data further back asw
};
//Making nodes of vitals data struct
vitalsData nodes[numberOfNodes]={};

    //example: TP1-4 : Tire Pressure Sensor 1: 1 19 82 30 60 22 75: 0 -32768 32767 100 300  
//number 1   2-3     4-5              6-7(on present if isCritical)
// numbits, range, warning range, critical ranges. (critical range optional)

//Specific data for each sensor (tells us how to parse it later when deciphering code)
struct nodeInfo{
    int numData;
    long ranges[16];            //ranges for each data low-high
    int numBits[8];             //number of bits for each data   
    long warningRanges[16];     //"yellow" ranges for each data
    long criticalRanges[16];    //"red" ranges for each data if applicable
};
//Adding the constant data
const nodeInfo lookUpInfo[] PROGMEM= {    //a constanct lookup table initialized at compile time used to find information needed to decode a nodes data based on its id.
    {4,{19,82,-32768,32767,-100,-1,9,18}, {6,16,7,4},{30, 60,100,300,-80,-20, 11,15},{22,75,0,0,0,0,10,17}},
    {4,{14,89,-32761,35767,-101,-1,7,21}, {7,17,7,4},{30, 60,100,300,-80,-20, 11,15},{22,75,0,0,0,0,10,17}},
    {1,{-32780,32780},{17},{0, 100},{-5,100}}
}; 

nodeInfo nodeI;     //used to access stuff in lookup Info. lookupinfo is put in progmem so that memory usage doesnt increase drastically with more nodes. only ever copy over on value from array to nodeI
//
PCANListenParamsCollection plpc;

//Task t[MAX_PCAN_PARAMS];
PScheduler ts;
CANPacket packet;
PTask sampleTask;
//vitalsNode start

unsigned long heartBeatSentTime;    //used to record how long full message cycle takes
//tasks to be used make sure these get initialized before passing them anywhere
int sendBeat=-1;
int checkBeats=-1;
int genericPrint=-1;

void sendHeartbeat(){
    CANPacket packet;
    packet.id =combinedID(sendPing,vitalsID);    //
    Serial.println("sendingHB at time");
    writeData(&packet, (int8_t*)1,  1);  //no need to write any data yet
    sendPacket(&packet);
    heartBeatSentTime=millis();
    Serial.println(heartBeatSentTime);
    ts.runOneTimeTask(checkBeats, 100); //pass in task to execute and time to execution.
    Serial.println("done sending");
}

int16_t listenForPong(CANPacket * packet){ //a listen param that will process a response to heartbeat
    Serial.println(3);   
    int NodeId=(packet->id)&0b1111111;  //exclude function code, just the id
    int nodePosition=NodeId-startingSensorId;//location of this id in vital's node array. this location corresponds to the NodeId
    nodes[nodePosition].flags|=1; //mark node as having responded
    nodes[nodePosition].milliSeconds=(int)(millis()-heartBeatSentTime);
    nodes[nodePosition].milliVolts=0;
    nodes[nodePosition].milliAmps=0;

    memcpy(&nodes[nodePosition].milliVolts,&packet->data[1],3);
    memcpy(&nodes[nodePosition].milliAmps,&packet->data[4],3);
    
    if(bitRead(nodes[nodePosition].milliVolts,23)==1){
        //Serial.println("milliVolts is neg");
        nodes[nodePosition].milliVolts|=0xFF000000L;
    }
    if(bitRead(nodes[nodePosition].milliAmps,23)==1){
                //Serial.println("milliAmps is neg");
        nodes[nodePosition].milliAmps|=0xFF000000L;
    }
    return 1;
}

void printAllData(){
    Serial.println("all node data:");
    unsigned long timeToParse=millis();
    for(int i=0;i<numberOfNodes;i++){
        memcpy_P( &nodeI, &lookUpInfo[i], sizeof(nodeInfo));
        int dataPos=nodeI.numData;
        for(int j=0;j<nodeI.numData;j++){//for each data

            if(bitRead(nodes[i].data[j/8],j%8)){ //reads the Jth bit out of the Ith byte in array
                //the data is present
                unsigned long temp=0;
                for(int k=0;k<nodeI.numBits[j];k++){    //for each bit
                    bitWrite(temp,k,bitRead(nodes[i].data[(dataPos+k)/8],(dataPos+k)%8));
                }
                long print=temp+nodeI.ranges[j*2];//add back the bottom constraint
                char buffer[50];                                                                                  
            Serial.println(buffer);
                if(!(nodeI.criticalRanges[j*2]==0&&nodeI.criticalRanges[j*2+1])&&nodeI.criticalRanges[j*2]>print&&print>nodeI.criticalRanges[j*2+1]){
                    sprintf(buffer, "%ld critical: %ld-%ld",print,nodeI.criticalRanges[j*2],nodeI.criticalRanges[j*2+1]);



                }else if(nodeI.warningRanges[j*2]>print||print<nodeI.warningRanges[j*2+1]){
                    sprintf(buffer, "%ld warning: %ld-%ld",print,nodeI.warningRanges[j*2],nodeI.warningRanges[j*2+1]);
                }else{
                    sprintf(buffer, "%ld good: %ld-%ld",print,nodeI.warningRanges[j*2],nodeI.warningRanges[j*2+1]);
                }
                Serial.println(print);
            }
        dataPos+=nodeI.numBits[j];
        }
    }
    Serial.print("tTP");
    Serial.println(millis()-timeToParse);
}
int16_t listenForData(CANPacket *packet){
    
    //Serial.println("data recieved");
    int NodeId=(packet->id)&0b1111111;  //exclude function code, just the id
    int nodePosition=NodeId-startingSensorId;//location of this id in vital's node array. this location corresponds to the NodeId
    nodes[nodePosition].flags|=0b00000010;
    //Serial.println(nodes[nodePosition].flags);

    int8_t* data=packet->data;
    //nodes.data=data array.
    //const nodeInfo& nodeI= lookUpInfo[nodePosition];
    //const nodeInfo& nodeI = *((const nodeInfo*) pgm_read_word(&lookUpInfo) + nodePosition);
    //myStruct myArraySRAM;
    
    memcpy(&nodes[nodePosition].data,&(packet->data),8);//stores data
    if(nodePosition==numberOfNodes-1){//if we just got a message from the last node
        printAllData();
    }
/*
    for(int i=0;i<numberOfNodes;i++){
        memcpy_P( &nodeI, &lookUpInfo[i], sizeof(nodeInfo));
        int numData=nodeI.numData;
        for(int j=0;j<numData;j++){
            if(nodes[i].data[j]!=0){
            Serial.println(nodes[i].data[j]);
            }
        }
        Serial.println("break");
    }*/
    return 1;
}
void checkPongs(){
    Serial.println(4);
    
    Serial.println("HeartBeat Info");
    for(int i=0;i<numberOfNodes;i++){
        
        int flags=nodes[i].flags;        
        if(flags&1){   //node responded
            char buffer[90];                                                                                  
            int milliSeconds=nodes[i].milliSeconds;
            long milliVolts=nodes[i].milliVolts;
            long milliAmps=nodes[i].milliAmps;
            sprintf(buffer, "node %d responded in %d ms and is recieving %ld mV and %ld mA",i,milliSeconds,milliVolts,milliAmps);
            Serial.println(buffer);
            
        }else{// problem
            Serial.print("node ");
            Serial.print(i);
            Serial.println(" did not respond with heart beat!"); //vitals/telemetry node will use this information in error report
        }
        if(!(flags&2)){//data has not been updated since last time this ran
            Serial.print("node ");
            Serial.print(i);
            Serial.println(" has not sent any data");
        }
        nodes[i].flags=0;
    }    
    
}
//vitalsNode end

//genericNode start

const int sensorNodeID=0b0000110;   //CHANGE THIS VALUE TO THE CAN ID OF YOUR NODE
bool responses[]={1,1,1,1,1};
int respindex=0;

long readMilliAmps(){
    return 74982;
    /*
    long ret=random(-2147483647L,2147483647L);
    Serial.print("generatedmA: ");
    Serial.println(ret);
    return ret;*/
}
long readMilliVolts(){
    return 484327;
    /*
    long ret=random(-2147483647L,2147483647L);
    Serial.print("generatedmV: ");
    Serial.println(ret);
    return ret;*/
}
//more than 85.5% Ram usage= CAN startup fails!
void setup() {
    Serial.begin(9600);
    Serial.println("solo");    
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    Serial.println("Vitals Starting");
    //CAN.loopback();    
    PTask mamaDuck;
    mamaDuck.function = sendHeartbeat;
    mamaDuck.interval = 20000;
    sendBeat=ts.scheduleTask(&mamaDuck);    
    
    PTask checkHeartBeats;  //we will call this to run 250ms after heatbeat
    checkHeartBeats.function=checkPongs;
    checkHeartBeats.interval=10;//can be any number doesnt matter, setting it to something just in case
    checkBeats=ts.scheduleOneTimeTask(&checkHeartBeats);
    
    CANListenParam processBeat;
    processBeat.handler=listenForPong;
    processBeat.listen_id =combinedID(sendPong,vitalsID);//id Doesnt matter
    processBeat.mt=MATCH_FUNCTION;
    if (addParam(&plpc,processBeat)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }

    CANListenParam processData;
    processData.handler=listenForData;
    processData.listen_id =combinedID(transmitData,vitalsID);//id Doesnt matter
    processData.mt=MATCH_FUNCTION;
    if (addParam(&plpc,processData)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
    //vitalsNode end

    //genericNode start
    /*
    CANListenParam babyDuck;
    babyDuck.handler=respondToHeartBeat;
    babyDuck.listen_id =combinedID(sendPing,vitalsID);
    babyDuck.mt=MATCH_EXACT;
    if (addParam(&plpc,babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
    
    PTask updateData;  //we will call this to run 250ms after heatbeat
    updateData.function=sendData;
    updateData.interval=88;//can be any number doesnt matter, setting it to something just in case
    int whatever=ts.scheduleTask(&updateData);
    //genericNode end
    */
    
}

void loop() {
    delay(1000);
    Serial.println("loop start");
    ts.mainloop((PCANListenParamsCollection*) &plpc);
    //everything should be written as a task in the setup part. the ps.mainloop call never ends
    //u have up to 85.5% Ram that can be used, possibly less depending on future stuff added. for an idea of how class to current RAM u are at. just a hello World is 67%, so 76% means using about half of available space
}
