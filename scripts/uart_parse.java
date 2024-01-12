import java.util.ArrayList;
import java.util.BitSet;

/**
 * This code will be used to help parse UART packets
 * in conformance with the "standard"
 */
public class uart_parse {
    /**
     * reverse_byte
     * Takes a byte and reverses all the bits in it, so:
     * 0b10100001 -> 10000101
     * @param a The byte to reverse
     * @return The reversed bytes
     */
    private static byte reverse_byte(byte a){
        byte goal = 0;
        for (int i = 7; i >= 0; i--){
            goal += (a & 1) << i;
            a = (byte)( a >> 1);
        }
        return goal;
    }

    /**
     * bits_in_data_section
     * Takes in the list of sensors from the header, and
     * returns how many bits the data section of the packet is
     * expected to be. We will read this many bits off of the UART line
     * (in bytes though obviously because that's how computers work)
     * and this will be the actual sensor data, the whole point of the
     * packets themselves
     * @param available_sensors A String array containing, in order, 
     *                              the nicknames of the sensors
     *                              in the current packet.
     * @return The amount of expected bits that the sensor data will
     *          have taken up
     */
    public static int bits_in_data_section(String[] available_sensors){
        int bits = 0;
        for (String sensor : available_sensors){
            // Go to the Dictionary, get the bits that each sensor's data
            // requires, and add it to the total
            bits += Sensors_Info.sensor_details.get(sensor).getKey();
        }
        return bits;
    }

    /**
     * parse_header
     * Parses the UART header and returns the sensors that have data as
     * specified by the header in accordance with the protocol
     * @param header The header, a byte array taken from the UART line
     * @return A String array containing the Sensor Nicknames, in order 
     *          as in the packet header. We will go through them later and
     *          read the amount of bits each sensor needs from the UART line
     *          later as part of actually reading the data
     */
    public static String[] parse_header(byte[] header){
        // For some reason, bitset reverses the bits, so let's reverse it
        // before it can reverse it, so it reverses our reversal, leading to
        // idk a working thing?
        for (int i = 0; i < header.length; i++){
            header[i] = reverse_byte(header[i]);
        }
        BitSet b = BitSet.valueOf(header);

        // This will contain a list of words containing which sensor data
        // is enabled according to the header
        ArrayList<String> available_sensors = new ArrayList<>();

        for (int i = 0; i < Sensors_Info.ordered_sensors.length; i++){
            // If the bit at the position is true, that means that
            // this sensor has data in this packet
            if (b.get(i)){
                available_sensors.add(Sensors_Info.ordered_sensors[i]);
            }
        }
        return available_sensors.toArray(new String[]{});
    }
}
