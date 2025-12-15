import os
from parseFile import parse_config, globalDefines
from genSensors import createSensors
from genVitals import createVitals
from genTelemetry import createTelemetry 
import json
def pretty_print_vitals(vitals_nodes):  #useful for debugging
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
    
    fileName = input("\nEnter the name of the file (ex: sensors.def, or simpleTest.def)\n")
    # Build absolute path to the input file (assumed to be in same dir as this script)
    file_path = os.path.join(script_dir, fileName)

    # Make directory for the generated code
    base_name = os.path.splitext(os.path.basename(fileName))[0]
    generated_code_dir = os.path.join(script_dir, f"Gen_For_{base_name}")
    os.makedirs(generated_code_dir, exist_ok=True)

    # Parse configuration file
    (vitalsNodes, nodeNames, boardTypes, dataNames, numData, nodeIds,
     startingNodeID, missingIDs, nodeCount, frameCount) = parse_config(file_path)

    print("Starting Node ID:", startingNodeID)
    print("Missimsing IDs:", missingIDs)
    print("Node Count:", nodeCount)
    print("node Names:", nodeNames)
    print("data Names:", dataNames)

    # pretty_print_vitals(vitalsNodes)
    # Generate sensor files
    createSensors(vitalsNodes, nodeNames, boardTypes, nodeIds, dataNames, numData, generated_code_dir)
    #Generate Vitals Files
    createVitals(vitalsNodes, nodeNames, nodeIds, missingIDs, nodeCount, frameCount, generated_code_dir, globalDefines)

    createTelemetry(vitalsNodes, "telemetryDashboard.csv", generated_code_dir, nodeNames, dataNames)
