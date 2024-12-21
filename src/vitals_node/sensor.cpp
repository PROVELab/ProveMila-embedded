#include "../common/pecan.hpp"
#include "Arduino.h"
#include "CAN.h"
#include "../arch/arduino.hpp"

//universal globals
const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;
PCANListenParamsCollection plpc;
PScheduler ts;

//name1 globals
const int name1Id=6;
int8_t name1dataArray[8];
int8_t name1VitalsFlags=0;  //0b1=startup, 0b0111 can be for whatver other things we end up wanting idk

//name2 globas
const int name2Id=7;
int8_t name2dataArray[8];
int8_t name2VitalsFlags=0;

//name3 globls
const int name3Id=8;
int8_t name3dataArray[8];
int8_t name3VitalsFlags=0;



long readMilliAmps(){
    return 74981;
    /*
    long ret=random(-2147483647L,2147483647L);
    Serial.print("generatedmA: ");
    Serial.println(ret);
    return ret;*/
}
long readMilliVolts(){
    return 484326;
    /*
    long ret=random(-2147483647L,2147483647L);
    Serial.print("generatedmV: ");
    Serial.println(ret);
    return ret;*/
}
//name1

int16_t name1RespondToHeartBeat(CANPacket *){
        delay(1);
        CANPacket packet;
        packet.id =combinedID(sendPong,name1Id);    
        writeData(&packet, &name1VitalsFlags,  1);  
        name1VitalsFlags=0;
        long milliAmps=1111;
        long milliVolts=1111;
        int8_t milliVoltsArr[3]={0};
        int8_t milliAmpsArr[3]={0};
        
        milliAmps=constrain(milliAmps,-8388606L,8388606L);
        milliVolts=constrain(milliVolts,-8388606L,8388606L);
        
        memcpy(milliVoltsArr,&milliVolts,3);
        memcpy(milliAmpsArr,&milliAmps,3);
        if(milliAmps<0){    //indicate if number is negative
            bitWrite(milliAmpsArr[2],7,1);
        }
        if(milliVolts<0){
            bitWrite(milliVoltsArr[2],7,1);
        }
        writeData(&packet,milliVoltsArr,3);
        writeData(&packet,milliAmpsArr,3);
        Serial.println("N1RHB");
        sendPacket(&packet);
    return 1;
}

long name1collectData1(){
    return 40;
}
long name1collectData2(){
    long ret=random(33767);
    ret*= random(2)*2-1;//makes ret negative half of the time 
    Serial.print("GD1.2: ");
    Serial.println(ret);
    return ret;
}
long name1collectData3(){
    long ret=random(120);
    ret*= -1;//makes ret negative 
    Serial.print("GD1.3: ");
    Serial.println(ret);
    return ret;
}
long name1collectData4(){
    long ret=random(9);
    ret+=11;
    Serial.print("GD1.4: ");
    Serial.println(ret);
    return ret;
}
void name1SendData(){
    CANPacket packet;
    packet.id =combinedID(transmitData,name1Id);
    writeData(&packet,&name1dataArray[0],8);
    sendPacket(&packet);
}

//will be called data after most commonly collected data is collect.
void name1ProcessData1234(){   //3 options for data size, byte, int (2 bytes), or long (4 bytes). rn, everyhting is signed
    int numData=4;  //the number of dataPresent
    int bitIndex=numData;
    //data1:
    if(true){//insert a given condition for collecting data, could be a timer, may switch each data collection to tasks later on
    long data1=name1collectData1();
    data1=constrain(data1,19,82);   //constrain to bounds
    unsigned long udata1=data1-19;  //subtract lower bound
    int data1Bits=6;    // will perform a logarithm in pyhton script to find this value: ciel(log(b2)(top-bot+1))
    for(int i=0;i<data1Bits;i++){
        bitWrite(name1dataArray[bitIndex/8],bitIndex%8,bitRead(udata1,i));
        bitIndex++;
    }
    //indicate we have sent this data
    bitWrite(name1dataArray[0],0,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    //data2:
    if(true){
        long data2=name1collectData2();
    data2=constrain(data2,-32768,32767);
    unsigned long udata2=data2+32768;   //will need to make positive if lower bound is negative
    int data2Bits=16;
    for(int i=0;i<data2Bits;i++){
        bitWrite(name1dataArray[bitIndex/8],bitIndex%8,bitRead(udata2,i));
        bitIndex++;
    }
        bitWrite(name1dataArray[0],1,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    
    //data3:
    if(true){
        long data4=name1collectData3();
    data4=constrain(data4,-100,-1);
    unsigned long udata4=data4+100;   //will need to make positive if lower bound is negative
    int data4Bits=7;
    for(int i=0;i<data4Bits;i++){
        bitWrite(name1dataArray[bitIndex/8],bitIndex%8,bitRead(udata4,i));
        bitIndex++;
    }
        bitWrite(name1dataArray[0],2,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    //data4:
    if(true){
        long data5=name1collectData4();
    data5=constrain(data5,9,18);
    unsigned long udata5=data5-9;   //will need to make positive if lower bound is negative
    int data5Bits=4;//must be exact size as needed
    for(int i=0;i<data5Bits;i++){
        bitWrite(name1dataArray[bitIndex/8],bitIndex%8,bitRead(udata5,i));
        bitIndex++;
    }
        bitWrite(name1dataArray[0],3,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    name1SendData();    //will execute after most commonly run data collection, feel free to move this around to be after most least common data, or whatever
}

//name2
int16_t name2RespondToHeartBeat(CANPacket *){
        CANPacket packet;
        packet.id =combinedID(sendPong,name2Id);    
        writeData(&packet, &name2VitalsFlags,  1);  
        name2VitalsFlags=0;
        long milliAmps=2222;
        long milliVolts=2222;
        int8_t milliVoltsArr[3]={0};
        int8_t milliAmpsArr[3]={0};
        
        milliAmps=constrain(milliAmps,-8388606L,8388606L);
        milliVolts=constrain(milliVolts,-8388606L,8388606L);
        
        memcpy(milliVoltsArr,&milliVolts,3);
        memcpy(milliAmpsArr,&milliAmps,3);
        if(milliAmps<0){    //indicate if number is negative
            bitWrite(milliAmpsArr[2],7,1);
        }
        if(milliVolts<0){
            bitWrite(milliVoltsArr[2],7,1);
        }
        writeData(&packet,milliVoltsArr,3);
        writeData(&packet,milliAmpsArr,3);
        Serial.println("N2RHB");
        sendPacket(&packet);
    return 1;
}
long name2collectData1(){
    return 40;
}
long name2collectData2(){
    long ret=random(33767);
    ret*= random(2)*2-1;//makes ret negative half of the time 
    Serial.print("GD2.2: ");
    Serial.println(ret);
    return ret;
}
long name2collectData3(){
    long ret=random(120);
    ret*= -1;//makes ret negative 
    Serial.print("GD2.3: ");
    Serial.println(ret);
    return ret;
}
long name2collectData4(){
    long ret=random(9);
    ret+=11;
    Serial.print("GD2.4: ");
    Serial.println(ret);
    return ret;
}
void name2SendData(){
    CANPacket packet;
    packet.id =combinedID(transmitData,name2Id);    
    writeData(&packet,&name2dataArray[0],8);
    sendPacket(&packet);
}

void name2ProcessData1234(){
    int numData=4;  //the number of dataPresent
    int bitIndex=numData;
    //data1:
    if(true){//insert a given condition for collecting data, could be a timer, may switch each data collection to tasks later on
    long data1=name2collectData1();
    data1=constrain(data1,14,89);   //constrain to bounds
    unsigned long udata1=data1-14;  //subtract lower bound
    int data1Bits=7;    // will perform a logarithm in pyhton script to find this value: ciel(log(b2)(top-bot+1))
    for(int i=0;i<data1Bits;i++){
        bitWrite(name2dataArray[bitIndex/8],bitIndex%8,bitRead(udata1,i));
        bitIndex++;
    }
    //indicate we have sent this data
    bitWrite(name2dataArray[0],0,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    //data2:
    if(true){
        long data2=name2collectData2();
    data2=constrain(data2,-32761,35767);
    unsigned long udata2=data2+32761;   //will need to make positive if lower bound is negative
    int data2Bits=17;
    for(int i=0;i<data2Bits;i++){
        bitWrite(name2dataArray[bitIndex/8],bitIndex%8,bitRead(udata2,i));
        bitIndex++;
    }
        bitWrite(name2dataArray[0],1,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    
    //data3:
    if(true){
        long data4=name2collectData3();
    data4=constrain(data4,-101,-1);
    unsigned long udata4=data4+101;   //will need to make positive if lower bound is negative
    int data4Bits=7;
    for(int i=0;i<data4Bits;i++){
        bitWrite(name2dataArray[bitIndex/8],bitIndex%8,bitRead(udata4,i));
        bitIndex++;
    }
        bitWrite(name2dataArray[0],2,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    //data4:
    if(true){
        long data5=name2collectData4();
    data5=constrain(data5,7,21);
    unsigned long udata5=data5-7;   //will need to make positive if lower bound is negative
    int data5Bits=4;//must be exact size as needed
    for(int i=0;i<data5Bits;i++){
        bitWrite(name2dataArray[bitIndex/8],bitIndex%8,bitRead(udata5,i));
        bitIndex++;
    }
        bitWrite(name2dataArray[0],3,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    name2SendData();
}

//name3

int16_t name3RespondToHeartBeat(CANPacket *){
        CANPacket packet;
        packet.id =combinedID(sendPong,name3Id);    
        writeData(&packet, &name3VitalsFlags,  1);  
        name3VitalsFlags=0;
        long milliAmps=3333;
        long milliVolts=3333;
        int8_t milliVoltsArr[3]={0};
        int8_t milliAmpsArr[3]={0};
        
        milliAmps=constrain(milliAmps,-8388606L,8388606L);
        milliVolts=constrain(milliVolts,-8388606L,8388606L);
        
        memcpy(milliVoltsArr,&milliVolts,3);
        memcpy(milliAmpsArr,&milliAmps,3);
        if(milliAmps<0){    //indicate if number is negative
            bitWrite(milliAmpsArr[2],7,1);
        }
        if(milliVolts<0){
            bitWrite(milliVoltsArr[2],7,1);
        }
        writeData(&packet,milliVoltsArr,3);
        writeData(&packet,milliAmpsArr,3);
        Serial.println("N2RHB");
        sendPacket(&packet);
    return 1;
}
long name3CollectData1(){
    long ret=random(60);
    Serial.print("GD3.1: ");//a bit backwards but watev
    Serial.println(ret);
    return ret;
}
void name3SendData(){
    CANPacket packet;
    packet.id =combinedID(transmitData,name3Id);    //
    writeData(&packet,&name3dataArray[0],8);
    sendPacket(&packet);
    memset(name3dataArray,0,sizeof(name3dataArray));    //reset all data here
}
void name3ProcessData1(){    
    int numData2=1;  //the number of dataPresent
    int bitIndex2=numData2;
    //data3:
    if(true){
        long data3=name3CollectData1();
    data3=constrain(data3,-32780,32780);
    unsigned long udata3=data3+32780;   //will need to make positive if lower bound is negative
    int data3Bits=17;
    for(int i=0;i<data3Bits;i++){
        bitWrite(name3dataArray[bitIndex2/8],bitIndex2%8,bitRead(udata3,i));
        bitIndex2++;
    }
    bitWrite(name3dataArray[0],0,1); //can be done in python, checking to make sure no overflow here if decide to have more than 8 data later
    }
    name3SendData();
}


//more than 85.5% Ram usage= CAN startup fails!
void setup() {
    Serial.begin(9600);
    Serial.println("senseor");
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    randomSeed(100);
    // CAN.loopback(); i think this be frickin things up sometime idrk

    //name1
    CANListenParam name1babyDuck;
    name1babyDuck.handler=name1RespondToHeartBeat;
    name1babyDuck.listen_id =combinedID(sendPing,vitalsID);
    name1babyDuck.mt=MATCH_EXACT;
    
    if (addParam(&plpc,name1babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
    PTask name1Data1234;  
    name1Data1234.function=name1ProcessData1234;
    name1Data1234.interval=10000;
    ts.scheduleTask(&name1Data1234);

    //if applicable, add another task that will attempt adding on another data less often


    //name2
    CANListenParam name2babyDuck;
    name2babyDuck.handler=name2RespondToHeartBeat;
    name2babyDuck.listen_id =combinedID(sendPing,vitalsID);
    name2babyDuck.mt=MATCH_EXACT;
    if (addParam(&plpc,name2babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }

    PTask name2Data1234;  
    name2Data1234.function=name2ProcessData1234;
    name2Data1234.interval=10007;
    ts.scheduleTask(&name2Data1234);

    //name3
    CANListenParam name3babyDuck;
    name3babyDuck.handler=name3RespondToHeartBeat;
    name3babyDuck.listen_id =combinedID(sendPing,vitalsID);
    name3babyDuck.mt=MATCH_EXACT;
    if (addParam(&plpc,name3babyDuck)!= SUCCESS){    //plpc declared above setup()
        Serial.println("plpc no room");
        while(1);
    }
    PTask name3Data1;  
    name3Data1.function=name3ProcessData1;
    name3Data1.interval=10015;
    ts.scheduleTask(&name3Data1);
}

void loop() {
    delay(1000);
    Serial.println("loop start");
    ts.mainloop((PCANListenParamsCollection*) &plpc);
    //everything should be written as a task in the setup part. the ps.mainloop call never ends
    //u have up to 85.5% Ram that can be used, possibly less depending on future stuff added. for an idea of how class to current RAM u are at. just a hello World is 67%, so 76% means using about half of available space
}
