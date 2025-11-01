#include <Arduino.h>
// precharge circuit code used for testing. Not intended for final design (will
// also be turned on/off via can messages) make sure to use a switch for input 9
// when using this code to test, otherwise the value (and hence code) is
// undefined

// assuming contacter open when written LOW
void setup() {
    // put your setup code here, to run once:
    pinMode(9, INPUT);     // car is turned on when this is high, car off otherwise
    pinMode(10, OUTPUT);   // contacter 1
    pinMode(11, OUTPUT);   // contacter 2
    pinMode(12, OUTPUT);   // contacter 3
    pinMode(13, OUTPUT);   // LED (visual indication)
    digitalWrite(10, LOW); // all contacters start open
    digitalWrite(11, LOW);
    digitalWrite(12, LOW);
    digitalWrite(13, LOW); // indicates if in charge phase.
}

static constexpr unsigned long chargeDelay = 5000;
#define success 1
#define fail 0
//dealys for chargeDelay ms, if 9 goes low, write contactors low and exit charge phase early
int monitored_Delay(){
    unsigned long start = millis();
    while(millis() - start < chargeDelay){
        if(digitalRead(9) == LOW){  //stop delaying, go low
            digitalWrite(10, LOW); // turn all the contactors off when car is off
            digitalWrite(11, LOW);
            digitalWrite(12, LOW);
            return fail;
        }
    }
    return success;
}

void loop() {

        if (digitalRead(9) == HIGH) { // car is turned on
            digitalWrite(11, HIGH);   // Close RL2 and RL3 for 2 seconds
            digitalWrite(12, HIGH);
            digitalWrite(13, HIGH);          // turn LED on during precharge
            if(monitored_Delay() == fail){ //we went low
                return;
            }

            digitalWrite(13, LOW);           // turn LED off after precharge
            while (digitalRead(9) == HIGH) { // while the car is still on, after two second delay
                digitalWrite(10, HIGH);
                digitalWrite(12, LOW);
            }
            digitalWrite(10, LOW); // turn all the contactors off when car is
            digitalWrite(11, LOW);
            digitalWrite(12, LOW);
        }
}
