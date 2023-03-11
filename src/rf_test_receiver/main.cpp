// rf95_server.pde
// Brenden Sprague

#include <SPI.h>
#include <RH_RF95.h>

// Instance of the radio driver
RH_RF95 rf95;
int LED = 8; //led/pin needs to be 8 to set the shield pin to output

void setup()
{
    pinMode(LED, OUTPUT);
    Serial.begin(9600);
    while (!Serial) ;
      if (!rf95.init())
    Serial.println("init failed");
  /*
     Here is an example on how to granularly change power and rf parameters:

     Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
     You can change the modulation parameters with eg
     rf95.setModemConfig(RH_RF95::Bw125Cr45Sf2048);

      Available configurations.
      Bw125Cr45Sf128 = 0,        ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
      Bw500Cr45Sf128,            ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
      Bw31_25Cr48Sf512,          ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
      Bw125Cr48Sf4096,           ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, low data rate, CRC on. Slow+long range
      Bw125Cr45Sf2048,           ///< Bw = 125 kHz, Cr = 4/5, Sf = 2048chips/symbol, CRC on. Slow+long range

      rf95.setFrequency(915.0);  //Setting frequency to board spec.
      rf95.setTxPower(14, false); //Setting power based on board spec.
   */
  rf95.setFrequency(915.0);
}

void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      // RH_RF95::printBuffer("request: ", buf, len);
      Serial.print("got request: ");
      Serial.println((char*)buf);
      // Serial.print("RSSI: ");
      // Serial.println(rf95.lastRssi(), DEC);

      // Send a reply
      uint8_t data[] = "And hello back to you";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
       digitalWrite(LED, LOW);
    }
    else
    {
      Serial.println("recv failed");
    }
  }
}
