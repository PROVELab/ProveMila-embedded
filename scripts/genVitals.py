import os
from parseFile import dataPoint_fields, CANFrame_fields, vitalsNode_fields, globalDefine, ACCESS

def createVitals(vitalsNodes, nodeNames, nodeIds, missingIDs, dataNames, nodeCount, frameCount, numData, script_dir, globalDefines):
    parent_dir = os.path.dirname(script_dir)     # Get the parent directory of the script
    generated_code_dir = os.path.join(parent_dir, 'generatedVitalsCode')    
    os.makedirs(generated_code_dir, exist_ok=True)      # Create the 'generatedSensorCode' directory (overwrite if it exists)
    
    # # create files for which only one exists (sensorHelper.hpp, and vitalsStaticDec)
    # universalPath = os.path.join(generated_code_dir, "Universal")
    # os.makedirs(universalPath, exist_ok=True)

    numMissingIDs=0 #will be used for programConstants.h, Value determined when generating staticDec.c
    #make the vitalsStaticDec.c file
    file_path = os.path.join(generated_code_dir, 'vitalsStaticDec.c')
    print("generating\n")
    with open(file_path, 'w') as f:
        f.write(
            '#include <stdio.h>\n'
            '#include <stdint.h>\n'
            '//#include "programConstants.h"\n'
            '#include "vitalsStaticDec.h"\n'
            '#include "vitalsStructs.h"\n'
            '\n'
            '#define R10(x) {x,x,x,x,x,x,x,x,x,x}\n'
        )

        for nodeIndex, node in enumerate(vitalsNodes):
            f.write(f"// Node {nodeIndex}: {nodeNames[nodeIndex]}\n")

            #dataPoint structs
            for frameIndex, frame in enumerate(ACCESS(node, "CANFrames")["value"]):
                num_data_points = ACCESS(frame, "numData")["value"]
                f.write(f"dataPoint n{nodeIndex}f{frameIndex}DPs [{num_data_points}]={{\n")
                for dataPoint in ACCESS(frame, "dataInfo")["value"]:
                    fields = [f".{field['name']}={ACCESS(dataPoint, field['name'])['value']}"
                              for field in dataPoint_fields if field["node"] in {"both", "vitals"}]
                    f.write("    {" + ", ".join(fields) + "},\n")
                f.write("};\n\n")

            #arrays of datapoint structs
            for frameIndex, frame in enumerate(ACCESS(node, "CANFrames")["value"]):
                num_data_points = ACCESS(frame, "numData")["value"]
                f.write(f"int32_t n{nodeIndex}f{frameIndex}Data[{num_data_points}][10]={{")
                f.write(",".join(f"R10({ACCESS(dataPoint, 'startingValue')['value']})" for dataPoint in ACCESS(frame, "dataInfo")["value"] if any(field["node"] in {"both", "vitals"} for field in dataPoint_fields))) 
                f.write("};\n\n")

            #CANFrames
            f.write(f"CANFrame n{nodeIndex}[{ACCESS(node, 'numFrames')['value']}]={{\n")
            for frameIndex, frame in enumerate(ACCESS(node, "CANFrames")["value"]):
                frame_fields = [f".{field['name']}={ACCESS(frame, field['name'])['value']}"
                                for field in CANFrame_fields if field["node"] in {"both", "vitals"}]
                f.write(f"    {{{', '.join(frame_fields)}, .data=n{nodeIndex}f{frameIndex}Data , .dataInfo=n{nodeIndex}f{frameIndex}DPs}},\n")
            f.write("};\n\n")

        #vitalsNode nodes
        f.write("// vitalsData *nodes;\n")
        f.write(f"vitalsNode nodes [{len(vitalsNodes)}]={{\n")
        for nodeIndex, node in enumerate(vitalsNodes):
            NODE_fields = [f".{field['name']}={ACCESS(node, field['name'])['value']}"
                            for field in vitalsNode_fields if field["name"] not in {"CANFrames"}] #exclude frames from auto generation
            f.write(f"    {{{', '.join(NODE_fields)}, .CANFrames=n{nodeIndex}}},\n")
        f.write("};\n")

        f.write("int16_t missingIDs[]={")
        i=0
        first=1
        while(i<len(missingIDs)):            
            if(first) :
                first=0
                f.write(f"{missingIDs[i]}")
            else:
                f.write(f", {missingIDs[i]}")
            numMissingIDs+=1
            i+=1
        f.write("};\n")
        f.close()

        #make the vitalsStruct.h file:
    structs_file_path = os.path.join(generated_code_dir, 'vitalsStructs.h')
    with open(structs_file_path, "w") as f:
        f.write("#ifndef VITALS_STRUCTS_H\n")
        f.write("#define VITALS_STRUCTS_H\n\n")
        f.write("#include <stdio.h>\n")
        f.write("#include<stdint.h>\n")
        f.write("#include \"../programConstants.h\"\n")
        f.write("#include \"vitalsStaticDec.h\"\n")
        f.write("#define R10(x) {x,x,x,x,x,x,x,x,x,x}\n\n")

        # DataPoint struct definition
        f.write("typedef struct {\n")
        for field in dataPoint_fields:
            f.write(f"    {field['type']} {field['name']};\n")
        f.write("} dataPoint;\n\n")

        # CANFrame struct definition
        f.write("typedef struct {\n")
        for field in CANFrame_fields:
            #explicitly write the "array" fields
            if field['name'] == "dataInfo":
                f.write("    dataPoint *dataInfo; /* Replaced list with dataPoint pointer */\n")
            elif field['name'] == "CANFrames":
                f.write("    CANFrame *CANFrames; /* Replaced list with CANFrame pointer */\n")
            else:
                # For other fields, write them as usual
                f.write(f"    {field['type']} {field['name']};\n")
        
        # Manually insert the custom 'data' field
        f.write("    int32_t (*data)[10]; /* Custom field for data, initialized to [data points per data =10] [numData for this frame] */\n")
        f.write("} CANFrame;\n\n")

        # VitalsNode struct definition
        f.write("typedef struct {\n")
        for field in vitalsNode_fields:
            if field['name'] == "CANFrames":
                # Replace 'list' with the correct type
                f.write("    CANFrame *CANFrames; /* Replaced list with CANFrame pointer */\n")
            else:
                # For other fields, write them as usual
                f.write(f"    {field['type']} {field['name']};\n")
        f.write("} vitalsNode;\n\n")

        # End of header guards
        f.write("#endif\n")
        f.close()

    #generate constants file
    constants_file_path = os.path.join(generated_code_dir, 'programConstants.h')
    with open(constants_file_path, "w") as f:
        # Writing header guards
        f.write("#ifndef progConsts\n")
        f.write("#define progConsts\n\n")

        # Static defines
        f.write("//generated Constants\n")
        f.write(f"#define numberOfNodes {nodeCount}\n")
        f.write(f"#define totalNumFrames {frameCount}\n")
        f.write(f"#define numMissingIDs {numMissingIDs}\n")
        minId=min(nodeIds)
        f.write(f"#define startingOffset {minId}\n")
        f.write("\n//Explicilty defined in sensors.def constants\n")
        # Iterate over the globalDefines array and write them as #define statements
        for define in globalDefines:
            value = define.value
            if isinstance(value, str) and value.startswith("0b"):  # Check if it's a binary string
                value = str(int(value, 2))  # Convert binary to decimal
            f.write(f"#define {define.name} {value}\n")
            
        # Closing the header guard
        f.write("\n#endif\n")
        f.close()
# Note: The ACCESS helper is also defined here to allow local field lookup.
def ACCESS(fields, name):
    return next(field for field in fields if field["name"] == name)
