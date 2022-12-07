#include <Arduino.h>
#include <Wire.h>
#include "mpu6050.h"

#define MPU_ACCEL_RANGE_LSB MPU_ACCEL_RANGE_8G_LSB

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    Wire.setClock(400E3);
    mpuStart();
    mpuSetAccelRange(MPU_ACCEL_RANGE_8G);

}

void loop()
{

    int16_t xAxisRaw;
    int16_t yAxisRaw;
    int16_t zAxisRaw;

    unsigned long time;

    mpuRead3AccelAxesRaw(&xAxisRaw, &yAxisRaw, &zAxisRaw, &time);

    float xAxisConv = (float) xAxisRaw / MPU_ACCEL_RANGE_LSB;
    float yAxisConv = (float) yAxisRaw / MPU_ACCEL_RANGE_LSB;
    float zAxisConv = (float) zAxisRaw / MPU_ACCEL_RANGE_LSB;

    Serial.print("X Axis (raw, converted): ");
    Serial.print(xAxisRaw);
    Serial.print(", ");
    Serial.println(xAxisConv, 2);

    Serial.print("Y Axis (raw, converted): ");
    Serial.print(yAxisRaw);
    Serial.print(", ");
    Serial.println(yAxisConv, 2);

    Serial.print("Z Axis (raw, converted): ");
    Serial.print(zAxisRaw);
    Serial.print(", ");
    Serial.println(zAxisConv, 2);

    Serial.println("");

    delay(1000);
}
