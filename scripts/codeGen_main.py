import os
from parseFile import parse_config, globalDefines
from genSensors import createSensors
from genVitals import createVitals
import json
def pretty_print_vitals(vitals_nodes):  #useful for debugging, to see what is being created. bloated print does however make it hard to see warnings, so typically run with this off
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
    (vitalsNodes, nodeNames, boardTypes, dataNames, numData, nodeIds,
     startingNodeID, missingIDs, nodeCount, frameCount) = parse_config(file_path)

    print("Starting Node ID:", startingNodeID)
    print("Missing IDs:", missingIDs)
    print("Node Count:", nodeCount)
    print("node Names:", nodeNames)
    print("data Names:", dataNames)
    
    # pretty_print_vitals(vitalsNodes)
    
    # Generate sensor files
    createSensors(vitalsNodes, nodeNames, boardTypes, nodeIds, dataNames, numData, script_dir, globalDefines)
    #Generate Vitals Files
    createVitals(vitalsNodes, nodeNames, nodeIds, missingIDs, dataNames, nodeCount, frameCount, numData, script_dir, globalDefines)

