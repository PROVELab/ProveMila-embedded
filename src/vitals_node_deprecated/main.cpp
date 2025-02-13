#include "../common/pecan.hpp"
#include "Arduino.h"
#include "CAN.h"
#include "../arch/arduino.hpp"


const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;
const int thisSensorId=6;
const int numberOfNodes=1;

const int data1Size=1;
const int data2Size=2;
const int data3Size=4;
int startingSensorId=6;  //the starting id of sensor nodes, sensor nodes will be arranged in the order the appear in the text file. currently setting to three
struct heartBeatData{
    int flags;
    long milliVolts;
    long milliAmps;
    int milliSeconds;
};
heartBeatData ponged[numberOfNodes];    //used to store heartbeat responses when they are recieved, for description of each sub-array, goto listenForPongs function.
PCANListenParamsCollection plpc;

//Task t[MAX_PCAN_PARAMS];
PScheduler ts;
CANPacket packet;
PTask sampleTask;
//vitalsNode start

unsigned long heartBeatSentTime;
//to account for mcu having id 1, and vitals having id 2  513-> 1,2
inline void intToByte(int arrayPosition, int8_t arr[],int value){   //puts the int value into arr[arrayPosition] and arr[arrayPosition+1]
    arr[arrayPosition+1]=(value) &0b11111111;
    arr[arrayPosition]=(value>>8)&0b11111111;
}
inline int byteToInt(int arrayPosition, int8_t arr[]){  //retrieves the combined int value from arr[arrayPosition] and arr[arrayPosition+1]
    return (arr[arrayPosition]<<8)+(((int)arr[arrayPosition+1]));
}

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
    writeData(&packet, 0,  1);  //no need to write any data yet
    sendPacket(&packet);
    heartBeatSentTime=millis();
    Serial.print("sent millis");
    Serial.println(millis());
    ts.runOneTimeTask(checkBeats, 1000); //pass in task to execute and time to execution.
}

int16_t listenForPong(CANPacket * packet){ //a listen param that will process a response to heartbeat
    Serial.println(3);   
    int NodeId=(packet->id)&0b1111111;  //exclude function code, just the id
    int nodePosition=NodeId-startingSensorId;//location of this id in vital's node array. this location corresponds to the NodeId
    ponged[nodePosition].flags|=1; //mark node as having responded
    Serial.print("flags: ");
    //Serial.println(ponged[nodePosition].flags);
    ponged[nodePosition].milliSeconds=(int)(millis()-heartBeatSentTime);
    //Serial.println("recieved data");    //this is also problematic i think, data not necessarily 8 long
    /*
    for(int i=1;i<7;i++){
        for(int j=0;j<8;j++){
            Serial.print(bitRead(packet->data[i],j));
        }
        Serial.println();
    }*/
    ponged[nodePosition].milliVolts=0;
    ponged[nodePosition].milliAmps=0;

    memcpy(&ponged[nodePosition].milliVolts,&packet->data[1],3);
    memcpy(&ponged[nodePosition].milliAmps,&packet->data[4],3);
    /*
    Serial.println("copied memory");
    Serial.println(ponged[nodePosition].milliVolts,BIN);
    Serial.println(ponged[nodePosition].milliAmps,BIN);*/
    //moves sign over, removing where it was before
    if(bitRead(ponged[nodePosition].milliVolts,23)==1){
        //Serial.println("milliVolts is neg");
        ponged[nodePosition].milliVolts|=0xFF000000L;
    }else{
        //Serial.println("milliVolts is pos");
        //bitWrite(ponged[nodePosition].milliVolts,31,1);
        //bitWrite(ponged[nodePosition].milliVolts,23,0);
    }
    if(bitRead(ponged[nodePosition].milliAmps,23)==1){
                //Serial.println("milliAmps is neg");
        ponged[nodePosition].milliAmps|=0xFF000000L;
    }else{
                //Serial.println("milliAmps is pos");
        //bitWrite(ponged[nodePosition].milliAmps,31,1);
        //bitWrite(ponged[nodePosition].milliAmps,23,0);
    }
    /*
    Serial.println("final val");
    Serial.println(ponged[nodePosition].milliVolts,BIN);
    Serial.println(ponged[nodePosition].milliAmps,BIN);
    Serial.println("millivolts val: ");
    Serial.println(ponged[nodePosition].milliVolts);*/
    //ponged[nodePosition].milliVolts=byteToInt(2,packet->data);  //03,21  ->3, 513
    //ponged[nodePosition].milliAmps=byteToInt(4,packet->data);
     
    return 1;
}
int16_t listenForData(CANPacket *packet){
    int NodeId=(packet->id)&0b1111111;  //exclude function code, just the id
    int nodePosition=NodeId-startingSensorId;//location of this id in vital's node array. this location corresponds to the NodeId
    ponged[nodePosition].flags|=0b00000010;
    //Serial.println(ponged[nodePosition].flags);

    
    //will be moved to telemetry, will replace the prints with sending data w/ antena
    int8_t* data=packet->data;
    int8_t data1=data[0];
    int data2;
    memcpy(&data2,&data[data1Size],sizeof(int));//will also want to add signed versions
    long data3;
    memcpy(&data3,&data[data1Size+data2Size],sizeof(long));
    //long data3=*((long *)data[data2Size]);
    /*
    Serial.print("data 1, 2, 3: ");
    Serial.println(data1);
    Serial.println(data2);
    Serial.println(data3);*/
}
void checkPongs(){
    Serial.println(4);
    
    Serial.println("HeartBeat Info");
    for(int i=0;i<numberOfNodes;i++){
        
        int flags=ponged[i].flags;        
        if(flags&1){   //node responded
            char buffer[90];                                                                                  
            int milliSeconds=ponged[i].milliSeconds;
            long milliVolts=ponged[i].milliVolts;
            long milliAmps=ponged[i].milliAmps;
            //Serial.println("no sprint: ");
            //Serial.println(milliSeconds);
            //Serial.println(milliVolts);
            //Serial.println(milliAmps);
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
        ponged[i].flags=0;
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
int16_t respondToHeartBeat(CANPacket *){
    //Serial.println("resoinse CALLED");
        Serial.println(2);
        CANPacket packet;
        packet.id =combinedID(sendPong,sensorNodeID);    
        //Serial.println("id: ");
        //Serial.println(packet.id);
        int8_t flags=1;
        writeData(&packet, &flags,  1);  //no additional data= working properly
        long milliAmps=readMilliAmps();
        long milliVolts=readMilliVolts();
        int8_t milliVoltsArr[3]={0};
        int8_t milliAmpsArr[3]={0};
        /*
        if(milliVolts<=-8388608L){
            milliVoltsArr[2]=0b10000000;//just the sign
            writeData(&packet,milliVoltsArr,3)
        }else if(milliVolts>=8388608L){  //
            milliVoltsArr
        }
        if(milliAmps<=-8388608L){
            ret
        }*/
        /*
        Serial.println("pre-constain: ");
        Serial.println(milliVolts,BIN);
        Serial.println(milliAmps,BIN);*/
        milliAmps=constrain(milliAmps,-8388606L,8388606L);
        milliVolts=constrain(milliVolts,-8388606L,8388606L);
        /*
        Serial.println("post-constain: ");
        Serial.println(milliVolts,BIN);
        Serial.println(milliAmps,BIN);*/
        memcpy(milliVoltsArr,&milliVolts,3);
        memcpy(milliAmpsArr,&milliAmps,3);
        /*
        Serial.println("pre-sign: ");
        for(int i=0;i<3;i++){
            Serial.println(milliVoltsArr[i],BIN);
        }
        for(int i=0;i<3;i++){
            Serial.println(milliAmpsArr[i],BIN);
        }*/
        if(milliAmps<0){    //indicate if number is negative
            bitWrite(milliAmpsArr[2],7,1);
        }
        if(milliVolts<0){
            bitWrite(milliVoltsArr[2],7,1);
        }
        /*
        Serial.println("pre-send: ");
        for(int i=0;i<3;i++){
            Serial.println(milliVoltsArr[i],BIN);
        }
        for(int i=0;i<3;i++){
            Serial.println(milliAmpsArr[i],BIN);
        }
        Serial.println("");
        Serial.println(milliVolts,BIN);
        Serial.println(milliAmps,BIN);      */   
        writeData(&packet,milliVoltsArr,3);
        writeData(&packet,milliAmpsArr,3);

        
        /*
        if(abs(milliVolts)>=8388607){
            if(milliVolts>0){
                memcpy(milliVoltsArr,)
                writeData(&packet,8388607,3)
            }
            writeData(&packet,8388607)
        }else{
            
        }*/
        /*
        int8_t nodeData[4];
        intToByte(0,nodeData,milliVolts);
        intToByte(2,nodeData,milliAmps);*/
        
        //writeData(&packet, nodeData,  3);
        /*
        Serial.println("packet being sent");
        Serial.println(packet.id);
        
        Serial.println(packet.id);  
        for(int i=0;i<8;i++){   //cant do this, packet dsnt necessarily have size 8
            Serial.println(packet.data[i],BIN);
        }*/
        sendPacket(&packet);
    return 1;
}
int8_t collectData1(){
    return -30;
}
int collectData2(){
    return -1000;
}
long collectData3(){
    return -30000000;
}
void sendData(){   //3 options for data size, byte, int (2 bytes), or long (4 bytes). rn, everyhting is signed
    /* temp comment out
    int8_t data1=collectData1();
    int data2=collectData2();
    int8_t* data2Array=(int8_t*)(&data2);
    long data3=collectData3();
    int8_t* data3Array=(int8_t*)(&data3);


    CANPacket packet;
    packet.id =combinedID(transmitData,thisSensorId);    //
    writeData(&packet, &data1,  data1Size);  //no need to write any data yet
    writeData(&packet, data2Array,data2Size);
    writeData(&packet, data3Array,data3Size);
    sendPacket(&packet);*/
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
    // Serial.println("aqui");
    // Serial.println(8388607,BIN);
    // for(int i=0;i<24;i++){
    //     Serial.print(1);
    // }
    // Serial.println("");
    // Serial.println((-8388607),BIN);
    
    // Serial.println(sampleTask.delay);  //why this and not line above
    
    //Serial.println(packet.dataSize);
    
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    
    Serial.println("Vitals Starting");
    // CAN.loopback();
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
    
    
}

void loop() {
    delay(1000);
    Serial.println("loop start");
    ts.mainloop((PCANListenParamsCollection*) &plpc);
    //everything should be written as a task in the setup part. the ps.mainloop call never ends
    //u have up to 85.5% Ram that can be used, possibly less depending on future stuff added. for an idea of how class to current RAM u are at. just a hello World is 67%, so 76% means using about half of available space
}
