#include "mbed.h"
#include "../common/pecan.hpp"
#include "Timer.h"

// Creates static event queues
static EventQueue repeatingEvents(0);
static EventQueue checkHeartBeatQueue(0);

//stuff to put in another file at some point

const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;
const int numberOfNodes=3;
int startingSensorId=6;  //the starting id of sensor nodes, sensor nodes will be arranged in the order the appear in the text file. currently setting to three


void bitWrite(int32_t *x, char n, char value) {
   if (value)
      *x |= (1 << n);
   else
      *x &= ~(1 << n);
}

int bitRead(int32_t *x, char n) {
   return (*x & (1 << n)) ? 1 : 0;
}

//end of stuff for another file

struct vitalsData{        
    int16_t flags;      //different status of info
    int32_t milliVolts; //are u alive? cool
    int32_t milliAmps;
    int16_t milliSeconds;
    int8_t data[8][10];   //sending what data actually holds //storing node data here currently not necessary for vitals, but likely will be in future depending on what alogirithm is used to decide node reliability (if it needs to be able to see previous data?) could make this 2d to store data further back asw
    int8_t dataLocation;    //stores most recent data location.
};
//Making nodes of vitals data struct
vitalsData nodes[numberOfNodes]={};

void checkResponses(){
    printf("checking Beats \n");
    for(int i=0;i<numberOfNodes;i++){
        
        int flags=nodes[i].flags;        
        if(flags&1){   //node responded
            int milliSeconds=nodes[i].milliSeconds;
            long milliVolts=nodes[i].milliVolts;
            long milliAmps=nodes[i].milliAmps;
            printf("node %d responded in %d ms and is recieving %ld mV and %ld mA \n",i,milliSeconds,milliVolts,milliAmps);
           
        }else{// problem
            printf("node %d did not respond with heart beat! \n", i);
        }
        if(!(flags&2)){//data has not been updated since last time this ran
            printf("node %d has not sent any data \n", i);
        }
        nodes[i].flags=0;
}
}
auto checkHeartBeat = checkHeartBeatQueue.make_user_allocated_event(checkResponses);
//returns milliseconds ellapsed since program start, can be used throughout program similar to arduino millis()
//code taken from: https://stackoverflow.com/questions/31255486/how-do-i-convert-a-stdchronotime-point-to-long-and-back. 
//chrono::milliseconds is guarenteed to be stored on a signedvalue of at least 45 bits. so should have 557 years before potential overflow: https://en.cppreference.com/w/cpp/chrono/duration
int64_t getMs(){ 
    auto now = Kernel::Clock::now(); //uses kernel clock instead of system clock, since system clock only has 1 second precision
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch();
    return int64_t(value.count());
}
    int8_t deadData[4]={ (int8_t)0xDE, (int8_t)0xAD, (int8_t)0xFA, (int8_t)0xCE};
int64_t HBSendTime;
void sendHeartBeat(){
    printf("sending heartbeat \n");
    CANPacket packet;
    packet.id =combinedID(sendPing,vitalsID);    //

    //    writeData(&packet, (int8_t*)1,  1);  //no need to write any data yet
    writeData(&packet, deadData,  4);  //no need to write any data yet
    sendPacket(&packet);    
    //auto HBSendTime= std::chrono::time_point_cast<std::chrono::milliseconds>(Kernel::Clock::now()).count();
    HBSendTime=getMs();
    //const std::chrono::time_point<std::chrono::system_clock> HBSendTime = Kernel::Clock::now();    //is similar to arduino millis(). returns milliseconds ellapsed since program start

    //try call attempts to schedule the heartbeat check to be executed in 250ms.
    //it will fail to schedule it if the check has already been scheduled. However,
    //this should never happen, because heart beats are sent every 1000ms, so the 
    //previous check should have already executed.
    checkHeartBeat.try_call();
}
auto sendHBEvent = repeatingEvents.make_user_allocated_event(sendHeartBeat);

int16_t listenForPong(CANPacket * packet){ //a listen param that will process a response to heartbeat. function attached to processBeat in main()
    printf("recievedPong");
    int NodeId=(packet->id)&0b1111111;  //exclude function code, just the id
    int nodePosition=NodeId-startingSensorId;//location of this id in vital's node array. this location corresponds to the NodeId
    printf("\nnodePos: %d", nodePosition);
    nodes[nodePosition].flags|=1; //mark node as having responded
    nodes[nodePosition].milliSeconds=(int16_t)(getMs()-HBSendTime);  
    //printf("time: %d \n", nodes[nodePosition].milliSeconds);
    nodes[nodePosition].milliVolts=0;
    nodes[nodePosition].milliAmps=0;
    memcpy(&nodes[nodePosition].milliVolts,&packet->data[1],3);
    memcpy(&nodes[nodePosition].milliAmps,&packet->data[4],3);
    
    if(bitRead(&nodes[nodePosition].milliVolts,23)==1){
        //Serial.println("milliVolts is neg");
        nodes[nodePosition].milliVolts|=0xFF000000L;
    }
    if(bitRead(&nodes[nodePosition].milliAmps,23)==1){
                //Serial.println("milliAmps is neg");
        nodes[nodePosition].milliAmps|=0xFF000000L;
    }

    printf("milliVolts: %ld",nodes[nodePosition].milliVolts);
    return 1;
}

PCANListenParamsCollection plpc;

int main()
{
    printf("*** start ***\n");
    
    checkHeartBeat.delay(250); //wait 200ms after sending HB to check responses. no need to assign period because its a one time event (only ever run through dispatch_once())
    sendHBEvent.delay(1002);    //send a HB every second
    sendHBEvent.period(1000);
    

    CANListenParam processBeat;
    processBeat.handler=listenForPong;
    processBeat.listen_id =combinedID(sendPong,vitalsID);//id Doesnt matter, we are matching function, not ID, setting it anyway though
    processBeat.mt=MATCH_FUNCTION;
    if (addParam(&plpc,processBeat)!= SUCCESS){    //plpc declared above setup()
        printf("plpc no room");
        while(1);
    }

    sendHBEvent.call();


    //After being dispatched, events wait through their delay timer before executing, and any attempt to re-dispatch won't do anything
    //Note that we do everything (including repeating events) through a dispatch_once. As a result, we are able to run wait packets 
    //constantly whenever an event isn't being executed, which is important so that vitals responds to critical data ASAP
    while (true){
        checkHeartBeatQueue.dispatch_once();
        waitPackets(NULL,&plpc);
        repeatingEvents.dispatch_once();       
    }
}