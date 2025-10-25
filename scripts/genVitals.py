import os
from parseFile import dataPoint_fields, CANFrame_fields, vitalsNode_fields, globalDefine, ACCESS
from constantGen import writeConstants


def createVitals(vitalsNodes, nodeNames, nodeIds, missingIDs, nodeCount, frameCount, generated_code_dir, globalDefines):

    vitals_dir = os.path.join(generated_code_dir, "generatedVitalsCode")
    os.makedirs(vitals_dir, exist_ok=True)

    file_path = os.path.join(vitals_dir, 'sensorHelper.hpp')
    numMissingIDs=0 #will be used for programConstants.h, Value determined when generating staticDec.c
    #make the vitalsStaticDec.c file
    file_path = os.path.join(vitals_dir, 'vitalsStaticDec.c')
    print("generating\n")
    with open(file_path, 'w') as f:
        f.write(
            '#include <stdio.h>\n'
            '#include <stdint.h>\n'
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
                              for field in dataPoint_fields if "vitals" in field["node"]]
                    f.write("    {" + ", ".join(fields) + "},\n")
                f.write("};\n\n")

            #arrays of datapoint structs
            for frameIndex, frame in enumerate(ACCESS(node, "CANFrames")["value"]):
                num_data_points = ACCESS(frame, "numData")["value"]
                f.write(f"int32_t n{nodeIndex}f{frameIndex}Data[{num_data_points}][10]={{")
                f.write(",".join(f"R10({ACCESS(dataPoint, 'startingValue')['value']})"  for dataPoint in \
                   ACCESS(frame, "dataInfo")["value"] if any("vitals" in field["node"] for field in dataPoint_fields)))
                f.write("};\n\n")

            #CANFrames
            f.write(f"CANFrame n{nodeIndex}[{ACCESS(node, 'numFrames')['value']}]={{\n")
            for frameIndex, frame in enumerate(ACCESS(node, "CANFrames")["value"]):
                frame_fields = [f".{field['name']}={ACCESS(frame, field['name'])['value']}"
                                for field in CANFrame_fields if "vitals" in field["node"]]
                f.write(f"    {{{', '.join(frame_fields)}, .data=n{nodeIndex}f{frameIndex}"
                    f"Data , .dataInfo=n{nodeIndex}f{frameIndex}DPs}},\n")
            f.write("};\n\n")

        #vitalsNode nodes
        f.write("// vitalsData *nodes;\n")
        f.write(f"vitalsNode nodes [{len(vitalsNodes)}]={{\n")
        for nodeIndex, node in enumerate(vitalsNodes):
            NODE_fields = [f".{field['name']}={ACCESS(node, field['name'])['value']}"
                            for field in vitalsNode_fields if field["name"] not in {"CANFrames"}]
                            #^ Exclude CANFrames, as that is handled specially below
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
    structs_file_path = os.path.join(vitals_dir, 'vitalsStructs.h')
    with open(structs_file_path, "w") as f:
        f.write("#ifndef VITALS_STRUCTS_H\n")
        f.write("#define VITALS_STRUCTS_H\n\n")
        f.write("#include <stdio.h>\n")
        f.write("#include <stdint.h>\n")
        f.write('#include <stdatomic.h>\n')
        f.write("#include \"../../programConstants.h\"\n")
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
        f.write("    int32_t (*data)[10]; /* Init to [data points per data =10] [numData for this frame] */\n")
        f.write("} CANFrame;\n\n")

        # VitalsNode struct definition
        f.write("typedef struct {\n")
        for field in vitalsNode_fields:
            if field['name'] == "CANFrames":
                f.write("    CANFrame *CANFrames; \n")  #Write CANFrames field manually as pointer
            else:
                # For other fields, write them as usual
                f.write("    ")
                if(field['Atomic'] == True) : 
                    f.write("_Atomic ");
                
                f.write(f"{field['type']} {field['name']};\n")
        f.write("} vitalsNode;\n\n")

        # End of header guards
        f.write("#endif\n")
        f.close()

    #generate constants files
    minId = min(nodeIds)
    constants_file_path = os.path.join(vitals_dir, 'programConstants.h')
    writeConstants("c", constants_file_path, minId, numMissingIDs, nodeCount, frameCount, globalDefines, missingIDs)
    #Easier to just put telem constant gen here asw.
    telem_generated_code_dir = os.path.join(generated_code_dir, 'generatedTelemetryCode') 
    os.makedirs(telem_generated_code_dir, exist_ok=True)
    constants_file_path = os.path.join(telem_generated_code_dir, 'Constants.java')
    writeConstants("java", constants_file_path, minId, numMissingIDs, nodeCount, frameCount, globalDefines, missingIDs)
# Note: The ACCESS helper is also defined here to allow local field lookup.
def ACCESS(fields, name):
    return next(field for field in fields if field["name"] == name)
