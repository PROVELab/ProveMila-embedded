import os
from parseFile import dataPoint_fields, CANFrame_fields, ACCESS

def createSensors(vitalsNodes, nodeNames, boardTypes, nodeIds, dataNames, numData, generated_code_dir):
    
    # create files for which only one exists (sensorHelper.hpp, and vitalsStaticDec)
    universalPath = os.path.join(generated_code_dir, "Universal")
    os.makedirs(universalPath, exist_ok=True)

    file_path = os.path.join(universalPath, 'sensorHelper.hpp')
    with open(file_path, 'w') as f:
        f.write(
            "#ifndef SENSOR_HELP\n"
            "#define SENSOR_HELP\n\n"
            "#ifdef __cplusplus\nextern \"C\" { //Need C linkage since ESP uses C \"C\"\n#endif\n"
            "#include \"../../programConstants.h\"\n"
            "#define STRINGIZE_(a) #a\n"
            "#define STRINGIZE(a) STRINGIZE_(a)\n"
            '#include STRINGIZE(../NODE_CONFIG)  //includes node Constants\n\n'
            '#include "../../pecan/pecan.h"\n'
            '#include <stdint.h>\n\n'
            "//universal globals. Used by every sensor\n"
        )

        # write the dataPoints struct (for sensors):
        f.write("typedef struct{\n")
        for field in dataPoint_fields:
            if "sensor" in field["node"]:
                f.write("    " + field["type"] + " " + field["name"] + ";\n")

        f.write("} dataPoint;\n\n")
        # write the CANFrame struct
        f.write("typedef struct{    //identified by a 2 bit identifier 0-3 in function code\n")
        for field in CANFrame_fields:
            if "sensor" in field["node"]:
                f.write("    " + field["type"] + " " + field["name"] + ";\n")     
        # custom fields here
        f.write("    int8_t startingDataIndex;  //starting index of data in this frame. used by collector function\n")
        f.write("    dataPoint *dataInfo;\n")
        f.write("} CANFrame;\n")
        #

        f.write("extern CANFrame myframes[numFrames];    //defined in sensorStaticDec.cpp in <sensor_name> folder\n\n"
        "//shortened versions of vitals structs, containing only stuff the sensors need for sending\n")

        f.write("//For ts, pass PScheduler* for arduino, else pass NULL\n")
        f.write("int8_t sensorInit(PCANListenParamsCollection* plpc, void* ts);\n")
        f.write("#ifdef __cplusplus\n}  // End extern \"C\"\n#endif\n#endif")
    
    # write stuff for each sensor
    nodeIndex = 0
    dataIndex = 0
    for node in vitalsNodes:
        sub_dir_path = os.path.join(generated_code_dir, nodeNames[nodeIndex])
        os.makedirs(sub_dir_path, exist_ok=True)  # Create each subdirectory
        
        file_path = os.path.join(sub_dir_path, 'myDefines.hpp')
        with open(file_path, 'w') as f:
            # includes
            f.write('#ifndef ' + nodeNames[nodeIndex] + '_DATA_H\n#define ' + nodeNames[nodeIndex] + '_DATA_H\n')
            f.write("//defines constants specific to " + nodeNames[nodeIndex])
            f.write('#include "../common/sensorHelper.hpp"\n#include<stdint.h>\n')
            f.write("#define myId " + str(nodeIds[nodeIndex]))
            f.write("\n#define numFrames " + str(ACCESS(node, "numFrames")["value"]))
            f.write("\n#define node_numData " + str(numData[nodeIndex]) + "\n\n")
            localDataIndex = dataIndex
            for i in range(numData[nodeIndex]):
                f.write("int32_t collect_" + dataNames[dataIndex] + "();\n")
                localDataIndex += 1
                dataIndex += 1  # increment dataIndex for each function declared
            f.write("\n#define dataCollectorsList ")
            f.write(', '.join("collect_" + name\
                    for name in dataNames[localDataIndex - numData[nodeIndex]: localDataIndex]))
            f.write("\n\n#endif")
        file_path
        if(boardTypes[nodeIndex]=="arduino"):
            file_path = os.path.join(sub_dir_path, 'main.cpp')
        elif(boardTypes[nodeIndex]=="esp"):
            file_path = os.path.join(sub_dir_path, 'main.c')

        else:
            print(f"Warning: For {nodeNames[nodeIndex]} (node {nodeIds[nodeIndex]})\
                  : Please Specify an appropraite board (esp, arduino, ...?)")
            while(1): pass
        with open(file_path, 'w') as f:
            if(boardTypes[nodeIndex]=="arduino"):    #create main.cpp for arduino sensors
                f.write("#include <Arduino.h>\n"
                    "#include <avr/wdt.h>\n"
                    "#include \"CAN.h\"\n"
                    "#include \"../../pecan/pecan.h\"                  //used for CAN\n"
                    "#include \"../../arduinoSched/arduinoSched.hpp\"  //used for scheduling\n"
                    "#include \"../common/sensorHelper.hpp\"      //used for compliance with vitals and sending data\n"
                    "#include \"myDefines.hpp\"    //contains #define statements specific to this node like myId.\n\n")
                f.write("PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0};"
                    "\nPScheduler ts;\n"
                    "//For Standard behavior, fill in the collectData<NAME>() function(s).\n"
                    "//In the function, return an int32_t with the corresponding data\n")
                localDataIndex = dataIndex - numData[nodeIndex]  # reset localDataIndex for this node
                for frame in ACCESS(node, "CANFrames")["value"]:
                    for dataPoint in ACCESS(frame, "dataInfo")["value"]:
                        f.write("int32_t collect_{0}(){{\n    int32_t {0} = {1};\n\
                                Serial.println(\"collecting {0}\");\n    return {0};\n}}\n\n".format(
                            dataNames[localDataIndex], str(ACCESS(dataPoint, "startingValue")["value"])))
                        localDataIndex += 1
                f.write("void setup() {\n"
                    "\tSerial.begin(9600);\n"
	                "\tSerial.println(\"sensor begin\");\n"
                    "\twdt_enable(WDTO_2S); // enable watchdog with 2s timeout. reset in ts.mainloop\n"
                    "\tpecanInit config={.nodeId= myId, .pin1= defaultPin, .pin2= defaultPin};\n"
                    "\tpecan_CanInit(config);\n"
                    "\tsensorInit(&plpc, &ts);\n"
                    "}\n\n")
                f.write("void loop() {\n"
                    "\twdt_reset();\n"
                    "\twhile( waitPackets(&plpc) != NOT_RECEIVED);	//handle CAN messages\n"
                    "\tts.execute();	//Execute scheduled tasks\n"
                    "}\n")
                f.close()
            elif(boardTypes[nodeIndex]=="esp"):
                f.write("#include \"freertos/FreeRTOS.h\"\n"  
                    "#include \"freertos/task.h\"\n"
                    "#include \"esp_system.h\"\n"
                    "#include \"driver/gpio.h\"\n"
                    "#include \"driver/twai.h\"\n"
                    "#include \"freertos/semphr.h\"\n"
                    "#include <string.h>\n"
                    "#include \"esp_timer.h\"\n\n"
                    "#include \"../../pecan/pecan.h\"             //used for CAN\n"
                    "#include \"../common/sensorHelper.hpp\"      //used for compliance with vitals and sending data\n"
                    "#include \"myDefines.hpp\"       //contains #define statements specific to this node like myId.\n"
                    "#include \"../../espBase/debug_esp.h\"\n"
                    "//add declerations to allocate space for additional tasks here as needed\n"
                    "StaticTask_t recieveMSG_Buffer;\n"
                    "StackType_t recieveMSG_Stack[STACK_SIZE]; //buffer that the task will use as its stack\n\n"
                    "//For Standard behavior, fill in the collectData<NAME>() function(s).\n"
                    "//In the function, return an int32_t with the corresponding data\n")
                localDataIndex = dataIndex - numData[nodeIndex]  # reset localDataIndex for this node
                for frame in ACCESS(node, "CANFrames")["value"]:
                    for dataPoint in ACCESS(frame, "dataInfo")["value"]:
                        f.write("int32_t collect_{0}(){{\n    int32_t {0} = {1};\n"
                                "\tmutexPrint(\"collecting {0}\\n\");\n    return {0};\n}}\n\n".format(
                            dataNames[localDataIndex], str(ACCESS(dataPoint, "startingValue")["value"])))
                        localDataIndex += 1
                f.write("void recieveMSG(){  //task handles recieving Messages\n"
                    "\tPCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };\n"
                    "\tsensorInit(&plpc,NULL); //vitals Compliance\n\n"
                    "\t//declare CanListenparams here, each param has 3 entries:\n"
                    "\t//When recv msg with id = 'listen_id' according to matchtype (or 'mt'), 'handler' is called.\n"
                    "\t\n//task calls the appropriate ListenParams function when a CAN message is recieved\n"
                    "\tfor(;;){\n"
                    "\t\twhile(waitPackets(&plpc) != NOT_RECEIVED);\n"
                    "\t\ttaskYIELD();\n"
                    "\t}\n"
                    "}\n\n"
                    "void app_main(void){\n" 
                    "\tbase_ESP_init();\n"
                    "\tpecanInit config={.nodeId= myId, .pin1= defaultPin, .pin2= defaultPin};\n"  
                    "\tpecan_CanInit(config);   //initialize CAN\n\n"
                    "\t//Declare tasks here as needed\n"
                    "\tTaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore(  //recieves CAN Messages \n"
                    "\t\trecieveMSG,       /* Function that implements the task. */\n"
                    "\t\t\"msgRecieve\",          /* Text name for the task. */\n"
                    "\t\tSTACK_SIZE,      /* Number of indexes in the xStack array. */\n"
                    "\t\t( void * ) 1,    /* Task Parameter. Must remain in scope or be constant!*/ \n"
                    "\t\ttskIDLE_PRIORITY,/* Priority at which the task is created. */\n"
                    "\t\trecieveMSG_Stack,          /* Array to use as the task's stack. */\n"
                    "\t\t&recieveMSG_Buffer,   /* Variable to hold the task's data structure. */\n"
                    "\t\ttskNO_AFFINITY);  //assign to either core\n"
                    "}\n")
                f.close()
        
        file_path = os.path.join(sub_dir_path, 'staticDec.cpp')
        # file_path = os.path.join(sub_dir_path, nodeNames[nodeIndex] + 'staticDec.cpp')
        with open(file_path, 'w') as f:
            frameNum = 0
            f.write('#include "myDefines.hpp"\n#include "../common/sensorHelper.hpp"\n\n'
                    '//creates CANFrame array from this node. It stores data to be sent, and info for how to send\n\n')
            for frame in ACCESS(node, "CANFrames")["value"]:
                num_Data = ACCESS(frame, "numData")["value"]
                f.write(f"dataPoint f{frameNum}DataPoints [{num_Data}]={{\n")
                frameNum += 1
                for dataPoint in ACCESS(frame, "dataInfo")["value"]:
                    fields = []
                    for field in dataPoint_fields:
                        if "sensor" in field["node"]:
                            value = ACCESS(dataPoint, field["name"])["value"]
                            fields.append(f".{field['name']}={value}")
                    f.write("    {" + ", ".join(fields) + "},\n")
                f.write("};\n\n")
            frame_index = 0
            startingDataIndex = 0
            f.write("CANFrame myframes[numFrames] = {\n")
            for frame in ACCESS(node, "CANFrames")["value"]:
                f.write("    {")
                first = True
                for field in CANFrame_fields:
                    if "sensor" in field["node"]:
                        if not first:
                            f.write(", ")
                        f.write(f".{field['name']} = {ACCESS(frame, field['name'])['value']}")
                        first = False
                f.write(f", .startingDataIndex={startingDataIndex}")
                startingDataIndex += ACCESS(frame, "numData")["value"]
                f.write(f", .dataInfo=f{frame_index}DataPoints")
                f.write("},\n")
                frame_index += 1
            f.write("};\n")
            f.close()
        nodeIndex += 1

    #Generate platformio.ini environments. Only contains environments for sensor nodes. 
    #Code to be pasted into actual platformio.ini file as an add-on
    file_path = os.path.join(generated_code_dir,'Generatedplatformio.ini')
    with open(file_path, 'w') as f:
        nodeIndex=0
        f.write("\n")
        for node in vitalsNodes:
            if(boardTypes[nodeIndex]=="arduino"):
                f.write(f"[env:{nodeNames[nodeIndex]}]\n")
                f.write("extends=arduinoSensorBase\n")
                f.write(f"build_src_filter = ${{arduinoSensorBase.build_src_filter}}"
                        f"+<sensors/{nodeNames[nodeIndex]}>\n")
                f.write(f"build_flags = -DNODE_CONFIG={nodeNames[nodeIndex]}"
                        "/myDefines.hpp -DSENSOR_ARDUINO_BUILD=ON\n\n")

            elif(boardTypes[nodeIndex]=="esp"):
                f.write(f"[env:{nodeNames[nodeIndex]}]\n")
                f.write("extends=espSensorBase\n")
                f.write(f"board_build.cmake_extra_args = ${{espSensorBase.board_build.cmake_extra_args}}"
                        f" -DSENS_DIR={nodeNames[nodeIndex]}\n")
                f.write(f"build_flags = ${{espSensorBase.build_flags}}"
                         f" -DNODE_CONFIG={nodeNames[nodeIndex]}/myDefines.hpp\n\n")
            nodeIndex+=1
        f.close()


# Note: The ACCESS helper is also defined here to allow local field lookup.
def ACCESS(fields, name):
    return next(field for field in fields if field["name"] == name)
