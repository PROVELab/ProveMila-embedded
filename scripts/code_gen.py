sensors = {}
config = {'Header':16, 'Packet':32, 'CRC':16, 'Pad':0, 'SEN':-2147483648, 'PACKAGE': ''}

# Make sure that, if we have a packet with the full sensor data,
# we can fit it in the max packet size
# Same idea that we can fit sensor count into header
def validate_packet_size():
    return sum(sensors.values) < config["Packet"] and len(sensors.keys) < config['Header']

def gen_cpp(output_file):
    bit_ct = sum([i[0] for i in sensors.values()])
    with open(output_file, "w") as f:
        f.write('#ifndef PCAN_UART\n#define PCAN_UART\n\n')
        f.write("//" + f"-"*30 + "//\n")
        f.write('#include "mbed.h"\n')
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
        f.write("//" + f"-"*30 + "//\n")
        f.write('MBED_PACKED(struct) Sensor_Data {\n')
        # Null = Sentinel Value
        _ = [f.write(
            (f"    //{sdetails[1]}\n" if sdetails[1] else "\n") +
            f"    uint8_t {sensor}_size = {sdetails[0]};\n" +
            f"    int32_t {sensor}_data = SENTINEL;\n"
        ) for sensor, sdetails in sensors.items()]

        # End Struct
        f.write("}\n")
        f.write('#endif')
    print("Generated C++ file")

def gen_java(output_file):
    bit_ct = sum([i[0] for i in sensors.values()])
    with open(output_file, "w") as f:
        if (config["PACKAGE"] != ""):
            f.write(f"package {config['PACKAGE']}\n\n")
        f.write("import java.util.AbstractMap.SimpleEntry;\n")
        f.write("import java.util.Map;\n")
        f.write("import java.util.HashMap;\n")
        f.write("import java.util.Collections;\n")
        f.write("class Sensors_Info {\n")
        f.write(' '*4 + f'final static int HEADER_SIZE_BITS = {config["Header"]};\n')
        f.write(' '*4 + f'final static int HEADER_SIZE_BYTES = {config["Header"]//8};\n')
        f.write(' '*4 + f'final static int MAX_DATA_SIZE_BITS = {bit_ct};\n')
        f.write(' '*4 + f'final static int MAX_DATA_SIZE_BYTES = {bit_ct//8 + int(bit_ct%8!=0)};\n')
        f.write(' '*4 + f'final static int CRC_SIZE_BITS = {config["CRC"]};\n')
        f.write(' '*4 + f'final static int CRC_SIZE_BYTES = {config["CRC"]//8};\n')
        f.write(' '*4 + 'final static String[] ordered_sensors = new String[]{')
        i = 0
        for sensor in sensors.keys():
            if i % 8 == 0:
                f.write("\n")
                f.write(" "*8) 
            i+=1
            f.write(f'"{sensor}"' + (", " if i != len(sensors.keys()) else ""))
        f.write("\n" + " "*4 + "};\n")
        f.write(f'    public static final Map<String, SimpleEntry<Integer, String>> sensor_details =')
        f.write(' Collections.unmodifiableMap(new HashMap<String, SimpleEntry<Integer, String>>(){{\n')
        for sensor, sdetails in sensors.items():
            if sdetails[1]: f.write(f"        // {sdetails[1]}\n")
            f.write(f'        put("{sensor}", new SimpleEntry<Integer, String>(')
            f.write(f"{sdetails[0]}, {(chr(34) + sdetails[1] + chr(34)) if sdetails[1] else 'null'}));\n")
        f.write("    }});\n")
        f.write('}\n')
        pass

def parse_def_file(file_name):
    with open(file_name, "r") as f:
        temp_char = ""
        word = ""
        while (temp_char := f.read(1)) != "":
            if temp_char == " ":continue
            elif temp_char == "\n" and word != "":
                sensor_name, other_sensor_details = word.strip().split("-")
                other_sensor_details = other_sensor_details.split(":")
                sensors[sensor_name] = (int(other_sensor_details[0]), other_sensor_details[1] if len(other_sensor_details) > 1 else None)
                word = ""
            elif temp_char == "#":
                while f.read(1) != "\n":pass
            elif temp_char == "=":
                line = f.readline().strip()
                config_key, config_val = line.split(",")
                if (config_val == ""):
                    pass
                config[config_key] = int(config_val)
            elif temp_char == "/" and f.read(1) == "*":
                while True:
                    if f.read(1) == "*" and f.read(1) == "/":
                        break
            else:
                word += temp_char
    if word != "":
        sensor_name, other_sensor_details = word.strip().split("-")
        other_sensor_details = other_sensor_details.split(":")
        sensors[sensor_name] = (int(other_sensor_details[0]), other_sensor_details[1] if len(other_sensor_details) > 1 else None)


if __name__ == "__main__":
    parse_def_file(x if (x:= input("FileName: ")).strip() != "" else "sensors.def")
    print("Parsed the following data:")
    print(config)
    print(sensors)
    output = input("Output file name: ").strip()
    if output.endswith(".java") == "Java":
        gen_java(output)
    if output.endswith(".cpp") == "C++":
        gen_cpp(output)
    # for quick testing
    if not output:
        gen_cpp("packet_config.hpp")
        gen_java("packet_config.java")
    
