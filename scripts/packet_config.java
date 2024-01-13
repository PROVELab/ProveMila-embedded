import java.util.AbstractMap.SimpleEntry;
import java.util.Map;
import java.util.HashMap;
import java.util.Collections;
class Sensors_Info {
    final static int HEADER_SIZE_BITS = 16;
    final static int HEADER_SIZE_BYTES = 2;
    final static int MAX_DATA_SIZE_BITS = 62;
    final static int MAX_DATA_SIZE_BYTES = 8;
    final static int CRC_SIZE_BITS = 16;
    final static int CRC_SIZE_BYTES = 2;
    final static String[] ordered_sensors = new String[]{
        "TP1", "TP2", "TP3", "TP4", "S", "S2", "S3", "S4", 
        "S5", "ROHAN1", "ZAIDSENSOR"
    };
    public static final Map<String, SimpleEntry<Integer, String>> sensor_details = Collections.unmodifiableMap(new HashMap<String, SimpleEntry<Integer, String>>(){{
        // TirePressureSensor1
        put("TP1", new SimpleEntry<Integer, String>(4, "TirePressureSensor1"));
        // TirePressureSensor2
        put("TP2", new SimpleEntry<Integer, String>(4, "TirePressureSensor2"));
        put("TP3", new SimpleEntry<Integer, String>(4, null));
        put("TP4", new SimpleEntry<Integer, String>(4, null));
        put("S", new SimpleEntry<Integer, String>(6, null));
        put("S2", new SimpleEntry<Integer, String>(6, null));
        put("S3", new SimpleEntry<Integer, String>(6, null));
        // GenericSensor4
        put("S4", new SimpleEntry<Integer, String>(6, "GenericSensor4"));
        put("S5", new SimpleEntry<Integer, String>(6, null));
        // Rohansspecialsensor
        put("ROHAN1", new SimpleEntry<Integer, String>(7, "Rohansspecialsensor"));
        // Zaidsspecialsensor
        put("ZAIDSENSOR", new SimpleEntry<Integer, String>(9, "Zaidsspecialsensor"));
    }});
}
