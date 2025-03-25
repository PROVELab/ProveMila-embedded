import os
import json
from copy import deepcopy
from dataclasses import dataclass

# Helper function to access a field by name
def ACCESS(fields, name):
    return next(field for field in fields if field["name"] == name)

# For expectation field:
# dontSpecify means a field is computed specially in some way based on other specified fields already present.
# required means this field must be provided by the user.
# optional means this field will take on the value already in value, unless otherwise specified.

# A copy of each of these arrays will be made for every node
dataPoint_fields = [                                                    
    {"name": "bitLength",       "type": "int8_t",  "expectation": "required", "value": 0, "node": "both"},    # bitLength is manually computed, no need to specify
    {"name": "isCritical",      "type": "int8_t",  "expectation": "dontSpecify", "value": 0,  "node": "vitals"},  # isCritical is set based on if minCritical and maxCritical are assigned
    {"name": "minCritical",     "type": "int32_t", "expectation": "optional",    "value": 0,  "node": "vitals"},
    {"name": "maxCritical",     "type": "int32_t", "expectation": "optional",    "value": 0,  "node": "vitals"},
    {"name": "min",             "type": "int32_t", "expectation": "required",    "value": 0,  "node": "both"},
    {"name": "max",             "type": "int32_t", "expectation": "required",    "value": 0,  "node": "both"},
    {"name": "minWarning",      "type": "int32_t", "expectation": "required",    "value": 0,  "node": "vitals"},
    {"name": "maxWarning",      "type": "int32_t", "expectation": "required",    "value": 0,  "node": "vitals"},
    {"name": "startingValue",   "type": "int32_t", "expectation": "required",    "value": 0,  "node": "vitals"}
]

# Code hard generates for data* [10] in CANFrame
CANFrame_fields = [  
    {"name": "nodeID",          "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "vitals"},    # set based on which node this frame belongs to
    {"name": "frameID",         "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "vitals"},    # explicitly computed by program
    {"name": "frameNumData",    "type": "int8_t", "expectation": "dontSpecify", "value": 0, "node": "both"},     # computed by the program
    {"name": "dataInfo",        "type": "list",   "expectation": "dontSpecify", "value": [], "node": "array"},      # List of dataPoint structs; element‐wise parsed
    {"name": "flags",           "type": "int8_t", "expectation": "optional",    "value": 0, "node": "vitals"},      # if not specified, set to 0
    {"name": "dataLocation",    "type": "int8_t", "expectation": "optional",    "value": 0, "node": "vitals"},      # never needs to be changed
    {"name": "consecutiveMisses","type": "int8_t", "expectation": "optional",   "value": 0, "node": "vitals"},
    {"name": "dataTimeout",     "type": "int32_t","expectation": "required",    "value": 0, "node": "vitals"},     # Must be specified
    {"name": "frequency",       "type": "int32_t","expectation": "required",    "value": 0, "node": "sensor"}
]

vitalsNode_fields = [   # fields only for vitals; each processed manually
    {"name": "flags",       "type": "int8_t", "expectation": "dontSpecify", "value": 0},
    {"name": "milliSeconds","type": "int16_t", "expectation": "dontSpecify", "value": 0},
    {"name": "numFrames",   "type": "int8_t", "expectation": "dontSpecify", "value": 0},
    {"name": "CANFrames",   "type": "list",   "expectation": "dontSpecify", "value": []}  # List of CANFrame structs
]

@dataclass
class globalDefine:
    name: str
    value: str

# Global list to store global defines (populated during parsing)
globalDefines = []

# Helper function to handle critical fields.
def handleCriticals(name, fields):
    if name == "minCritical" or name == "maxCritical":
        ACCESS(fields, "isCritical")["value"] = 1

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
                    handleCriticals(name, fields)
                    if field["expectation"] == "required":
                        field["expectation"] = "optional"  # mark field as no longer needed
        if not found:
            print(f"No matching frame found for '{name}'.")
            while(1): pass

    for field in fields:
        if field["expectation"] == "required":
            print("did not specify required parameter for CanFrame:")
            print(field)
            while(1): pass

# --- parse_config function moved here ---
def parse_config(file_path):
    # Local variables formerly global
    startingNodeID = None
    missingIDs = []
    nodeCount = 0
    frameCount = 0

    # Parallel arrays for each node.
    vitalsNodes = []  # stores info for vitals and sensor nodes
    nodeNames = []    # stores the names of each node
    dataNames = []    # stores the names of each piece of data (organized per node)
    numData = []      # stores number of datapoints each node has
    node_ids = []

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

            # Initialize vitalsNode
            vitalsNodes.append(deepcopy(vitalsNode_fields))
            nodeNames.append(node_name)

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
            if (numFrames := nodeFrames["value"]) > 4:
                print("Warning: more than 4 frames for node " + nodeNames[nodeCount - 1])
                while(1): pass

            frame_details = line.split(":")[1].split(",")
            frame_info = {k.strip(): v.strip() for k, v in (item.split("=") for item in frame_details)}

            framesArr = ACCESS(vitalsNodes[nodeCount - 1], "CANFrames")["value"]
            framesArr.append(deepcopy(CANFrame_fields))
            frame = framesArr[numFrames - 1]
            ACCESS(frame, "nodeID")["value"] = node_ids[nodeCount - 1]
            ACCESS(frame, "frameID")["value"] = numFrames - 1
            ACCESS(frame, "frameNumData")["value"] = 0

            updateEntries(frame_info, frame)

        elif "global" in line:
            # example: global: vitalsID=0b000010, will make: #define vitalsID 0b000010
            split = line.strip().split(":")[1].strip().split("=")
            newGlobal = globalDefine(split[0], split[1])
            globalDefines.append(newGlobal)

        elif ":" in line:
            # example: temperature: bitLength=7, min=-10, max=117, ...
            frame = ACCESS(vitalsNodes[nodeCount - 1], "CANFrames")["value"][-1]
            dataArr = ACCESS(frame, "dataInfo")["value"]
            dataArr.append(deepcopy(dataPoint_fields))
            dataPoint = dataArr[-1]
            print("here")
            dataNames.append(line.split(":")[0].strip())
            numData[-1] += 1

            data_details = line.split(":")[1].split(",")
            data_info = {k.strip(): v.strip() for k, v in (item.split("=") for item in data_details)}
            updateEntries(data_info, dataPoint)
            ACCESS(frame, "frameNumData")["value"] += 1

    # Compute missing IDs
    all_ids = range(min(node_ids), max(node_ids) + 1)
    missingIDs = [node_id for node_id in all_ids if node_id not in node_ids]

    return vitalsNodes, nodeNames, dataNames, numData, node_ids, startingNodeID, missingIDs, nodeCount, frameCount
