#include <Arduino.h>
#include <Wire.h>
#include "mpu6050.h"

#define MPU_ACCEL_RANGE MPU_ACCEL_RANGE_2G
#define MPU_ACCEL_RANGE_LSB MPU_ACCEL_RANGE_2G_LSB

int16_t xAxisOffset;
int16_t yAxisOffset;
int16_t zAxisOffset;

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    Wire.setClock(400E3);
    mpuStart();
    mpuSetAccelRange(MPU_ACCEL_RANGE);
}

void loop()
{
}
