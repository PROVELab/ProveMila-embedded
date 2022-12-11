#include <Arduino.h>
#include <Wire.h>
#include "mpu6050.h"

// ==== Test Parameters ====
#define SAMPLE_RATE_HZ 500UL
#define SAMPLE_DELAY_MS 10UL * 1000UL
#define SAMPLE_WINDOW_MS 30UL * 1000UL
#define SAMPLE_PERIOD_MS 1000UL / SAMPLE_PERIOD_HZ
#define N_DECIMALS 3

// ==== Calibration Parameters ====
#define X_ACCEL_OFFSET 0
#define Y_ACCEL_OFFSET 0
#define Z_ACCEL_OFFSET 0

// ==== SD Parameters ====
#define SD_CS 10

// ==== MPU Parameters ====
#define MPU_ACCEL_RANGE MPU_ACCEL_RANGE_2G
#define MPU_ACCEL_RANGE_LSB MPU_ACCEL_RANGE_2G_LSB

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
