import os
import json
from copy import deepcopy
from dataclasses import dataclass
import math
#values must fit in int32_t
INT_MIN = -2147483648
INT_MAX = 2147483647

# Access a field by name
def ACCESS(fields, name):
    return next(field for field in fields if field["name"] == name)

# For expectation field:
# dontSpecify means a field is computed specially in some way based on other specified fields already present.
# required means this field must be provided by the user.
# optional means this field will take on the value already in value, unless otherwise specified.

# A copy of each of these will be made for every dataPoint
dataPoint_fields = [                                                    
    {"name": "bitLength",       "type": "int8_t",  "expectation": "required", "value": 0, "node": "both", "isSet": False},    # bitLength is manually computed, no need to specify
    {"name": "minCritical",     "type": "int32_t", "expectation": "optional",    "value": 0,  "node": "vitals", "isSet": False},
    {"name": "maxCritical",     "type": "int32_t", "expectation": "optional",    "value": 0,  "node": "vitals", "isSet": False},
    {"name": "min",             "type": "int32_t", "expectation": "required",    "value": 0,  "node": "both", "isSet": False},
    {"name": "max",             "type": "int32_t", "expectation": "required",    "value": 0,  "node": "both", "isSet": False},
    {"name": "minWarning",      "type": "int32_t", "expectation": "optional",    "value": 0,  "node": "vitals", "isSet": False},
    {"name": "maxWarning",      "type": "int32_t", "expectation": "optional",    "value": 0,  "node": "vitals", "isSet": False},
    {"name": "startingValue",   "type": "int32_t", "expectation": "required",    "value": 0,  "node": "vitals", "isSet": False}
]

# A copy of each of these will be made for every CANFrame
CANFrame_fields = [  
    {"name": "nodeID",          "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "vitals", "isSet": False},    # set based on which node this frame belongs to
    {"name": "frameID",         "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "vitals", "isSet": False},    # explicitly computed by program
    {"name": "numData",    "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "both", "isSet": False},     # computed by the program
    {"name": "dataInfo",        "type": "list",   "expectation": "dontSpecify", "value": [], "node": "array", "isSet": False},      # List of dataPoint structs; element‐wise parsed
    {"name": "isCritical",      "type": "int8_t",  "expectation": "optional", "value": 0,  "node": "vitals", "isSet": False},  # should we raise critical error if this frame is not being sent? 
    {"name": "flags",           "type": "int8_t", "expectation": "optional",    "value": 0, "node": "vitals", "isSet": False},      # if not specified, set to 0
    {"name": "dataLocation",    "type": "int8_t", "expectation": "optional",    "value": 0, "node": "vitals", "isSet": False},      # never needs to be changed
    {"name": "consecutiveMisses","type": "int8_t", "expectation": "optional",   "value": 0, "node": "vitals", "isSet": False},
    {"name": "dataTimeout",     "type": "int32_t","expectation": "required",    "value": 0, "node": "vitals", "isSet": False},     # Must be specified
    {"name": "frequency",       "type": "int32_t","expectation": "required",    "value": 0, "node": "sensor", "isSet": False}
]
#A copy of each of these will be made for every sensor node
vitalsNode_fields = [   # fields only for vitals; each processed manually
    {"name": "flags",       "type": "int8_t", "expectation": "dontSpecify", "value": 0, "isSet": False},
    {"name": "milliSeconds","type": "int16_t", "expectation": "dontSpecify", "value": 0, "isSet": False},
    {"name": "numFrames",   "type": "int8_t", "expectation": "dontSpecify", "value": 0, "isSet": False},
    {"name": "CANFrames",   "type": "list",   "expectation": "dontSpecify", "value": [], "isSet": False}  # List of CANFrame structs
]

@dataclass
class globalDefine:
    name: str
    value: str

# Global list to store global defines (populated during parsing)
globalDefines = []

#this function contains several manual checks to confirm that the values entered for a datapoint make sense
def validate_datapoint(dp, dataName, node_id):
    # Retrieve values from the datapoint.
    bit_length    = int(ACCESS(dp, "bitLength")["value"])
    min_value     = int(ACCESS(dp, "min")["value"])
    max_value     = int(ACCESS(dp, "max")["value"])
    minWarning   = int(ACCESS(dp, "minWarning")["value"])
    minWarningSet= bool(ACCESS(dp, "minWarning")["isSet"])
    maxWarning   = int(ACCESS(dp, "maxWarning")["value"])
    maxWarningSet= bool(ACCESS(dp, "maxWarning")["isSet"])
    minCritical   = int(ACCESS(dp, "minCritical")["value"])
    minCriticalSet= bool(ACCESS(dp, "minCritical")["isSet"])
    maxCritical   = int(ACCESS(dp, "maxCritical")["value"])
    maxCriticalSet= bool(ACCESS(dp, "maxCritical")["isSet"])
    starting_val  = int(ACCESS(dp, "startingValue")["value"])   
    # Check overall range is valid.
    if not (min_value < max_value):
        print(f"Error: For {dataName} (node {node_id}): Overall range invalid: min ({min_value}) must be less than max ({max_value}).")
        while(1): pass
    # Check that bit_length is appropriate,Compute the number of bits required for the range.
    req = math.log2(int(max_value) - int(min_value) + 1)
    required_bits = math.ceil(req)
    if bit_length != required_bits:
        print(f"Warning: For {dataName} (node {node_id}): Bit length {bit_length} does not match the required {required_bits} for range size ({max_value} - {min_value}).")
        while(1): pass
    # Check that ranges are within in max and min
    if (min_value<INT_MIN):
        print(f"Warning: For {dataName} (node {node_id}): minValue less than INT_MIN.")
        while(1): pass
    if (max_value>INT_MAX):
        print(f"Warning: For {dataName} (node {node_id}): maxValue more than INT_MAX.")
        while(1): pass

    #set ranges so that they will be ignored if they were not set.
    #note: with current system, warning/critical ranges will be triggered on exclusive comparisons.
    if not minWarningSet:
         minWarning=(int)(ACCESS(dp, "min")["value"])
         ACCESS(dp, "minWarning")["value"]= minWarning
    if not maxWarningSet:
         maxWarning=(int)(ACCESS(dp, "max")["value"])
         ACCESS(dp, "maxWarning")["value"]= maxWarning
    if not minCriticalSet:
         minCritical=(int)(ACCESS(dp, "min")["value"])
         ACCESS(dp, "minCritical")["value"]= minCritical
    if not maxCriticalSet:
         maxCritical=(int)(ACCESS(dp, "max")["value"])
         ACCESS(dp, "maxCritical")["value"]= maxCritical

    # Check range ordering.
    if(minWarningSet and minWarning<min_value or (maxWarningSet and maxWarning>max_value)):
        print(f"Warning: For {dataName} (node {node_id}): warningRange outside of given range")
        while(1): pass
    if(minCriticalSet and minCritical<min_value or (maxCriticalSet and maxCritical>max_value)):
        print(f"Warning: For {dataName} (node {node_id}): critical outside of given range")
        while(1): pass
    if(minWarningSet and minCriticalSet and minWarning<minCritical or (maxWarningSet and maxCriticalSet and maxWarning>maxCritical)):
        print(f"Warning: For {dataName} (node {node_id}): warningRange outside of critical range")
        while(1): pass


    #check startingVal
    if(starting_val<minWarning or starting_val>maxWarning):
        print(f"Warning: For {dataName} (node {node_id}): startingVal outside of acceptable range")
        while(1): pass

#update the data in a given set of fields based on what was read from a line
def updateEntries(parsedFields, fields):
    for name, value in parsedFields.items():
        found = False
        for field in fields:
            if field["name"] == name:
                found = True
                if field["expectation"] == "dontSpecify":
                    print(f"specified something locked: {name}")
                    while(1): pass
                else:
                    field["value"] = value
                    ACCESS(fields, name)["isSet"] = True
        if not found:
            print(f"No matching frame found for '{name}'.")
            while(1): pass

    for field in fields:
        if field["expectation"] == "required" and not field["isSet"]:
            print("did not specify required parameter for CanFrame:")
            print(field)
            while(1): pass

# --- parse_config function moved here ---
def parse_config(file_path):
    # Variables storing info as we go about our parsing
    startingNodeID = None
    nodeCount = 0
    frameCount = 0

    # Parallel arrays for each node.
    vitalsNodes = []  # stores info for vitals and sensor nodes
    nodeNames = []    # stores the names of each node
    boardTypes = []
    dataNames = []    # stores the names of each piece of data (organized per node)
    numData = []      # stores number of datapoints each node has
    node_ids = []
    missingIDs = []     #not parallel. 

    with open(file_path, 'r') as file:
        lines = file.readlines()

    for line in lines:
        line = line.strip()
        if line.startswith("#") or not line:
            continue

        # Process node definition
        if line.startswith("node:"):
            node_details = line.split(":")[1].split(",")
            node_info = {k.strip(): v.strip() for k, v in (item.split("=") for item in node_details)}
            node_id = int(node_info["id"])
            node_name = node_info["name"]
            board_type= node_info["board"]
            # Initialize vitalsNode
            vitalsNodes.append(deepcopy(vitalsNode_fields)) #create a copy of array of structs at top of file, and modify values as we parse
            nodeNames.append(node_name)
            boardTypes.append(board_type)
            if startingNodeID is None:
                startingNodeID = node_id
            node_ids.append(node_id)
            numData.append(0)
            nodeCount += 1

        # Process a CANFrame
        elif line.startswith("CANFrame"):
            frameCount+=1
            nodeFrames = ACCESS(vitalsNodes[nodeCount - 1], "numFrames")
            nodeFrames["value"] += 1
            numFrames=nodeFrames["value"]
            # if (numFrames := nodeFrames["value"]) > 4:    #
            #     print("Warning: more than 4 frames for node " + nodeNames[nodeCount - 1])
            #     while(1): pass

            frame_details = line.split(":")[1].split(",")
            frame_info = {k.strip(): v.strip() for k, v in (item.split("=") for item in frame_details)}

            framesArr = ACCESS(vitalsNodes[nodeCount - 1], "CANFrames")["value"]
            framesArr.append(deepcopy(CANFrame_fields))
            frame = framesArr[numFrames - 1]
            ACCESS(frame, "nodeID")["value"] = node_ids[nodeCount - 1]
            ACCESS(frame, "frameID")["value"] = frameCount - 1
            ACCESS(frame, "numData")["value"] = 0

            updateEntries(frame_info, frame)

        elif "global" in line:
            # example: global: vitalsID=0b000010, will make: #define vitalsID 2
            split = line.strip().split(":")[1].strip().split("=")
            newGlobal = globalDefine(split[0], split[1])
            globalDefines.append(newGlobal)

        elif ":" in line:
            # example: temperature: bitLength=7, min=-10, max=117, ...
            frame = ACCESS(vitalsNodes[nodeCount - 1], "CANFrames")["value"][-1]
            dataArr = ACCESS(frame, "dataInfo")["value"]
            dataArr.append(deepcopy(dataPoint_fields))
            dataPoint = dataArr[-1]
            dataNames.append(line.split(":")[0].strip())
            numData[-1] += 1

            data_details = line.split(":")[1].split(",")
            data_info = {k.strip(): v.strip() for k, v in (item.split("=") for item in data_details)}
            updateEntries(data_info, dataPoint)
            validate_datapoint(dataPoint, dataNames[-1], node_ids[-1])
            ACCESS(frame, "numData")["value"] += 1


    # Compute missing IDs
    all_ids = range(min(node_ids), max(node_ids) + 1)
    missingIDs = [node_id for node_id in all_ids if node_id not in node_ids]

    return vitalsNodes, nodeNames, boardTypes, dataNames, numData, node_ids, startingNodeID, missingIDs, nodeCount, frameCount
