#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <TaskScheduler.h>
#include "mpu6050.h"

// ==== Test Parameters ====
#define SAMPLE_RATE_HZ 500UL
#define SAMPLE_DELAY_MS 10UL * 1000UL
#define SAMPLE_WINDOW_MS 30UL * 1000UL
#define SAMPLE_PERIOD_MS 1000UL / SAMPLE_RATE_HZ
#define N_DECIMALS 3

#define N_SAMPLES (unsigned long) (SAMPLE_WINDOW_MS / SAMPLE_PERIOD_MS)

// ==== Calibration Parameters ====
#define X_ACCEL_OFFSET 0
#define Y_ACCEL_OFFSET 0
#define Z_ACCEL_OFFSET 0

// ==== SD Parameters ====
#define SD_CS 10

// ==== MPU Parameters ====
#define MPU_ACCEL_RANGE MPU_ACCEL_RANGE_2G
#define MPU_ACCEL_RANGE_LSB MPU_ACCEL_RANGE_2G_LSB

File logFile;

void getSample();
bool getSampleOnEnable();
void getSampleOnDisable();

Scheduler runner;
Task getSampleTask(SAMPLE_PERIOD_MS, N_SAMPLES, getSample);

void writeAccelValuesToSD(float xAccelG, float yAccelG, float zAceelG, unsigned long time)
{

}

void getSample()
{
    int16_t xAccelRaw;
    int16_t yAccelRaw;
    int16_t zAccelRaw;
    unsigned long timeMillis;

    mpuRead3AccelAxesRaw(&xAccelRaw, &yAccelRaw, &zAccelRaw, &timeMillis);

    float xAccelG = (float) (xAccelRaw + X_ACCEL_OFFSET) / MPU_ACCEL_RANGE_LSB;
    float yAccelG = (float) (yAccelRaw + Y_ACCEL_OFFSET) / MPU_ACCEL_RANGE_LSB;
    float zAccelG = (float) (zAccelRaw + Z_ACCEL_OFFSET) / MPU_ACCEL_RANGE_LSB;

    writeAccelValuesToSD(xAccelG, yAccelG, zAccelG, timeMillis);
}

bool getSampleOnEnable()
{
    return true;
}

void getSampleOnDisable()
{

}

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    Wire.setClock(400E3);
    mpuStart();
    mpuSetAccelRange(MPU_ACCEL_RANGE);

    if (!SD.begin(SD_CS))
    {
        Serial.println("Unable to connect to SD card");
        while (1);
    }

    getSampleTask.setOnEnable(getSampleOnEnable);
    getSampleTask.setOnDisable(getSampleOnDisable);

    getSampleTask.enableDelayed(SAMPLE_DELAY_MS);
}

void loop()
{
    runner.execute();
}
