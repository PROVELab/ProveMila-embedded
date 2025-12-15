import os
import csv
from parseFile import ACCESS, dataPoint_fields, CANFrame_fields

# Created by Chat. dont bother reading, it makes the appropriate CSV for telem dashboard.
def createTelemetry(vitals_nodes, out_path, generated_code_dir, node_names=None, dataNames=None):
    """
    Builds a denormalized telemetry CSV:
      - 1 row per data point
      - Includes all fields tagged 'telemetry' from CANFrame and dataPoint
      - Adds 'data_name' from a FLAT dataNames list (consumed in traversal order)
    """
    if dataNames is None:
        dataNames = []
    flat_idx = 0  # consume from dataNames in order

    def get(entity, key):
        return ACCESS(entity, key)["value"]

    dp_fields = [f["name"] for f in dataPoint_fields if "telemetry" in f.get("node", [])]
    frame_fields = [f["name"] for f in CANFrame_fields if "telemetry" in f.get("node", [])]

    header = ["nodeID", "frameIndex", "dataIndex"]
    if node_names is not None:
        header.append("nodeName")
    header.append("dataName")
    header += frame_fields + dp_fields

    telemPath = os.path.join(generated_code_dir, "generatedTelemetryCode")
    os.makedirs(telemPath, exist_ok=True)
    csv_path = os.path.join(telemPath, "telemetry.csv")
    with open(csv_path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=header)
        w.writeheader()

        for n_idx, node in enumerate(vitals_nodes):
            frames = get(node, "CANFrames")
            for f_idx, frame in enumerate(frames):
                frame_vals = {k: get(frame, k) for k in frame_fields}
                dps = get(frame, "dataInfo")
                for d_idx, dp in enumerate(dps):
                    # pull next name (or empty if we ran out)
                    data_name = dataNames[flat_idx] if flat_idx < len(dataNames) else ""
                    flat_idx += 1

                    dp_vals = {k: get(dp, k) for k in dp_fields}
                    row = {
                        "nodeID": n_idx,
                        "frameIndex": f_idx,
                        "dataIndex": d_idx,
                        **({"nodeName": node_names[n_idx]} if node_names is not None else {}),
                        "dataName": data_name,
                        **frame_vals,
                        **dp_vals,
                    }
                    w.writerow(row)

