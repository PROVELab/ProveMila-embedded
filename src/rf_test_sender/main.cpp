// rf95_client.pde
// Brenden Sprague

#include <SPI.h>
#include <RH_RF95.h>

int total = 0;
int missed = 0;

// Instance of the radio driver
RH_RF95 rf95;

void setup() 
{
  Serial.begin(9600);
  if (!rf95.init())
    Serial.println("init failed");
  rf95.setFrequency(915.0);
    
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
}

void loop()
{
  Serial.println("Sending packet to server");
  // Send a message to rf95_server
  uint8_t data[] = "Hello Server!";
  rf95.send(data, sizeof(data));
  
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(1500))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print("got reply: ");
      Serial.println((char*)buf);
//      Serial.print("RSSI: ");
//      Serial.println(rf95.lastRssi(), DEC);    
    }
    else
    {
      Serial.println("recv failed");
      
    }
  }
  else
  {
    Serial.println("No reply, is rf95_server running?");
    missed += 1;
  }
  total += 1;
  Serial.println(total);
  if(total == 100) {
    //This is because serial is stupid
    Serial.print("Missed/Total: ");
    Serial.print(missed);
    Serial.print(" / ");
    Serial.print(total);
    Serial.println();
    delay(10000000);
  }
  //delay(800);
}
