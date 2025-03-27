import os
from parseFile import parse_config, globalDefines
from genSensors import createSensors
from genVitals import createVitals
# For pretty-printing the vitals (mostly for debugging)
import json
def pretty_print_vitals(vitals_nodes):
    print("Vitals Nodes:")
    for i, node in enumerate(vitals_nodes):
        print(f"\nNode {i + 1}:")
        for field in node:
            if field["type"] == "list" and isinstance(field["value"], list):
                print(f"  {field['name']} ({field['type']}):")
                for j, sub_field in enumerate(field["value"]):
                    if isinstance(sub_field, list):
                        print(f"    Sub-List {j + 1}:")
                        for k, item in enumerate(sub_field):
                            print(f"      {k + 1}. {json.dumps(item, indent=8)}")
                    else:
                        print(f"    {j + 1}. {json.dumps(sub_field, indent=6)}")
            else:
                print(f"  {field['name']} ({field['type']}): {field['value']}")

if __name__ == "__main__":
    # Get the directory of the current script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Construct the full path to the sensors definition file
    file_path = os.path.join(script_dir, 'sensors.def')
    
    # Parse configuration file
    (vitalsNodes, nodeNames, dataNames, numData, nodeIds,
     startingNodeID, missingIDs, nodeCount, frameCount) = parse_config(file_path)

    print("Starting Node ID:", startingNodeID)
    print("Missing IDs:", missingIDs)
    print("Node Count:", nodeCount)
    print("node Names:", nodeNames)
    print("data Names:", dataNames)
    
    # pretty_print_vitals(vitalsNodes)
    
    # Generate sensor files
    createSensors(vitalsNodes, nodeNames, nodeIds, dataNames, numData, script_dir, globalDefines)
    createVitals(vitalsNodes, nodeNames, nodeIds, dataNames, nodeCount, frameCount, numData, script_dir, globalDefines)



# import os, sys
# import math
# from collections import defaultdict
# import copy
# from copy import deepcopy
# from dataclasses import dataclass

# def ACCESS(fields, name):
#     return next(field for field in fields if field["name"] == name)
# #for expectation field:
# #dontSpecify means a field is computed specially in some way based on other specified fields already present. In the code, don't specify always calls the "special-case" function, which will do something depending on the other fields, and will not trigger the default behavior
# #required means this field must be provided by the user
# #optional means this field will take on the value already in value, unless otherwise specified

# #a copy of each of these array will be made for every node
# dataPoint_fields = [                                                    
#     {"name": "bitLength",       "type": "int8_t",  "expectation": "required", "value": 0, "node": "both"},    # note: bitLength is manually computed, no need to specify
#     {"name": "isCritical",      "type": "int8_t",  "expectation": "dontSpecify", "value": 0,  "node": "vitals"},  #value of isCritical is set based on if minCritical and maxCritical are assigned (hard-coded) 0b1=minCritical, 0b01=maxCritical
#     {"name": "minCritical",     "type": "int32_t", "expectation": "optional",    "value": 0,  "node": "vitals"},
#     {"name": "maxCritical",     "type": "int32_t", "expectation": "optional",    "value": 0,  "node": "vitals"},
#     {"name": "min",             "type": "int32_t", "expectation": "required",    "value": 0,  "node": "both"},
#     {"name": "max",             "type": "int32_t", "expectation": "required",    "value": 0,  "node": "both"},
#     {"name": "minWarning",      "type": "int32_t", "expectation": "required",    "value": 0,  "node": "vitals"},
#     {"name": "maxWarning",      "type": "int32_t", "expectation": "required",    "value": 0,  "node": "vitals"},
#     {"name":"startingValue",    "type": "int32_t", "expectation": "required",    "value": 0,  "node": "vitals"}    #note: startingValue is parsed differently than these other fields, and is used for declaring data array of the CANFrame
# ]

# #code hard generates for data* [10] in CANFrame
# numDataStored = 10

# CANFrame_fields = [  
#     {"name": "nodeID",          "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "vitals"},    #this Value is set based on to which node this frame belongs
#     {"name": "frameID",         "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "vitals"},   #this value is explicitly computed by program,
#     {"name": "frameNumData",         "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "both"},   #this value is computed by the program
#     {"name": "dataInfo",        "type": "list",   "expectation": "dontSpecify", "value": [], "node": "array"},    # List of dataPoint structs. "node"="array" indicates it needs to be parsed element wise
#     {"name": "flags",           "type": "int8_t", "expectation": "optional", "value": 0, "node": "vitals"},               #will fill in a value of 0 if flags is not specified
#     {"name": "dataLocation",    "type": "int8_t", "expectation": "optional", "value": 0, "node": "vitals"},        #no need to ever change this
#     {"name":"consecutiveMisses","type": "int8_t", "expectation": "optional", "value": 0, "node": "vitals"},
#     {"name": "dataTimeout",     "type": "int32_t","expectation": "required", "value": 0, "node": "vitals"}, #Must be specified
#     {"name": "frequency",   "type": "int32_t","expectation": "required", "value": 0, "node": "sensor"}
# ]

# vitalsNode_fields = [   #stuff only for vitals. Each processed manually
#     {"name":"flags", "type": "int8_t", "expectation": "dontSpecify", "value": 0},
#     {"name":"milliSeconds", "type": "int16_t", "expectation": "dontSpecify", "value": 0},
#     {"name":"numFrames", "type": "int8_t", "expectation": "dontSpecify", "value": 0},
#     {"name":"CANFrames", "type": "list", "expectation": "dontSpecify", "value": []}  # List of CANFrame structs
# ]

# @dataclass
# class globalDefine:
#     name: str
#     value: str

# # Global variables
# startingNodeID = None
# missingIDs = []
# globalDefines=[]
# nodeCount = 0
# frameCount = 0

# # Helper function to create a struct-like dictionary
# def create_struct(field_definitions):
#     struct = {}
#     for field in field_definitions:
#         field_name = field["name"]
#         field_type = field["type"]
#         if field_type == "list":
#             struct[field_name] = []  # Initialize lists as empty
#         elif field_type in ["int", "int8_t", "int16_t", "int32_t"]:
#             struct[field_name] = 0  # Default integers to 0
#         elif field_type == "float":
#             struct[field_name] = 0.0  # Default floats to 0.0
#         elif field_type == "string":
#             struct[field_name] = ""  # Default strings to an empty string
#         else:
#             struct[field_name] = None  # Default for unknown types
#     return struct

# def handleCriticals(name, fields):  #if we set a min or maxCritical, we want to also set the corresponding isCritical bit
#     if(name=="minCritical" or (name =="maxCritical")):
#         ACCESS(fields,"isCritical")["value"]=1


# def updateEntries(parsedFields,fields):
#     for name, value in parsedFields.items():
#         found=False
#         for field in fields:
#             if field["name"] == name:   #we found the matching name
#                 found=True
#                 if(field["expectation"]=="dontSpecify"):
#                     print(f"specified something locked: {name}")
#                     while(1): pass
#                 else:   #we want to update this field
#                     field["value"]=value
#                     handleCriticals(name, fields)
#                     if(field["expectation"]=="required"):
#                         field["expectation"]="optional" #indicate this field is no longer needed
#         if not found:
#             print(f"No matching frame found for '{name}'.")
#             while(1): pass 
#             # This 'else' executes if no break occurs (no match found)


#     for field in fields:
#         if (field["expectation"]=="required"):
#             print("did not specify required parameter for CanFrame:")
#             print(field)
#             while(1): pass

# def parse_config(file_path):
#     global startingNodeID, missingIDs, nodeCount, frameCount

#     with open(file_path, 'r') as file:
#         lines = file.readlines()

#     #parallel arrays for each node. Use these for things not in the actual structs, but that are convenient to collect now. We can add as many more as necessary :)
#     vitalsNodes =[] #stores the info needed to generate Static structs for vitals and sensor node
#     nodeNames = []   #stores the names of each node
#     dataNames = []  #stores the names of each piece of data. Will have form: [[name1, name2, ...], [name3, ...] ,    ...]
#     #                                                                             ^each data       ^each frame^  ^for each node
#     numData= [] #stores number of datapoints that each node has
#     node_ids = []
#     for line in lines:
#         line = line.strip()
#         if line.startswith("#") or not line:
#             continue

#         # Process node definition
#         if line.startswith("node:"):
#             node_details = line.split(":")[1].split(",")    #store each assignment as an element ex: "<name>=value", ...
#             node_info = {k.strip(): v.strip() for k, v in (item.split("=") for item in node_details)}   #convert equality to dicitonary
#             node_id = int(node_info["id"])  #not part of main struct
#             node_name = node_info["name"]   #not part of main struct

#             # Initialize vitalsNode
#             vitalsNodes.append(deepcopy(vitalsNode_fields))
#             nodeNames.append(node_name)
            
#             # Update global variables
#             if startingNodeID is None:
#                 startingNodeID = node_id
#             node_ids.append(node_id)
#             numData.append(0)
#             current_node = node_id
#             nodeCount += 1
#         # Process a CANFrame
#         elif line.startswith("CANFrame"):   #add a frame to our node
#             nodeFrames=ACCESS(vitalsNodes[nodeCount-1], "numFrames")
#             nodeFrames["value"]+=1
#             if((numFrames:=nodeFrames["value"]) >4):
#                 print("Warning: more than 4 frames for node "+nodeNames[nodeCount-1])
#                 while(1): pass
                    
#             #do the same exact thing for parsing as for node
#             frame_details = line.split(":")[1].split(",")    #store each assignment as an element ex: "<name>=value", ...
#             frame_info = {k.strip(): v.strip() for k, v in (item.split("=") for item in frame_details)}   #convert equality to dicitonary
            
#             framesArr = ACCESS(vitalsNodes[nodeCount-1],"CANFrames")["value"]
#             framesArr.append(deepcopy(CANFrame_fields))
#             frame=framesArr[numFrames-1]
#             ACCESS(frame,"nodeID")["value"]= node_ids[nodeCount-1]
#             ACCESS(frame,"frameID")["value"]== numFrames-1
#             ACCESS(frame,"frameNumData")["value"]=0
#             #ACCESS(frame,"dataInfo")["value"] = deepcopy(dataPoint_fields)   #make list for future dataPoints
#             #vitalsNodes[nodeCount-1]["dataInfo"] = []   #make list for future dataPoints
#             #iterate through each additional field, updating there values.
#             updateEntries(frame_info,frame)
#         elif "global" in line:  #example: global: vitalsID=0b000010, will make : #define vitalsID 0b000010
#             split=line.strip().split(":")[1].strip().split("=")
#             newGlobal= globalDefine(split[0], split[1])
#             globalDefines.append(newGlobal)
#         elif ":" in line:  # Example: temperature: bitLength=7, min=-10, max=117, ...
#             frame=ACCESS(vitalsNodes[nodeCount-1],"CANFrames")["value"][-1]
#             dataArr = ACCESS(frame,"dataInfo")["value"]
#             dataArr.append(deepcopy(dataPoint_fields))
#             dataPoint=dataArr[-1]
#             print("here")
#             dataNames.append(line.split(":")[0].strip())
#             numData[-1]+=1
#             data_details = line.split(":")[1].split(",")    #store each assignment as an element ex: "<name>=value", ...
#             data_info = {k.strip(): v.strip() for k, v in (item.split("=") for item in data_details)}   #convert equality to dicitonary
#             updateEntries(data_info,dataPoint)
#             ACCESS(frame,"frameNumData")["value"]+=1



#     # Compute missing IDs
#     all_ids = range(min(node_ids), max(node_ids) + 1)
#     missingIDs = [node_id for node_id in all_ids if node_id not in node_ids]

#     return vitalsNodes, nodeNames, dataNames, numData, node_ids

# def createFiles(vitalsNodes, nodeNames, nodeIds,dataNames, numData, script_dir):
#     parent_dir = os.path.dirname(script_dir)     # Get the parent directory of the script
    
#     generated_code_dir = os.path.join(parent_dir, 'generatedSensorCode')    
#     os.makedirs(generated_code_dir, exist_ok=True)      # Create the 'generatedCode' directory, overwrite if it exists
    
#     #create files for which only one exists (sensorHelper.hpp, and vitalsStaticDec)
#     universalPath = os.path.join(generated_code_dir, "Universal")
#     os.makedirs(universalPath, exist_ok=True)

#     file_path = os.path.join(universalPath, 'sensorHelper.hpp')
#     with open(file_path, 'w') as f:
#         f.write(
#             "#ifndef SENSOR_HELP\n"
#             "#define SENSOR_HELP\n\n"
#             "#define STRINGIZE_(a) #a\n"
#             "#define STRINGIZE(a) STRINGIZE_(a)\n"
#             '#include STRINGIZE(../NODE_CONFIG)  //includes node Constants\n\n'
#             '#include "../../pecan/pecan.h"\n'
#             '#include "../../arduinoSched/arduinoSched.hpp"\n'
#             '#include <stdint.h>\n\n'
#             "//universal globals. Used by every sensor\n"
#         )
#         for i in globalDefines:
#             f.write("#define ")
#             f.write(i.name)
#             f.write(" ")
#             f.write(i.value)
#             f.write("\n")
#         f.write("extern struct CANFrame myframes[numFrames];    //defined in sensorStaticDec.cpp in <sensor_name> folder\n\n"
#                 "//shortened versions of vitals structs, containing only stuff the sensors need for sending\n")
        
#         #write the dataPoints struct (for sensors):
#         f.write("struct dataPoint{\n")
#         for field in dataPoint_fields:
#             if(field["node"]=="sensor" or field["node"] == "both"):
#                 f.write("    "+field["type"] + " " + field["name"] + ";\n")
#         #custom fields here
#         f.write("    int32_t data;   //the actual data stored here\n")  
#         f.write("};\n\n")
#         #write the CANFrame struct
#         f.write("struct CANFrame{    //identified by a 2 bit identifier 0-3 in function code\n")
#         for field in CANFrame_fields:
#             if(field["node"]=="sensor" or field["node"] == "both"):
#                 f.write("    "+field["type"] + " " + field["name"] + ";\n")
#         #custom fields here
#         f.write("    int8_t startingDataIndex;  //what is the starting index of data in this frame? (needed for calling appropriate collecter function)\n")
#         f.write("    struct dataPoint *dataInfo;\n")
#         f.write("};\n")
#         f.write("int8_t vitalsInit(PCANListenParamsCollection* plpc, PScheduler* ts);\n#endif")
#         #write the 
#         f.close()
    
#     #write stuff for each sensor
#     nodeIndex=0
#     dataIndex=0
#     for node in vitalsNodes:
#         sub_dir_path = os.path.join(generated_code_dir, nodeNames[nodeIndex])
#         os.makedirs(sub_dir_path, exist_ok=True)  # Create each subdirectory\
        
#         file_path = os.path.join(sub_dir_path, 'myDefines.hpp')
#         with open(file_path, 'w') as f:
#             #includes
#             f.write('#ifndef '+nodeNames[nodeIndex]+'_DATA_H\n#define '+nodeNames[nodeIndex]+'_DATA_H\n')#header guard
#             f.write("//defines constants specific to "+nodeNames[nodeIndex]+"\n//each sensor file gets one of these .h files")   #comment for what this file is
#             f.write('#include "../common/sensorHelper.hpp"\n#include<stdint.h>\n')    #includes
#             f.write("#define myId "+str(nodeIds[nodeIndex])) #nodeId
#             f.write("\n#define numFrames "+str(ACCESS(node,"numFrames")["value"]))
#             f.write("\n#define numData "+str(numData[nodeIndex])+"\n\n")
#             localDataIndex=dataIndex
#             for i in range (numData[nodeIndex]):   #declare a function for collecting each piece of data
#                 f.write("int32_t collect_"+dataNames[dataIndex]+"();\n")
#                 localDataIndex+=1
#             f.write("\n#define dataCollectorsList ")
#             f.write(', '.join("collect_" + name for name in dataNames[dataIndex : dataIndex + numData[nodeIndex]]))
#             f.write("\n\n#endif")
#             f.close()
        
#         #writes the main.cpp for each node
#         file_path = os.path.join(sub_dir_path, str(nodeNames[nodeIndex])+'_main.cpp')      

#         with open(file_path, 'w') as f:
#              #write the includes and scheduler decleration
#             f.write("#include <Arduino.h>\n"
#             "#include \"CAN.h\"\n"
#             "#include \"../../pecan/pecan.h\"                  //used for CAN\n"
#             "#include \"../../arduinoSched/arduinoSched.hpp\"  //used for scheduling\n"
#             "#include \"../common/sensorHelper.hpp\"           //used for compliance with vitals and sending data\n"
#             "#include \"myDefines.hpp\"          //containts #define statements specific to this node like myId.\n"
#             "\n"
#             "\n"
#             "PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };    //use for adding Can listen parameters (may not be necessary)\n"
#             "PScheduler ts;                      //use for scheduling tasks (may not be necessary)\n"
#             "                                    //^ data collection and vitals compliance tasks are already scheduled.\n"
#             "    //if no special behavior, all you need to fill in the the collectData<NAME>() function(s). Have them return an int32_t with the corresponding data\n")
#             localDataIndex=dataIndex
#             for frame in (ACCESS(node,"CANFrames")["value"]):   #for each Can Frame
#                 for dataPoint in (ACCESS(frame, "dataInfo")["value"]):   #for each data point
#                     f.write("""int32_t collect_{0}(){{
#     int32_t {0} = {1};
#     Serial.println("collecting {0}");
#     return {0};
# }}
                            
# """.format(dataNames[localDataIndex], str(ACCESS(dataPoint, "startingValue")["value"])))                    
#                     localDataIndex+=1

#             f.write(
#             "void setup() {\n"
#                 "\tSerial.begin(9600);\n"
#                 "\tSerial.println(\"sensor begin\");\n"
#                 "\tif (!CAN.begin(500E3)) {\n"
#                 "\t\tSerial.println(\"Starting CAN failed!\");\n"
#                 "\t\twhile (1);\n"
#                 "\t}\n"
#                 "\tvitalsInit(&plpc, &ts);\n"
#             "}\n\n"
#             )  # First f.write for setup()
#             f.write(
#             "void loop() {\n"
#                 "\tts.mainloop(&plpc);\n"
#             "}\n"
#             )  # Second f.write for loop()
#             f.close()

#         file_path = os.path.join(sub_dir_path, str(nodeNames[nodeIndex])+'staticDec.cpp')      

#         with open(file_path, 'w') as f:
#             frameNum=0
#             f.write(
#             '#include "myDefines.hpp"\n'
#             '#include "../common/sensorHelper.hpp"\n\n'
#             '//creates CANFRame array from this node. It stores data to be sent, and info for how to send\n\n'
#             )
#             for frame in ACCESS(node, "CANFrames")["value"]:  # For each CAN Frame
#                 num_frames = ACCESS(node, "numFrames")["value"]
#                 f.write(f"struct dataPoint f{frameNum}DataPoints [{num_frames}]={{\n")
#                 frameNum+=1
#                 for dataPoint in ACCESS(frame, "dataInfo")["value"]:  # For each data point
#                     fields = []
#                     for field in dataPoint_fields:
#                         if field["node"] == "both" or field["node"] == "sensor":  # Only include incude fields specified for the sensor
#                             value = ACCESS(dataPoint, field["name"])["value"]
#                             fields.append(f".{field['name']}={value}")

#                     f.write("    {" + ", ".join(fields) + "},\n")

#                 f.write("};\n\n")  # Close the struct array
            
#             #write the CANFrame struct array
#             frame_index = 0  # Initialize the frame index
#             f.write("struct CANFrame myframes[numFrames] = {\n")  
#             #localDataIndex=dataIndex
#             startingDataIndex=0
#             for frame in ACCESS(node, "CANFrames")["value"]:  
#                 f.write("    {")  
#                 first = True  # Used to track if we need a comma before the next field  

#                 for field in CANFrame_fields:  
#                     if field["node"] == "both" or field["node"] == "sensor":
#                         if not first:  
#                             f.write(", ")  
#                         f.write(f".{field['name']} = {ACCESS(frame, field['name'])['value']}")  
#                         first = False  

#                     #fields that are manually included (not specified):
#                     #startingData
#                 f.write(f", .startingDataIndex={startingDataIndex}")   
#                 startingDataIndex+=ACCESS(frame, "frameNumData")['value']
#                     #localDataIndex+=1
#                     #.data, should be initialized


#                 f.write(f", .dataInfo=f{frame_index}DataPoints")  # Attach the corresponding dataPoints struct  
#                 f.write("},\n")  

#                 frame_index += 1  # Increment the frame index

#             f.write("};\n")
#             f.close()
            
#         dataIndex+=numData[nodeIndex]
#         nodeIndex+=1
#     #start new code here ChatGPT!


# import json

# def pretty_print_vitals(vitals_nodes):  #prints all the parsed information in a nice structured way. Special shout out to Chat GPT, it wrote this on the first try :)
#     print("Vitals Nodes:")
#     for i, node in enumerate(vitals_nodes):
#         print(f"\nNode {i + 1}:")
#         for field in node:
#             if field["type"] == "list" and isinstance(field["value"], list):
#                 print(f"  {field['name']} ({field['type']}):")
#                 for j, sub_field in enumerate(field["value"]):
#                     if isinstance(sub_field, list):
#                         print(f"    Sub-List {j + 1}:")
#                         for k, item in enumerate(sub_field):
#                             print(f"      {k + 1}. {json.dumps(item, indent=8)}")
#                     else:
#                         print(f"    {j + 1}. {json.dumps(sub_field, indent=6)}")
#             else:
#                 print(f"  {field['name']} ({field['type']}): {field['value']}")


# if __name__ == "__main__":
#     # Get the directory of the current script
#     script_dir = os.path.dirname(os.path.abspath(__file__))

#     # Construct the full path to the file
#     file_path = os.path.join(script_dir, 'sensors.def')
#     vitals_nodes, nodeNames, dataNames, numData, nodeIds = parse_config(file_path)

#     print("Starting Node ID:", startingNodeID)
#     print("Missing IDs:", missingIDs)
#     print("Node Count:", nodeCount)
#     print("node Names:", nodeNames)
#     print("data Names:", dataNames)
#     pretty_print_vitals(vitals_nodes)
#     createFiles(vitals_nodes,nodeNames,nodeIds, dataNames,numData, script_dir)
#     #print("Vitals Nodes:", vitals_nodes)
        
