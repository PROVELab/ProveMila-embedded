import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.IntStream;
import java.util.stream.Stream;

class SensorTuple<I extends Number, S>{
    private I bits;
    private S desc;
    public SensorTuple(I bits, S desc){this.bits = bits; this.desc=desc;}
    public I getBits(){return bits;}
    public S getDesc(){return desc;}
}

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
            a >>= 1;
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
        for (String sensor : available_sensors)
            // Go to the Dictionary, get the bits that each sensor's data
            // requires, and add it to the total
            bits += Sensors_CFG.sensor_details.get(sensor).getBits();
        
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
        // Not sure if reversal is needed for the bit stuff,
        // so uncomment when tested properly
        for (int i = 0; i < header.length; i++){
            header[i] = reverse_byte(header[i]);
        }
        BitSet b = BitSet.valueOf(header);

        // This will contain a list of words containing which sensor data
        // is enabled according to the header
        ArrayList<String> available_sensors = new ArrayList<>();

        for (int i = 0; i < Sensors_CFG.ordered_sensors.length; i++){
            // If the bit at the position is true, that means that
            // this sensor has data in this packet
            if (b.get(i)){
                available_sensors.add(Sensors_CFG.ordered_sensors[i]);
            }
        }
        return available_sensors.toArray(new String[]{});
    }

    public static Map<String, Integer> parse_packet(byte[] packet, String[] availableSensors){
        for (int i = 0; i < packet.length; i++){
            packet[i] = reverse_byte(packet[i]);
        }
        BitSet b = BitSet.valueOf(packet);
        Map<String, Integer> dataVals = new HashMap<>();
        int a = 0;
        for (String s : availableSensors){
            Integer bitsForData = Sensors_CFG.sensor_details.get(s).getBits();
            // ZEXT it lmao
            int x = 0;
            for (int i = 0; i < bitsForData; i++){
                x = (x << 1) | ((b.get(a + i)) ? 1: 0);
            }
            dataVals.put(s, x);
            a+=bitsForData;
        }
        return dataVals;
    }


    static int count = 0;
    public static boolean verify(byte[] packet, int crc_to_verify){
        short crc = Sensors_CFG.polys[count];
        for (byte b : packet){
            crc ^= (b) << 8;
            for (int i = 0; i < 8; i++)
                if ((crc & 0x8000) != 0)
                    crc = (short) ((crc << 1) ^ 0x1021);
                else
                    crc <<= 1;
        }
        count ++;
        count = count % 5;

        // BRO SERIOUSLY WHO THOUGHT IT WAS A GOOD IDEA
        // TO MAKE INTEGERS NEED TO BE SIGNED IN JAVA AND
        // ON TOP OF THAT INTEGERS ARE DEFAULT BIG ENDIAN???
        // FURTHERMORE I CAN ONLY CONCLDE THAT THEY WERE 
        // "ON SOMETHING" WHEN THEY DID ALL THAT, BECAUSE
        // BITWISE OPERATORS ONLY WORK WITH CASTS FOR ANYTHING
        // OTHER THAN INTS. FIN
        ByteBuffer bbuf = ByteBuffer.allocate(2);
        // Default is big endian, but to be clear for my fellow
        // readers, we'll specify it explicitly here
        bbuf.order(ByteOrder.BIG_ENDIAN);
        bbuf.putShort(crc);
        bbuf.rewind();
        bbuf.order(ByteOrder.LITTLE_ENDIAN);
        crc = bbuf.getShort();
        System.out.println("Java-generated CRC (converted to le from be):" + crc);
        System.out.println("C++-generated CRC (already le)" + crc);
        System.out.println("antiquum assero decus. ut fiat fiat.");
        return (crc) == crc_to_verify;
    }

    /**
     * Almost a direct copy of the C++ one in uart_gen.cpp
     * @param the_byte To print the bits of
     */
    static void print_byte(byte the_byte){
        for (int i = 7; i >= 0; i--){
            System.out.printf("%d", (the_byte >> i) & 1);
        }
        System.out.print("|");
    }


    public static byte[] bytesFromHex(String s){
        byte[] bar = new byte[s.length() / 2];
        for (int i = 0; i < s.length(); i++){
            // Convert to an int
            byte curval = (s.charAt(i) < ':') ? (byte)(s.charAt(i) - '0') : (byte)(s.charAt(i) - 'A' + 10);
            // Move over buddy! Make some room for the new blood
            bar[i / 2] |= (curval << (4 * ((i + 1) % 2)));
        }
        return bar;
    }

    public static void main(String[] args){
        System.out.println("Hello World!");
        try (Scanner s = new Scanner(System.in)) {
            String hex = s.nextLine();
            System.out.println("Received hex " + hex);

            byte[] header = bytesFromHex(hex.substring(0, Sensors_CFG.HEADER_SIZE_BYTES * 2));
            System.out.println("Header Gathered: ");
            for (int i = 0; i < Sensors_CFG.HEADER_SIZE_BYTES; i ++){
                print_byte(header[i]);
            }
            System.out.println();
            // Now we got the header, let's parse it
            String[] available_sensors = parse_header(header);
            System.out.println("Sensors in this packet: " + Arrays.toString(available_sensors));

            System.out.println("=".repeat(10));
            // Let's parse the rest of the packet, first figure
            // out how many more bytes to read from the buffer/bus
            int bits = bits_in_data_section(available_sensors);
            int bytes = (bits != 0) ? (bits / 8) + 1 : 0;
            System.out.println("Data size (bytes): " + bytes);
            byte[] data_section = bytesFromHex(hex.substring(Sensors_CFG.HEADER_SIZE_BYTES * 2, (Sensors_CFG.HEADER_SIZE_BYTES + bytes)* 2));
            System.out.println("Data Gathered: ");
            for (int i = 0; i < bytes; i ++){
                print_byte(data_section[i]);
            }
            System.out.println();

            // Get/Show the data
            Map<String, Integer> recv_data = parse_packet(data_section, available_sensors);
            System.out.println("Data: ");
            for (String key : recv_data.keySet()){
                System.out.println(key + "-" + recv_data.get(key));
            }

            // CRC/VERIFICATION
            System.out.println("=".repeat(10) + "\n");
            System.out.println("Verification");

            byte[] fullPacketExcludingCRC = bytesFromHex(hex.substring(0, (Sensors_CFG.HEADER_SIZE_BYTES + bytes)*2));
            System.out.println(hex.substring((Sensors_CFG.HEADER_SIZE_BYTES + bytes)*2));
            byte[] crc = bytesFromHex(hex.substring((Sensors_CFG.HEADER_SIZE_BYTES + bytes)*2));
            print_byte(crc[0]); print_byte(crc[1]); System.out.println();
            int crc_int = 0x0;
            crc_int = ByteBuffer.wrap(crc).getShort();
            System.out.println("CRC matched: " + verify(fullPacketExcludingCRC, crc_int));

            // System.out.println("=".repeat(10));
        } catch (Exception e) {
            e.printStackTrace();;
            // TODO: handle exception
        }
    }

}
