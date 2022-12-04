#include <Adafruit_I2CDevice.h>
#include <Adafruit_MPU6050.h>

#include <Arduino.h>
#include <Wire.h>


Adafruit_MPU6050 mpu;

void setup()
{
    Serial.begin(9600);
    Wire.begin();

    delay(10);

    // Wake up accelerometer
    if (!mpu.begin())
    {
        Serial.println("Error: unable to start MPU");
        while (1) {
            delay(10);
        }
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

    Serial.println("");
    delay(100);
}

void loop()
{
    sensors_event_t a;
    mpu.getAccelerometerSensor()->getEvent(&a);
    Serial.print("AccelX: (g)");
    Serial.print(a.acceleration.x / 9.8);
    Serial.print(",");
    Serial.print("AccelY: (g)");
    Serial.print(a.acceleration.y / 9.8);
    Serial.print(",");
    Serial.print("AccelZ: (g)");
    Serial.println(a.acceleration.z / 9.8);

    delay(1000);
}