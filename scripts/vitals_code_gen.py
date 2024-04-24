import os, sys
import math
'''
struct nodeInfo{
    int numData;
    long ranges[16];            //ranges for each data low-high
    int numBits[8];             //number of bits for each data   
    long warningRanges[16];     //"yellow" ranges for each data
    long criticalRanges[16];    //"red" ranges for each data if applicable
;
{    
    {4,{19,82,-32768,32767,-100,-1,9,18}, {6,16,7,4},{30, 60,100,300,-80,-20, 11,15},{22,75,0,0,0,0,10,17}},
    {4,{14,89,-32761,35767,-101,-1,7,21}, {7,17,7,4},{30, 60,100,300,-80,-20, 11,15},{22,75,0,0,0,0,10,17}},
    {1,{-32780,32780},{17},{0, 100},{-5,100}}
}; 
what is needed to gen everything:
vitals info, ranges, data num, etc. ^ have each data within sensor grouped by how often they are sent,
and have a number in front of each node for if that node has other identical nodes.
so no make it:
    //have array of nodeInfos like before
    struct nodeInfo{
    int numSensors  **new!  //will trigger the makeing of multiple simular sensor files, j w/ slightly dif names at the same time
    int numData;
    int interval[8]; **new! (in ascending order), place send at end of smallest group
    long ranges[16];            //ranges for each data low-high
    int numBits[8];             //number of bits for each data   
    long warningRanges[16];     //"yellow" ranges for each data
    long criticalRanges[16];    //"red" ranges for each data if applicable
};


{   //times
{4,{100,100,100,220},{19,82,-32768,32767,-100,-1,9,18}, {6,16,7,4},{30, 60,100,300,-80,-20, 11,15},{22,75,0,0,0,0,10,17}},
    {4,100{14,89,-32761,35767,-101,-1,7,21}, {7,17,7,4},{30, 60,100,300,-80,-20, 11,15},{22,75,0,0,0,0,10,17}},
    {1,{-32780,32780},{17},{0, 100},{-5,100}}
};
'''

from dataclasses import dataclass
from dataclasses import field
@dataclass
class Node:
    shortenedName: str
    name: str
    numData: int
    dataNames:list[str]=field(default_factory=list)
    interval: list[int]=field(default_factory=list)
    ranges: list[int]=field(default_factory=list)
    numBits: list[int]=field(default_factory=list)
    warning: list[int]=field(default_factory=list)
    critical: list[int]=field(default_factory=list)

nodeList=[]
startingSensorId=6#will be 6 if not otherwise initialized
'''
//name1 globals
const int name1Id=6;
int8_t name1dataArray[8];
int8_t name1VitalsFlags=0;  //0b1=startup, 0b0111 can be for whatver other things we end up wanting idk'''
def createSensorFiles():#uses nodeList
    sensorId=startingSensorId
    for node in nodeList:
        with open("src/generatedCode/"+node.name+".cpp","w") as f:
            #includes
            f.write('#include "../common/pecan.hpp"\n#include "Arduino.h"\n#include "CAN.h"\n#include "../arch/arduino.hpp"\n')
            #universal variabls global varaibles that every sensor has
            f.write('\n//universal globals\nconst int vitalsID=0b0000010;\nconst int sendPing=0b0011;\nconst int sendPong=0b0100;\nconst int transmitData=0b0111;\nPCANListenParamsCollection plpc;\nPScheduler ts;\n')
            print("shortened name: "+node.shortenedName)
            f.write('//{0} globals\nconst int {0}Id={1};\nint8_t {0}dataArray[8];\nint8_t {0}VitalsFlags=0;'.format(node.shortenedName,sensorId))
            f.close()

            

        sensorId+=1



    print("comment")
    '''
    with open(output_file, "w") as f:
        f.write('#ifndef PCAN_UART\n#define PCAN_UART\n\n')
        f.write("//" + f"-"*30 + "//\n")

        if bool(config["PC"]):f.write("#include <cstdint>\n#include <stdio.h>\n")
        else: f.write('#include "mbed.h"\n')
        
        f.write("//" + f"-"*30 + "//\n")
        f.write(f'#define SENTINEL {config["SEN"]} \n')
        f.write(f'#define HEADER_SIZE_BITS {config["Header"]}\n')
        f.write(f'#define HEADER_SIZE_BYTES {config["Header"]//8}\n')
        f.write(f'#define MAX_DATA_SIZE_BITS {bit_ct}\n')
        f.write(f'#define MAX_DATA_SIZE_BYTES {bit_ct//8 + int(bit_ct%8!=0)}\n')
        f.write(f'#define CRC_SIZE_BITS {config["CRC"]}\n')
        f.write(f'#define CRC_SIZE_BYTES {config["CRC"]//8}\n\n')
        f.write("//" + f"-"*30 + "//\n")
        f.write(f"#define SENSOR_CT {len(sensors.keys())}\n")
        f.write(f'#define DATA_STRUCT_SIZE {len(sensors.keys())*5}\n\n')
        f.write("//" + f"-"*30 + "//\n\n")
        f.write("// Polynomials for CRC stuff\n")

        polys = config["POLY"].replace("{", "").replace("}", "").split(";")
        f.write("static uint16_t polys[] = {")
        for i, p in enumerate(polys): f.write(p +("," if i != len(polys)-1 else ""))
        f.write("};\n\n")
        if bool(config["PC"]):
            f.write('#pragma pack(1)\nstruct __attribute__((__packed__)) Sensor_Data {\n')
        else:
            f.write('MBED_PACKED(struct) Sensor_Data {\n')
        # Null = Sentinel Value
        _ = [f.write(
            (f"    //{sdetails[1]}\n" if sdetails[1] else "\n") +
            f"    uint8_t {sensor}_size = {sdetails[0]};\n" +
            f"    int32_t {sensor}_data = SENTINEL;\n"
        ) for sensor, sdetails in sensors.items()]

        # End Struct
        f.write("};\n")
        f.write('#endif')
    '''
    print("Generated C++ file")



def addNode(word):
    dataChunks=word.split(":")
    shortenedName=(dataChunks[0])
    print(shortenedName)
    name=(dataChunks[1])
    numData=len(dataChunks)-2
    nodeList.append(newNode:=Node(shortenedName,name,numData))
    print("name: "+name)
    i=2
    while(i<len(dataChunks)):
        data=dataChunks[i].split(".")
        dataIndex=0
        newNode.dataNames.append(data[dataIndex])
        dataIndex+=1
        newNode.interval.append(int(data[dataIndex]))
        dataIndex+=1
        ranges=data[dataIndex].split(",")
        rangesInt=[ int(x) for x in ranges ]
        newNode.ranges.append(rangesInt)
        newNode.numBits.append((int(math.ceil(math.log2(rangesInt[1]-rangesInt[0]+1)))))
        dataIndex+=1
        if(len(data)>3):#includes a warning
            warnings=data[dataIndex].split(",")
            newNode.warning.append([ int(x) for x in warnings ])
        else:
            newNode.warning.append(["",""])#make sure check if empty
        dataIndex+=1
        if(len(data)>4):#includes a critical
            criticals=data[dataIndex].split(",")
            newNode.critical.append([ int(x) for x in criticals ])
        else:
            newNode.critical.append(["",""])#make sure check if empty
        i+=1

        #sort by lowest interval
        newNode.ranges=[x for _, x in sorted(zip(newNode.interval, newNode.ranges),key=lambda pair: pair[0])]
        newNode.dataNames=[x for _, x in sorted(zip(newNode.interval, newNode.dataNames),key=lambda pair: pair[0])]
        newNode.numBits=[x for _, x in sorted(zip(newNode.interval, newNode.numBits),key=lambda pair: pair[0])]
        newNode.warning=[x for _, x in sorted(zip(newNode.interval, newNode.warning),key=lambda pair: pair[0])]
        newNode.critical=[x for _, x in sorted(zip(newNode.interval, newNode.critical),key=lambda pair: pair[0])]
        newNode.interval.sort()

def parse_def_file(file_name):
    print("in parser")
    print(file_name)
    print("empty?")
    with open(file_name, "r") as f:
        temp_char = ""
        word = ""
        while (temp_char := f.read(1)) != "":
            print(temp_char)
            if temp_char == " ":continue
            elif temp_char == "\n":
                if word != "" and word!="\n":
                    addNode(word)
                '''
                sensor_name, other_sensor_details = word.strip().split("-")
                other_sensor_details = other_sensor_details.split(":")
                sensors[sensor_name] = (int(other_sensor_details[0]), other_sensor_details[1] if len(other_sensor_details) > 1 else None)'''
                word = ""
            elif temp_char == "#":                
                while f.read(1) != "\n":pass
                word=""
            elif temp_char == "=":
                line=f.readline().strip().split(",")
                if(line[0]=="startingSensorId"):
                    startingSensorId=int(line[1])
                word=""
            elif temp_char == "/" and f.read(1) == "*":
                while True:
                    if f.read(1) == "*" and f.read(1) == "/":
                        break
                word=""
            else:
                word += temp_char
    if word != "":
        addNode(word)
        '''
        sensor_name, other_sensor_details = word.strip().split("-")
        other_sensor_details = other_sensor_details.split(":")
        sensors[sensor_name] = (int(other_sensor_details[0]), other_sensor_details[1] if len(other_sensor_details) > 1 else None)'''





if __name__ == "__main__":
    print(os.getcwd())
    print("can print things")
    parse_def_file("scripts/sensors.def")
    #parse_def_file(x if (x:= input("FileName: ")).strip() != "" else "sensors.def") #wat the frick even why
    print("Parsed the following data:")
    for n in nodeList:
        print(n)
    createSensorFiles()
    
