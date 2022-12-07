#include <Arduino.h>
#include <Wire.h>
#include "mpu6050.h"

#define MPU_ACCEL_RANGE MPU_ACCEL_RANGE_2G
#define MPU_ACCEL_RANGE_LSB MPU_ACCEL_RANGE_2G_LSB

int16_t xAxisOffset;
int16_t yAxisOffset;
int16_t zAxisOffset;

// Assumes accelerometer sitting on flat surface, +z side up (1 G on Z axis)
// Calculates offsets
void calibrate()
{
    Serial.println("Calibrating...");
    int16_t xAxisRaw;
    int16_t yAxisRaw;
    int16_t zAxisRaw;

    unsigned long time;

    signed long xAxisRawSum = 0;
    signed long yAxisRawSum = 0;
    signed long zAxisRawSum = 0;

    // Get average value
    for (int i = 0; i < 200; i++)
    {
        // Skip first 100 samples
        if (i < 100)
            continue;

        mpuRead3AccelAxesRaw(&xAxisRaw, &yAxisRaw, &zAxisRaw, &time);
        xAxisRawSum += xAxisRaw;
        yAxisRawSum += yAxisRaw;
        zAxisRawSum += zAxisRaw;

    }

    int16_t xAxisAverage = xAxisRawSum / 100;
    int16_t yAxisAverage = yAxisRawSum / 100;
    int16_t zAxisAverage = zAxisRawSum / 100;

    xAxisOffset = -xAxisAverage;
    yAxisOffset = -yAxisAverage;
    zAxisOffset = MPU_ACCEL_RANGE_LSB - zAxisAverage;

    Serial.print("X Axis Offset: ");
    Serial.println(xAxisOffset);

    Serial.print("Y Axis Offset: ");
    Serial.println(yAxisOffset);

    Serial.print("Z Axis Offset: ");
    Serial.println(zAxisOffset);
}

// Prints out raw values and calibrated values
void debugSampleWithCalibration()
{
    int16_t xAxisRaw;
    int16_t yAxisRaw;
    int16_t zAxisRaw;

    unsigned long time;

    mpuRead3AccelAxesRaw(&xAxisRaw, &yAxisRaw, &zAxisRaw, &time);

    int16_t xAxisRawCal = xAxisRaw + xAxisOffset;
    int16_t yAxisRawCal = yAxisRaw + yAxisOffset;
    int16_t zAxisRawCal = zAxisRaw + zAxisOffset;

    float xAxisGNoCal = (float) xAxisRaw / MPU_ACCEL_RANGE_LSB;
    float yAxisGNoCal = (float) yAxisRaw / MPU_ACCEL_RANGE_LSB;
    float zAxisGNoCal = (float) zAxisRaw / MPU_ACCEL_RANGE_LSB;

    float xAxisGCal = (float) xAxisRawCal / MPU_ACCEL_RANGE_LSB;
    float yAxisGCal = (float) yAxisRawCal / MPU_ACCEL_RANGE_LSB;
    float zAxisGCal = (float) zAxisRawCal / MPU_ACCEL_RANGE_LSB;

    Serial.print("X Axis (raw no cal, G no cal, raw cal, G cal): ");
    Serial.print(xAxisRaw);
    Serial.print(", ");
    Serial.print(xAxisGNoCal, 2);
    Serial.print(", ");
    Serial.print(xAxisRawCal);
    Serial.print(", ");
    Serial.println(xAxisGCal);

    Serial.print("Y Axis (raw no cal, G no cal, raw cal, G cal): ");
    Serial.print(yAxisRaw);
    Serial.print(", ");
    Serial.print(yAxisGNoCal, 2);
    Serial.print(", ");
    Serial.print(yAxisRawCal);
    Serial.print(", ");
    Serial.println(yAxisGCal);

    Serial.print("Z Axis (raw no cal, G no cal, raw cal, G cal): ");
    Serial.print(zAxisRaw);
    Serial.print(", ");
    Serial.print(zAxisGNoCal, 2);
    Serial.print(", ");
    Serial.print(zAxisRawCal);
    Serial.print(", ");
    Serial.println(zAxisGCal);

    Serial.println("");
}

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    Wire.setClock(400E3);
    mpuStart();
    mpuSetAccelRange(MPU_ACCEL_RANGE);
    delay(100);
    calibrate();
}

void loop()
{
    // debugSampleWithCalibration();
    delay(3000);
}
